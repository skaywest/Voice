#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class VoiceCallOfDutyAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit VoiceCallOfDutyAudioProcessorEditor (VoiceCallOfDutyAudioProcessor&);
    ~VoiceCallOfDutyAudioProcessorEditor() override = default;
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseUp (const juce::MouseEvent& event) override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    VoiceCallOfDutyAudioProcessor& processor;
    juce::ComboBox presetBox, userPresetBox;
    juce::TextEditor userPresetName;
    juce::TextButton saveUserPresetButton { "SAVE" }, loadUserPresetButton { "LOAD" };
    juce::TextButton exportUserPresetsButton { "EXPORT" }, importUserPresetsButton { "IMPORT" };
    std::unique_ptr<juce::FileChooser> presetFileChooser;
    juce::Slider amountSlider, dryWetSlider;
    juce::ToggleButton radioButton, reverbButton, echoButton;
    juce::Slider radioCutoff, radioDistortion, radioGain, radioNoise, reverbMix, reverbDecay, reverbDamping, echoTime, echoFeedback, echoMix, echoWidth;
    juce::ComboBox radioType;
    juce::Label titleLabel, amountLabel, radioLabel, reverbLabel, echoLabel, globalLabel;
    juce::Rectangle<int> mapBounds;
    enum class EffectPage { radio, reverb, echo };
    EffectPage selectedPage = EffectPage::radio;
    std::unique_ptr<ComboAttachment> presetAttachment, radioTypeAttachment;
    std::unique_ptr<SliderAttachment> amountAttachment, dryWetAttachment;
    std::unique_ptr<ButtonAttachment> radioButtonAttachment, reverbButtonAttachment, echoButtonAttachment;
    std::unique_ptr<SliderAttachment> radioCutoffAttachment, radioDistortionAttachment, radioGainAttachment, radioNoiseAttachment;
    std::unique_ptr<SliderAttachment> reverbMixAttachment, reverbDecayAttachment, reverbDampingAttachment;
    std::unique_ptr<SliderAttachment> echoTimeAttachment, echoFeedbackAttachment, echoMixAttachment, echoWidthAttachment;

    void styleSlider (juce::Slider& slider, const juce::String& suffix = {});
    void styleToggle (juce::ToggleButton& button);
    void styleLabel (juce::Label& label, const juce::String& text, int size, juce::Colour colour);
    void toggleEffectFromMap (const juce::Point<int>& point);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoiceCallOfDutyAudioProcessorEditor)
};
