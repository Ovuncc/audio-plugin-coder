#pragma once

#include <array>
#include <complex>
#include <memory>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "ParameterIDs.hpp"

class MorphantAudioProcessor : public juce::AudioProcessor
{
public:
    MorphantAudioProcessor();
    ~MorphantAudioProcessor() override;

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
    enum class ProcessingMode
    {
        Filterbank = 0,
        Spectral = 1
    };

    static constexpr int kStableBands = 48;
    static constexpr int kHybridBands = 96;
    static constexpr int kExperimentalBands = 144;
    static constexpr int kMaxBands = kExperimentalBands;

    struct Band
    {
        juce::dsp::StateVariableTPTFilter<float> modFilter;
        juce::dsp::StateVariableTPTFilter<float> carFilter;
        float envelope = 0.0f;
    };

    juce::AudioProcessorValueTreeState apvts_;

    std::array<Band, kMaxBands> bands_;
    int activeBands_ = kStableBands;
    double sampleRateHz_ = 44100.0;
    ProcessingMode previousMode_ = ProcessingMode::Filterbank;

    juce::AudioBuffer<float> dryBuffer_;
    juce::AudioBuffer<float> carrierBuffer_;
    juce::AudioBuffer<float> wetBuffer_;
    juce::AudioBuffer<float> modulatorMonoBuffer_;
    juce::AudioBuffer<float> analysisMonoBuffer_;

    juce::SmoothedValue<float> pitchScaleSmoother_;
    juce::SmoothedValue<float> filterbankMakeupSmoother_;
    juce::SmoothedValue<float> spectralMakeupSmoother_;
    juce::SmoothedValue<float> mixSmoother_;
    juce::SmoothedValue<float> outputGainSmoother_;

    juce::Random noiseRandom_;
    float modulatorHistory_ = 0.0f;
    float modulatorGateEnv_ = 0.0f;
    float unvoicedEnv_ = 0.0f;
    float noiseHpStateL_ = 0.0f;
    float noiseHpStateR_ = 0.0f;
    float noiseHpInputL_ = 0.0f;
    float noiseHpInputR_ = 0.0f;
    float outputLimiterEnv_ = 0.0f;
    int debugLogCounter_ = 0;
    bool debugWasEnabled_ = false;
    juce::File debugLogFile_;

    int spectralOrder_ = 11;
    int spectralSize_ = 2048;
    std::unique_ptr<juce::dsp::FFT> spectralFft_;
    std::vector<juce::dsp::Complex<float>> spectralCarrierTimeL_;
    std::vector<juce::dsp::Complex<float>> spectralCarrierTimeR_;
    std::vector<juce::dsp::Complex<float>> spectralModTime_;
    std::vector<juce::dsp::Complex<float>> spectralCarrierFreqL_;
    std::vector<juce::dsp::Complex<float>> spectralCarrierFreqR_;
    std::vector<juce::dsp::Complex<float>> spectralModFreq_;
    std::vector<float> spectralWindow_;
    std::vector<float> spectralRingCarrierL_;
    std::vector<float> spectralRingCarrierR_;
    std::vector<float> spectralRingMod_;
    std::vector<float> spectralOutAccumL_;
    std::vector<float> spectralOutAccumR_;
    std::vector<float> spectralModMag_;
    std::vector<float> spectralModMagSmoothed_;
    std::vector<float> spectralCarrierMag_;
    std::vector<float> spectralCarrierMagSmoothed_;
    int spectralWritePos_ = 0;
    int spectralSamplesSinceFrame_ = 0;
    int spectralHopSize_ = 512;

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    static int bandsForReality(float reality) noexcept;
    void configureBandFilters(int bandCount, float pitchScale, float focus, float formantFollower, float reality) noexcept;
    static float computeEnvelopeCoeff(float timeMs, double sampleRate) noexcept;
    float estimatePitchHz(const float* mono, int numSamples, float* confidenceOut = nullptr) const noexcept;
    static float estimateBrightness(const float* mono, int numSamples) noexcept;
    void resetEnvelopes() noexcept;
    void ensureSpectralSetup(int requiredBlockSize);
    ProcessingMode getProcessingMode() const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MorphantAudioProcessor)
};
