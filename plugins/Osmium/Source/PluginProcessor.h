#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <memory>
#include <vector>

#include "ParameterIDs.hpp"

//==============================================================================
/**
*/
class OsmiumAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    OsmiumAudioProcessor();
    ~OsmiumAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Expose APVTS for Editor/Relays
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

private:
   //==============================================================================
   // State
   juce::AudioProcessorValueTreeState apvts;
   juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
   
   // DSP Components - Multiband Transient Shaper

   // Transient Shaper for one band
   class TransientShaper {
   public:
       void prepare(double newSampleRate, int samplesPerBlock, int numChannels) {
           juce::ignoreUnused(samplesPerBlock);
           sampleRate = newSampleRate;
           channelFastEnvelope.assign(numChannels, 0.0f);
           channelSlowEnvelope.assign(numChannels, 0.0f);
           channelAttackGain.assign(numChannels, 1.0f);
           channelCompEnvelope.assign(numChannels, 0.0f);
           channelCompGain.assign(numChannels, 1.0f);
           updateCoefficients();
       }
       
       void setParameters(float attackBoost,
                          float postAttackCompression,
                          float newAttackTimeMs,
                          float newCompTimeMs) {
           attackBoostDb = juce::jmax(0.0f, attackBoost);
           postAttackCompDb = juce::jmax(0.0f, postAttackCompression);
           attackTimeMs = juce::jlimit(2.0f, 40.0f, newAttackTimeMs);
           compTimeMs = juce::jlimit(5.0f, 120.0f, newCompTimeMs);
           updateCoefficients();
       }
       
       void process(juce::AudioBuffer<float>& buffer) {
            auto numChannels = buffer.getNumChannels();
            auto numSamples = buffer.getNumSamples();

            if ((int)channelFastEnvelope.size() != numChannels) {
                channelFastEnvelope.assign(numChannels, 0.0f);
                channelSlowEnvelope.assign(numChannels, 0.0f);
                channelAttackGain.assign(numChannels, 1.0f);
                channelCompEnvelope.assign(numChannels, 0.0f);
                channelCompGain.assign(numChannels, 1.0f);
            }
            
            for (int ch = 0; ch < numChannels; ++ch) {
                auto* data = buffer.getWritePointer(ch);
                float fastEnvelope = channelFastEnvelope[(size_t)ch];
                float slowEnvelope = channelSlowEnvelope[(size_t)ch];
                float attackGain = channelAttackGain[(size_t)ch];
                float compEnvelope = channelCompEnvelope[(size_t)ch];
                float compGain = channelCompGain[(size_t)ch];
                
                for (int i = 0; i < numSamples; ++i) {
                    const float inputSample = data[i];
                    const float sample = std::abs(inputSample);
                    
                    const float fastCoeff = (sample > fastEnvelope) ? fastAttackCoeff : fastReleaseCoeff;
                    const float slowCoeff = (sample > slowEnvelope) ? slowAttackCoeff : slowReleaseCoeff;
                    fastEnvelope = fastCoeff * fastEnvelope + (1.0f - fastCoeff) * sample;
                    slowEnvelope = slowCoeff * slowEnvelope + (1.0f - slowCoeff) * sample;
                    
                    const float envelopeDelta = juce::jmax(0.0f, fastEnvelope - slowEnvelope);
                    const float transientAmount = juce::jlimit(0.0f, 1.0f, (envelopeDelta / (slowEnvelope + 1.0e-4f)) * 2.0f);
                    const float attackGainDb = transientAmount * attackBoostDb;
                    const float targetAttackGain = juce::Decibels::decibelsToGain(attackGainDb);
                    attackGain = attackGainSmoothingCoeff * attackGain + (1.0f - attackGainSmoothingCoeff) * targetAttackGain;

                    const float compEnvelopeCoeff = (transientAmount > compEnvelope) ? compAttackCoeff : compReleaseCoeff;
                    compEnvelope = compEnvelopeCoeff * compEnvelope + (1.0f - compEnvelopeCoeff) * transientAmount;
                    const float compGainDb = -postAttackCompDb * compEnvelope;
                    const float targetCompGain = juce::Decibels::decibelsToGain(compGainDb);
                    compGain = compGainSmoothingCoeff * compGain + (1.0f - compGainSmoothingCoeff) * targetCompGain;
                    
                    data[i] = inputSample * attackGain * compGain;
                }

                channelFastEnvelope[(size_t)ch] = fastEnvelope;
                channelSlowEnvelope[(size_t)ch] = slowEnvelope;
                channelAttackGain[(size_t)ch] = attackGain;
                channelCompEnvelope[(size_t)ch] = compEnvelope;
                channelCompGain[(size_t)ch] = compGain;
            }
        }
        
    private:
        void updateCoefficients() {
            if (sampleRate <= 0.0) {
                return;
            }

            // Fast detector tracks transients, slow detector tracks body/sustain.
            fastAttackCoeff = std::exp(-1.0f / (float)(sampleRate * 0.001f * 1.0f));
            fastReleaseCoeff = std::exp(-1.0f / (float)(sampleRate * 0.001f * 15.0f));
            slowAttackCoeff = std::exp(-1.0f / (float)(sampleRate * 0.001f * attackTimeMs));
            slowReleaseCoeff = std::exp(-1.0f / (float)(sampleRate * 0.001f * 120.0f));
            attackGainSmoothingCoeff = std::exp(-1.0f / (float)(sampleRate * 0.001f * 1.8f));
            const float compAttackMs = juce::jmax(2.0f, compTimeMs * 0.25f);
            compAttackCoeff = std::exp(-1.0f / (float)(sampleRate * 0.001f * compAttackMs));
            compReleaseCoeff = std::exp(-1.0f / (float)(sampleRate * 0.001f * compTimeMs));
            compGainSmoothingCoeff = std::exp(-1.0f / (float)(sampleRate * 0.001f * 1.6f));
        }

        double sampleRate = 44100.0;
        std::vector<float> channelFastEnvelope;
        std::vector<float> channelSlowEnvelope;
        std::vector<float> channelAttackGain;
        std::vector<float> channelCompEnvelope;
        std::vector<float> channelCompGain;
        float fastAttackCoeff = 0.0f;
        float fastReleaseCoeff = 0.0f;
        float slowAttackCoeff = 0.0f;
        float slowReleaseCoeff = 0.0f;
        float attackGainSmoothingCoeff = 0.0f;
        float compAttackCoeff = 0.0f;
        float compReleaseCoeff = 0.0f;
        float compGainSmoothingCoeff = 0.0f;
        float attackBoostDb = 0.0f;
        float postAttackCompDb = 0.0f;
        float attackTimeMs = 5.0f;
        float compTimeMs = 28.0f;
    };
   
   enum class ProcessingMode
   {
       osmium = 0,
       tight,
       chaotic
   };

   enum class SaturationBand
   {
       low,
       high
   };

   enum class WaveShaperType
   {
       sine = 0,
       softSign,
       hardClip
   };

   float applyWaveShaper(float input, WaveShaperType type) const;
   void applySaturationStage(juce::AudioBuffer<float>& bandBuffer,
                             float mix,
                             float drive,
                             int oversamplingMode,
                             WaveShaperType waveShaperType,
                             SaturationBand band);
   void applyTightnessStage(juce::AudioBuffer<float>& buffer,
                            float sustainDepthDb,
                            float sustainReleaseMs,
                            float bellFreqHz,
                            float bellCutDb,
                            float bellQ,
                            float highShelfCutDb,
                            float highShelfFreqHz,
                            int lookaheadSamples);
   void applyOutputSoftClipper(juce::AudioBuffer<float>& buffer,
                               float ceilingDb,
                               float drive) const;

   // Multiband processing components
   juce::dsp::LinkwitzRileyFilter<float> lowpassFilter;  // For low band
   juce::dsp::LinkwitzRileyFilter<float> highpassFilter; // For high band
   
   // Low band (20-150Hz) - Clean transient boost
   TransientShaper lowBandTransientShaper;
   juce::dsp::Limiter<float> lowBandLimiter;
   
   // High band (150Hz+) - Aggressive processing
   TransientShaper highBandTransientShaper;

   // Saturation oversampling path
   std::unique_ptr<juce::dsp::Oversampling<float>> lowOversampling4x;
   std::unique_ptr<juce::dsp::Oversampling<float>> lowOversampling8x;
   std::unique_ptr<juce::dsp::Oversampling<float>> highOversampling4x;
   std::unique_ptr<juce::dsp::Oversampling<float>> highOversampling8x;

   juce::dsp::FirstOrderTPTFilter<float> lowPreFilterOff;
   juce::dsp::FirstOrderTPTFilter<float> lowPostFilterOff;
   juce::dsp::FirstOrderTPTFilter<float> lowPreFilter4x;
   juce::dsp::FirstOrderTPTFilter<float> lowPostFilter4x;
   juce::dsp::FirstOrderTPTFilter<float> lowPreFilter8x;
   juce::dsp::FirstOrderTPTFilter<float> lowPostFilter8x;

   juce::dsp::FirstOrderTPTFilter<float> highPreFilterOff;
   juce::dsp::FirstOrderTPTFilter<float> highPostFilterOff;
   juce::dsp::FirstOrderTPTFilter<float> highPreFilter4x;
   juce::dsp::FirstOrderTPTFilter<float> highPostFilter4x;
   juce::dsp::FirstOrderTPTFilter<float> highPreFilter8x;
   juce::dsp::FirstOrderTPTFilter<float> highPostFilter8x;

   // Tight mode dynamic shaping filters
   juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> tightBellFilter;
   juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> tightHighShelfFilter;
   std::vector<float> tightFastEnvelope;
   std::vector<float> tightSlowEnvelope;
   std::vector<float> tightProgramEnvelope;
   std::vector<float> tightGainDbEnvelope;
   std::vector<std::vector<float>> tightLookaheadBuffers;
   std::vector<int> tightLookaheadWritePositions;
   int tightLookaheadBufferLength = 0;
   int tightLookaheadSamplesCurrent = 0;
   int tightReportedLatencySamples = -1;

    // Output
    juce::dsp::Gain<float> outputGain;
   
   // Buffers for multiband processing
   juce::AudioBuffer<float> lowBandBuffer;
   juce::AudioBuffer<float> highBandBuffer;

   // Parameter Smoothing
   juce::SmoothedValue<float> smoothedIntensity;
   juce::SmoothedValue<float> smoothedOutput;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OsmiumAudioProcessor)
};
