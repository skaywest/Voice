#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>

class VoiceCallOfDutyAudioProcessor final : public juce::AudioProcessor
{
public:
    VoiceCallOfDutyAudioProcessor();
    ~VoiceCallOfDutyAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override;
    int getNumPrograms() override { return 6; }
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }
    void loadFactoryPreset (int index);
    void saveUserPreset (int slot, const juce::String& name);
    void loadUserPreset (int slot);
    juce::String getUserPresetName (int slot) const;
    bool exportUserPresets (const juce::File& file) const;
    bool importUserPresets (const juce::File& file);
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    juce::AudioProcessorValueTreeState apvts;
    juce::ValueTree userPresets { "USER_PRESETS" };

    juce::Reverb reverb;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> radioDrive;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> radioGain;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> radioNoise;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> radioEnable;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> reverbMixSmooth;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> reverbRoomSmooth;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> reverbDampingSmooth;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> globalWetSmooth;
    juce::AudioBuffer<float> dryBuffer;
    std::array<float, 2> hpPreviousInput { 0.0f, 0.0f };
    std::array<float, 2> hpPreviousOutput { 0.0f, 0.0f };
    std::array<float, 2> lpState { 0.0f, 0.0f };
    std::array<float, 2> radioDcPreviousInput { 0.0f, 0.0f };
    std::array<float, 2> radioDcPreviousOutput { 0.0f, 0.0f };
    float radioDcBlockCoefficient = 0.997f;
    float hpAlpha = 0.95f;
    float lpAlpha = 0.2f;
    float targetHpAlpha = 0.95f;
    float targetLpAlpha = 0.2f;
    float radioFilterSmoothingCoefficient = 0.001f;
    uint32_t noiseState = 0x6d2b79f5u;

    std::array<std::vector<float>, 2> delayBuffer;
    int delayWritePosition = 0;
    std::array<float, 2> echoDampingState { 0.0f, 0.0f };
    float echoDampingCoefficient = 0.35f;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> delayTimeSamples;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> delayFeedbackSmooth;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> delayMixSmooth;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> delayWidthSmooth;
    double currentSampleRate = 44100.0;

    float getParam (const char* id) const noexcept;
    void updateRadioFilters();
    void processRadio (juce::AudioBuffer<float>& buffer, int numSamples, float amount);
    void processEcho (juce::AudioBuffer<float>& buffer, int numSamples);
    void processReverb (juce::AudioBuffer<float>& buffer, int numSamples);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoiceCallOfDutyAudioProcessor)
};
