#pragma once

#include <array>
#include <random>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "ParameterIDs.hpp"

class NeuralPanicAudioProcessor : public juce::AudioProcessor
{
public:
    NeuralPanicAudioProcessor();
    ~NeuralPanicAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
   #endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts_; }

private:
    juce::AudioProcessorValueTreeState apvts_;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    double sampleRateHz_ = 44100.0;
    int maximumBlockSize_ = 512;

    // Core buffers.
    juce::AudioBuffer<float> dryBuffer_;
    juce::AudioBuffer<float> wetBuffer_;

    // Dynamics and safety chain.
    juce::dsp::Compressor<float> meltdownCompressor_;
    juce::dsp::WaveShaper<float> meltdownSaturator_;
    juce::dsp::Limiter<float> outputLimiter_;
    float saturatorDrive_ = 1.0f;

    // Smoothed controls.
    juce::SmoothedValue<float> panicSmoother_;
    juce::SmoothedValue<float> mixSmoother_;
    juce::SmoothedValue<float> outputGainSmoother_;
    juce::SmoothedValue<float> broadcastSmoother_;
    juce::SmoothedValue<float> smearSmoother_;

    // Input-driven dynamics + safety gate.
    float inputPrevSample_ = 0.0f;
    float inputRmsSmoothed_ = 0.0f;
    float inputDeltaSmoothed_ = 0.0f;
    float inputActivity_ = 0.0f;
    float inputMotion_ = 0.0f;
    bool gateOpenState_ = false;
    float gateEnvelope_ = 0.0f;
    float gateAttackCoeff_ = 0.0f;
    float gateReleaseCoeff_ = 0.0f;
    int gateHoldSamples_ = 0;
    int gateHoldCounter_ = 0;
    std::vector<float> gateBuffer_;
    int silenceSampleCounter_ = 0;
    int silenceResetSamples_ = 0;
    bool deepSilenceLatched_ = false;

    // Deterministic randomness.
    std::mt19937 rng_;
    std::uniform_real_distribution<float> random01_ { 0.0f, 1.0f };
    int currentSeed_ = -1;

    // Codec stress (decimation + quantization).
    std::array<int, 2> decimCounter_ { 0, 0 };
    std::array<float, 2> heldSample_ { 0.0f, 0.0f };

    // Jitter stage.
    int jitterDelaySize_ = 1;
    int jitterWritePos_ = 0;
    float jitterOffsetCurrent_ = 0.0f;
    float jitterOffsetTarget_ = 0.0f;
    int jitterUpdateCounter_ = 0;
    std::array<std::vector<float>, 2> jitterDelay_;

    // Vocoder instability states.
    std::array<float, 2> lowState_ { 0.0f, 0.0f };
    std::array<float, 2> highState_ { 0.0f, 0.0f };
    double driftPhase_ = 0.0;
    float driftRandom_ = 0.0f;
    int driftRandomCounter_ = 0;
    std::array<float, 2> identityPrev_ { 0.0f, 0.0f };
    std::array<float, 2> identityColor_ { 0.0f, 0.0f };
    int identityEventCounter_ = 0;
    float identityEventDepth_ = 0.0f;

    // Temporal smear buffer.
    int smearBufferSize_ = 1;
    int smearWritePos_ = 0;
    float smearReadOffset_ = 0.0f;
    int smearRandomCounter_ = 0;
    int smearFreezeCounter_ = 0;
    std::array<float, 2> smearFrozen_ { 0.0f, 0.0f };
    std::array<std::vector<float>, 2> smearBuffer_;

    void resetInternalState();
    void updateRandomSeed(int seedValue);
    float randomSigned();
    float readCircularInterpolated(const std::vector<float>& buffer, int writePos, float delaySamples) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NeuralPanicAudioProcessor)
};
