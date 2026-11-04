#include "PluginEditor.h"

namespace
{
    const juce::Colour background (0xff080b0b);
    const juce::Colour panel (0xff101515);
    const juce::Colour orange (0xffff7900);
    const juce::Colour muted (0xff7d8582);
    const juce::Colour pale (0xffd6d8d2);
}

VoiceCallOfDutyAudioProcessorEditor::VoiceCallOfDutyAudioProcessorEditor (VoiceCallOfDutyAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (600, 900);
    setResizable (true, true);
    setResizeLimits (560, 860, 820, 1180);

    titleLabel.setJustificationType (juce::Justification::centred);
    styleLabel (titleLabel, "VOICE CALL OF DUTY", 21, pale);
    addAndMakeVisible (titleLabel);

    presetBox.addItemList ({ "FIELD OPS", "SAFE HOUSE", "CASINO HEIST", "SABOTAGE", "GALA", "REDACTED" }, 1);
    presetBox.setColour (juce::ComboBox::backgroundColourId, panel);
    presetBox.setColour (juce::ComboBox::textColourId, orange);
    presetBox.setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff343b38));
    addAndMakeVisible (presetBox);
    presetAttachment = std::make_unique<ComboAttachment> (processor.getAPVTS(), "Preset", presetBox);
    presetBox.onChange = [this] { processor.loadFactoryPreset (presetBox.getSelectedItemIndex()); };

    userPresetBox.setColour (juce::ComboBox::backgroundColourId, panel);
    userPresetBox.setColour (juce::ComboBox::textColourId, pale);
    userPresetBox.setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff343b38));
    addAndMakeVisible (userPresetBox);
    for (int i = 0; i < 8; ++i)
        userPresetBox.addItem (processor.getUserPresetName (i), i + 1);
    userPresetBox.setSelectedId (1, juce::dontSendNotification);
    userPresetName.setText ("MY PRESET", juce::dontSendNotification);
    userPresetName.setColour (juce::TextEditor::backgroundColourId, panel);
    userPresetName.setColour (juce::TextEditor::textColourId, pale);
    userPresetName.setColour (juce::TextEditor::outlineColourId, juce::Colour (0xff343b38));
    userPresetName.setInputRestrictions (24);
    addAndMakeVisible (userPresetName);

    for (auto* button : { &saveUserPresetButton, &loadUserPresetButton, &exportUserPresetsButton, &importUserPresetsButton })
    {
        button->setColour (juce::TextButton::buttonColourId, panel);
        button->setColour (juce::TextButton::textColourOffId, orange);
        addAndMakeVisible (*button);
    }
    saveUserPresetButton.onClick = [this]
    {
        const int slot = juce::jmax (0, userPresetBox.getSelectedId() - 1);
        processor.saveUserPreset (slot, userPresetName.getText());
        userPresetBox.changeItemText (slot + 1, processor.getUserPresetName (slot));
    };
    loadUserPresetButton.onClick = [this]
    {
        processor.loadUserPreset (juce::jmax (0, userPresetBox.getSelectedId() - 1));
    };
    exportUserPresetsButton.onClick = [this]
    {
        presetFileChooser = std::make_unique<juce::FileChooser> ("Export user presets", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory), "*.xml");
        auto safeThis = juce::Component::SafePointer<VoiceCallOfDutyAudioProcessorEditor> (this);
        presetFileChooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
            [safeThis] (const juce::FileChooser& chooser)
            {
                if (safeThis == nullptr) return;
                auto file = chooser.getResult();
                if (file == juce::File()) return;
                if (! file.hasFileExtension (".xml")) file = file.withFileExtension (".xml");
                const bool ok = safeThis->processor.exportUserPresets (file);
                juce::AlertWindow::showMessageBoxAsync (ok ? juce::MessageBoxIconType::InfoIcon : juce::MessageBoxIconType::WarningIcon,
                    "Preset export", ok ? "User preset slots exported." : "Could not write the preset file.");
            });
    };
    importUserPresetsButton.onClick = [this]
    {
        presetFileChooser = std::make_unique<juce::FileChooser> ("Import user presets", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory), "*.xml");
        auto safeThis = juce::Component::SafePointer<VoiceCallOfDutyAudioProcessorEditor> (this);
        presetFileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [safeThis] (const juce::FileChooser& chooser)
            {
                if (safeThis == nullptr) return;
                const auto file = chooser.getResult();
                if (file == juce::File()) return;
                const bool ok = safeThis->processor.importUserPresets (file);
                if (ok)
                    for (int i = 0; i < 8; ++i) safeThis->userPresetBox.changeItemText (i + 1, safeThis->processor.getUserPresetName (i));
                juce::AlertWindow::showMessageBoxAsync (ok ? juce::MessageBoxIconType::InfoIcon : juce::MessageBoxIconType::WarningIcon,
                    "Preset import", ok ? "User preset slots imported." : "Invalid or unsupported preset file. Existing slots were kept.");
            });
    };

    styleSlider (amountSlider, "%"); amountSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    styleLabel (amountLabel, "AMOUNT", 11, muted); addAndMakeVisible (amountLabel); addAndMakeVisible (amountSlider);
    amountAttachment = std::make_unique<SliderAttachment> (processor.getAPVTS(), "Amount", amountSlider);

    styleSlider (dryWetSlider, "%"); dryWetSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    styleLabel (globalLabel, "GLOBAL DRY / WET", 11, muted); addAndMakeVisible (globalLabel); addAndMakeVisible (dryWetSlider);
    dryWetAttachment = std::make_unique<SliderAttachment> (processor.getAPVTS(), "DryWetMix", dryWetSlider);

    styleToggle (radioButton); radioButton.setButtonText ("RADIO"); addAndMakeVisible (radioButton);
    styleToggle (reverbButton); reverbButton.setButtonText ("REVERB"); addAndMakeVisible (reverbButton);
    styleToggle (echoButton); echoButton.setButtonText ("ECHO"); addAndMakeVisible (echoButton);
    radioButtonAttachment = std::make_unique<ButtonAttachment> (processor.getAPVTS(), "RadioToggle", radioButton);
    reverbButtonAttachment = std::make_unique<ButtonAttachment> (processor.getAPVTS(), "ReverbToggle", reverbButton);
    echoButtonAttachment = std::make_unique<ButtonAttachment> (processor.getAPVTS(), "DelayToggle", echoButton);

    styleLabel (radioLabel, "01  /  RADIO COMMUNICATION", 13, orange); addAndMakeVisible (radioLabel);
    styleLabel (reverbLabel, "02  /  REVERB SPACE", 13, orange); addAndMakeVisible (reverbLabel);
    styleLabel (echoLabel, "03  /  ECHO TRANSMISSION", 13, orange); addAndMakeVisible (echoLabel);

    auto addKnob = [this] (juce::Slider& s, const juce::String& suffix)
    {
        styleSlider (s, suffix); s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag); addAndMakeVisible (s);
    };
    addKnob (radioCutoff, " Hz"); addKnob (radioDistortion, "%"); addKnob (radioGain, " dB"); addKnob (radioNoise, "%");
    addKnob (reverbMix, "%"); addKnob (reverbDecay, ""); addKnob (reverbDamping, "%");
    addKnob (echoTime, " s"); addKnob (echoFeedback, "%"); addKnob (echoMix, "%"); addKnob (echoWidth, "%");

    radioType.addItemList ({ "CLASSIC RADIO", "TACTICAL RADIO" }, 1);
    radioType.setColour (juce::ComboBox::backgroundColourId, panel);
    radioType.setColour (juce::ComboBox::textColourId, pale);
    radioType.setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff343b38));
    addAndMakeVisible (radioType);

    radioCutoff.setTooltip ("Radio band-pass upper cutoff frequency");
    radioDistortion.setTooltip ("Radio saturation amount");
    radioGain.setTooltip ("Radio output gain / demodulation level");
    radioNoise.setTooltip ("Add controlled radio static noise");
    reverbMix.setTooltip ("Reverb wet level");
    reverbDecay.setTooltip ("Approximate reverb room size / decay");
    reverbDamping.setTooltip ("High-frequency damping: higher values sound darker");
    echoTime.setTooltip ("Delay time in seconds");
    echoFeedback.setTooltip ("Amount of signal fed back into the delay");
    echoMix.setTooltip ("Echo level");
    echoWidth.setTooltip ("Stereo width of the echo repeats");

    auto& state = processor.getAPVTS();
    radioCutoffAttachment = std::make_unique<SliderAttachment> (state, "RadioCutoffFreq", radioCutoff);
    radioDistortionAttachment = std::make_unique<SliderAttachment> (state, "RadioDistortionAmount", radioDistortion);
    radioGainAttachment = std::make_unique<SliderAttachment> (state, "RadioDemodulationGain", radioGain);
    radioNoiseAttachment = std::make_unique<SliderAttachment> (state, "RadioNoise", radioNoise);
    radioTypeAttachment = std::make_unique<ComboAttachment> (state, "RadioType", radioType);
    reverbMixAttachment = std::make_unique<SliderAttachment> (state, "ReverbMix", reverbMix);
    reverbDecayAttachment = std::make_unique<SliderAttachment> (state, "ReverbDecayTime", reverbDecay);
    reverbDampingAttachment = std::make_unique<SliderAttachment> (state, "ReverbDamping", reverbDamping);
    echoTimeAttachment = std::make_unique<SliderAttachment> (state, "DelayTime", echoTime);
    echoFeedbackAttachment = std::make_unique<SliderAttachment> (state, "DelayFeedback", echoFeedback);
    echoMixAttachment = std::make_unique<SliderAttachment> (state, "DelayMix", echoMix);
    echoWidthAttachment = std::make_unique<SliderAttachment> (state, "DelayWidth", echoWidth);
}

void VoiceCallOfDutyAudioProcessorEditor::styleSlider (juce::Slider& s, const juce::String& suffix)
{
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 76, 18);
    s.setTextValueSuffix (suffix);
    s.setColour (juce::Slider::rotarySliderFillColourId, orange);
    s.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff303735));
    s.setColour (juce::Slider::thumbColourId, orange);
    s.setColour (juce::Slider::trackColourId, orange);
    s.setColour (juce::Slider::backgroundColourId, juce::Colour (0xff242b28));
    s.setColour (juce::Slider::textBoxTextColourId, pale);
    s.setColour (juce::Slider::textBoxBackgroundColourId, background);
    s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0xff303735));
}

void VoiceCallOfDutyAudioProcessorEditor::styleToggle (juce::ToggleButton& b)
{
    b.setColour (juce::ToggleButton::textColourId, pale);
    b.setColour (juce::ToggleButton::tickColourId, orange);
    b.setColour (juce::ToggleButton::tickDisabledColourId, muted);
}

void VoiceCallOfDutyAudioProcessorEditor::styleLabel (juce::Label& l, const juce::String& text, int size, juce::Colour colour)
{
    l.setText (text, juce::dontSendNotification);
    l.setFont (juce::Font ((float) size, juce::Font::bold));
    l.setColour (juce::Label::textColourId, colour);
}

void VoiceCallOfDutyAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (background);
    auto bounds = getLocalBounds().toFloat().reduced (12.0f);
    g.setColour (juce::Colour (0xff1b2421));
    g.drawRoundedRectangle (bounds, 7.0f, 1.0f);
    g.setColour (panel);
    g.fillRoundedRectangle (bounds.reduced (5.0f), 5.0f);

    // Tactical map panel: lightweight vector streets and grid, no external assets.
    auto map = mapBounds.toFloat();
    if (! map.isEmpty())
    {
        g.setColour (juce::Colour (0xff080d0c));
        g.fillRoundedRectangle (map, 5.0f);
        g.saveState();
        g.reduceClipRegion (map.toNearestInt());
        g.setColour (juce::Colour (0x243b5149));
        for (int x = map.getX(); x < map.getRight(); x += 13)
            g.drawVerticalLine (x, map.getY(), map.getBottom());
        for (int y = map.getY(); y < map.getBottom(); y += 13)
            g.drawHorizontalLine (y, map.getX(), map.getRight());

        g.setColour (juce::Colour (0x453b4843));
        juce::Path roads;
        roads.startNewSubPath (map.getX() - 15.0f, map.getY() + 55.0f);
        roads.lineTo (map.getX() + map.getWidth() * 0.28f, map.getY() + 95.0f);
        roads.lineTo (map.getX() + map.getWidth() * 0.52f, map.getY() + 70.0f);
        roads.lineTo (map.getRight() + 20.0f, map.getY() + 145.0f);
        roads.startNewSubPath (map.getX() + 30.0f, map.getBottom() + 10.0f);
        roads.lineTo (map.getX() + map.getWidth() * 0.42f, map.getY() + 145.0f);
        roads.lineTo (map.getRight() - 35.0f, map.getY() + 35.0f);
        roads.startNewSubPath (map.getX() + map.getWidth() * 0.12f, map.getY() - 10.0f);
        roads.lineTo (map.getX() + map.getWidth() * 0.32f, map.getY() + 125.0f);
        roads.lineTo (map.getX() + map.getWidth() * 0.18f, map.getBottom() + 15.0f);
        roads.startNewSubPath (map.getX() + map.getWidth() * 0.68f, map.getY() - 10.0f);
        roads.lineTo (map.getX() + map.getWidth() * 0.58f, map.getY() + 110.0f);
        roads.lineTo (map.getRight() + 10.0f, map.getBottom() - 25.0f);
        g.strokePath (roads, juce::PathStrokeType (2.2f));
        g.setColour (juce::Colour (0x253b5149));
        for (int i = 0; i < 5; ++i)
        {
            const float x = map.getX() + 22.0f + i * (map.getWidth() - 44.0f) / 4.0f;
            g.drawLine (x, map.getY() + 12.0f, x - 18.0f, map.getBottom() - 12.0f, 1.0f);
        }
        g.restoreState();

        auto marker = [&g] (float x, float y, const juce::String& name, bool active)
        {
            const auto c = active ? orange : juce::Colour (0xff626b67);
            g.setColour (c.withAlpha (0.18f));
            g.fillEllipse (x - 13.0f, y - 13.0f, 26.0f, 26.0f);
            g.setColour (c);
            g.fillEllipse (x - 6.0f, y - 6.0f, 12.0f, 12.0f);
            g.setColour (active ? orange : muted);
            g.setFont (juce::Font (10.0f, juce::Font::bold));
            g.drawText (name, (int) x - 45, (int) y - 29, 90, 14, juce::Justification::centred);
        };
        const auto isEnabled = [this] (const char* id)
        {
            if (auto* value = processor.getAPVTS().getRawParameterValue (id))
                return value->load() >= 0.5f;
            return false;
        };

        marker (map.getX() + map.getWidth() * 0.34f, map.getY() + map.getHeight() * 0.30f,
                "RADIO", isEnabled ("RadioToggle"));
        marker (map.getX() + map.getWidth() * 0.55f, map.getY() + map.getHeight() * 0.61f,
                "REVERB", isEnabled ("ReverbToggle"));
        marker (map.getX() + map.getWidth() * 0.24f, map.getY() + map.getHeight() * 0.78f,
                "ECHO", isEnabled ("DelayToggle"));
        g.setColour (juce::Colour (0xff303b36));
        g.drawRoundedRectangle (map, 5.0f, 1.0f);
    }

    g.setColour (orange);
    g.fillRect (22, 54, getWidth() - 44, 2);
    g.setColour (juce::Colour (0xff202a26));
    for (auto panelBounds : { juce::Rectangle<float> (22.0f, (float) mapBounds.getBottom() + 10.0f, (float) getWidth() - 44.0f, 135.0f),
                              juce::Rectangle<float> (22.0f, (float) mapBounds.getBottom() + 153.0f, (float) getWidth() - 44.0f, 100.0f),
                              juce::Rectangle<float> (22.0f, (float) mapBounds.getBottom() + 261.0f, (float) getWidth() - 44.0f, 105.0f) })
        g.drawRoundedRectangle (panelBounds, 5.0f, 1.0f);
}

void VoiceCallOfDutyAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (26);
    titleLabel.setBounds (area.removeFromTop (30));
    area.removeFromTop (10);

    auto top = area.removeFromTop (96);
    auto selectorArea = top.removeFromLeft (juce::jmax (250, top.getWidth() - 145));
    presetBox.setBounds (selectorArea.removeFromTop (34).reduced (4, 2));
    auto userRow = selectorArea.removeFromTop (34);
    userPresetBox.setBounds (userRow.removeFromLeft (76).reduced (2, 2));
    userPresetName.setBounds (userRow.removeFromLeft (88).reduced (2, 2));
    saveUserPresetButton.setBounds (userRow.removeFromLeft (44).reduced (2, 2));
    loadUserPresetButton.setBounds (userRow.removeFromLeft (44).reduced (2, 2));
    exportUserPresetsButton.setBounds (userRow.removeFromLeft (50).reduced (2, 2));
    importUserPresetsButton.setBounds (userRow.removeFromLeft (50).reduced (2, 2));
    auto amountArea = top;
    amountLabel.setBounds (amountArea.removeFromTop (18));
    amountSlider.setBounds (amountArea);

    area.removeFromTop (4);
    mapBounds = area.removeFromTop (250).reduced (8, 4);
    area.removeFromTop (12);

    const bool showRadio = selectedPage == EffectPage::radio;
    const bool showReverb = selectedPage == EffectPage::reverb;
    const bool showEcho = selectedPage == EffectPage::echo;
    const std::array<juce::Component*, 7> radioComponents { &radioLabel, &radioButton, &radioType, &radioCutoff, &radioDistortion, &radioGain, &radioNoise };
    const std::array<juce::Component*, 5> reverbComponents { &reverbLabel, &reverbButton, &reverbMix, &reverbDecay, &reverbDamping };
    const std::array<juce::Component*, 6> echoComponents { &echoLabel, &echoButton, &echoTime, &echoFeedback, &echoMix, &echoWidth };
    for (auto* c : radioComponents) c->setVisible (showRadio);
    for (auto* c : reverbComponents) c->setVisible (showReverb);
    for (auto* c : echoComponents) c->setVisible (showEcho);

    auto panel = area.removeFromTop (210).reduced (12, 6);
    if (showRadio)
    {
        radioLabel.setBounds (panel.removeFromTop (22));
        auto row = panel;
        radioButton.setBounds (row.removeFromLeft (86).withY (row.getY() + 48).withHeight (28));
        radioType.setBounds (row.removeFromLeft (130).withY (row.getY() + 48).withHeight (28));
        const int w = juce::jmax (72, row.getWidth() / 4);
        radioCutoff.setBounds (row.removeFromLeft (w));
        radioDistortion.setBounds (row.removeFromLeft (w));
        radioGain.setBounds (row.removeFromLeft (w));
        radioNoise.setBounds (row);
    }
    else if (showReverb)
    {
        reverbLabel.setBounds (panel.removeFromTop (22));
        auto row = panel;
        reverbButton.setBounds (row.removeFromLeft (100).withY (row.getY() + 48).withHeight (28));
        const int w = row.getWidth() / 3;
        reverbMix.setBounds (row.removeFromLeft (w));
        reverbDecay.setBounds (row.removeFromLeft (w));
        reverbDamping.setBounds (row);
    }
    else
    {
        echoLabel.setBounds (panel.removeFromTop (22));
        auto row = panel;
        echoButton.setBounds (row.removeFromLeft (100).withY (row.getY() + 48).withHeight (28));
        const int w = juce::jmax (72, row.getWidth() / 4);
        echoTime.setBounds (row.removeFromLeft (w));
        echoFeedback.setBounds (row.removeFromLeft (w));
        echoMix.setBounds (row.removeFromLeft (w));
        echoWidth.setBounds (row);
    }

    area.removeFromTop (6);
    globalLabel.setBounds (area.removeFromTop (18));
    dryWetSlider.setBounds (area.removeFromTop (38));
}


void VoiceCallOfDutyAudioProcessorEditor::mouseUp (const juce::MouseEvent& event)
{
    toggleEffectFromMap (event.getPosition());
}

void VoiceCallOfDutyAudioProcessorEditor::toggleEffectFromMap (const juce::Point<int>& point)
{
    const auto map = mapBounds.toFloat();
    if (map.isEmpty() || ! map.contains (point.toFloat()))
        return;

    struct Marker { EffectPage page; float x, y; };
    const Marker markers[]
    {
        { EffectPage::radio,  0.34f, 0.30f },
        { EffectPage::reverb, 0.55f, 0.61f },
        { EffectPage::echo,   0.24f, 0.78f }
    };

    for (const auto& marker : markers)
    {
        const juce::Point<float> centre (map.getX() + map.getWidth() * marker.x,
                                         map.getY() + map.getHeight() * marker.y);
        if (centre.getDistanceFrom (point.toFloat()) <= 20.0f)
        {
            selectedPage = marker.page;
            resized();
            repaint();
            return;
        }
    }
}
