#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>
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
       
       void setParameters(float attackBoost, float postAttackCompression, float newAttackTimeMs) {
           attackBoostDb = juce::jmax(0.0f, attackBoost);
           postAttackCompDb = juce::jmax(0.0f, postAttackCompression);
           attackTimeMs = juce::jlimit(2.0f, 40.0f, newAttackTimeMs);
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
            compAttackCoeff = std::exp(-1.0f / (float)(sampleRate * 0.001f * 6.0f));
            compReleaseCoeff = std::exp(-1.0f / (float)(sampleRate * 0.001f * 28.0f));
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
    };
   
   // Multiband processing components
   juce::dsp::LinkwitzRileyFilter<float> lowpassFilter;  // For low band
   juce::dsp::LinkwitzRileyFilter<float> highpassFilter; // For high band
   
   // Low band (20-150Hz) - Clean transient boost
   TransientShaper lowBandTransientShaper;
   juce::dsp::Limiter<float> lowBandLimiter;
   
   // High band (150Hz+) - Aggressive processing
   TransientShaper highBandTransientShaper;
   juce::dsp::Gain<float> highBandDrive;
   juce::dsp::WaveShaper<float> highBandSaturator;
   
    // Output
    juce::dsp::Gain<float> outputGain;
    juce::dsp::Limiter<float> outputLimiter;
   
   // Buffers for multiband processing
   juce::AudioBuffer<float> lowBandBuffer;
   juce::AudioBuffer<float> highBandBuffer;

   // Parameter Smoothing
   juce::SmoothedValue<float> smoothedIntensity;
   juce::SmoothedValue<float> smoothedOutput;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OsmiumAudioProcessor)
};
