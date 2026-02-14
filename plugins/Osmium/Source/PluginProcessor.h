#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

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
       void prepare(double sampleRate, int samplesPerBlock) {
           this->sampleRate = sampleRate;
           envelope = 0.0f;
           attackCoeff = std::exp(-1.0f / (sampleRate * 0.001f)); // 1ms attack
           releaseCoeff = std::exp(-1.0f / (sampleRate * 0.05f)); // 50ms release
       }
       
       void setParameters(float attackBoost, float sustainCut, float attackTimeMs) {
           this->attackBoostDb = attackBoost;
           this->sustainCutDb = sustainCut;
           this->attackTimeMs = attackTimeMs;
       }
       
       void process(juce::AudioBuffer<float>& buffer) {
           auto numChannels = buffer.getNumChannels();
           auto numSamples = buffer.getNumSamples();
           
           for (int ch = 0; ch < numChannels; ++ch) {
               auto* data = buffer.getWritePointer(ch);
               
               for (int i = 0; i < numSamples; ++i) {
                   float sample = std::abs(data[i]);
                   
                   // Envelope follower
                   if (sample > envelope)
                       envelope = attackCoeff * envelope + (1.0f - attackCoeff) * sample;
                   else
                       envelope = releaseCoeff * envelope + (1.0f - releaseCoeff) * sample;
                   
                   // Detect transient (rising envelope)
                   float envelopeDelta = sample - envelope;
                   bool isTransient = envelopeDelta > 0.01f;
                   
                   // Calculate gain
                   float gainDb = isTransient ? attackBoostDb : sustainCutDb;
                   float gain = juce::Decibels::decibelsToGain(gainDb);
                   
                   // Apply gain
                   data[i] *= gain;
               }
           }
       }
       
   private:
       double sampleRate = 44100.0;
       float envelope = 0.0f;
       float attackCoeff = 0.0f;
       float releaseCoeff = 0.0f;
       float attackBoostDb = 0.0f;
       float sustainCutDb = 0.0f;
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
   
   // Buffers for multiband processing
   juce::AudioBuffer<float> lowBandBuffer;
   juce::AudioBuffer<float> highBandBuffer;

   // Parameter Smoothing
   juce::SmoothedValue<float> smoothedIntensity;
   juce::SmoothedValue<float> smoothedOutput;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OsmiumAudioProcessor)
};
