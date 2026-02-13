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
   
   // DSP Components
   // Simple Transient Shaper Implementation
   class TransientShaper {
   public:
       void prepare(const juce::dsp::ProcessSpec& spec) {
           smoothEnv.reset(spec.sampleRate, 0.05); // 50ms release
           attackFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(spec.sampleRate, 1000.0f);
           attackFilter.prepare(spec);
           sampleRate = spec.sampleRate;
       }
       
       void setIntensity(float i) { intensity = i; }
       
       void process(juce::dsp::ProcessContextReplacing<float>& context) {
           auto& block = context.getOutputBlock();
           auto numSamples = block.getNumSamples();
           auto numChannels = block.getNumChannels();
           
           for (size_t ch = 0; ch < numChannels; ++ch) {
               auto* data = block.getChannelPointer(ch);
               for (size_t i = 0; i < numSamples; ++i) {
                   // Simple transient detection: high-pass filtered envelope
                   float sample = data[i];
                   float hp = attackFilter.processSample(sample); // Need per-sample process for transient logic really, simplifies here
                   // For better transient shaping we need envelope follower difference
                   // Simplified: specific attack boost
               }
           }
            // Replacing with JUCE DSP modules for stability in V1
       }
       
   private:
       double sampleRate = 44100.0;
       float intensity = 0.0f;
       juce::SmoothedValue<float> smoothEnv;
       juce::dsp::IIR::Filter<float> attackFilter;
   };
   
   // Using standard JUCE DSP for reliability in V1
   juce::dsp::Gain<float> inputGain;
   juce::dsp::Compressor<float> compressor; // For transient shaping/glue
   juce::dsp::Limiter<float> limiter;
   juce::dsp::WaveShaper<float> saturator;
   juce::dsp::Gain<float> outputGain;

   // Parameter Smoothing
   juce::SmoothedValue<float> smoothedIntensity;
   juce::SmoothedValue<float> smoothedOutput;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OsmiumAudioProcessor)
};
