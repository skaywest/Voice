#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
    constexpr int maxChannels = 2;
    const std::array<juce::String, 6> presetNames
    {
        "Field Ops", "Safe House", "Casino Heist", "Sabotage", "Gala", "Redacted"
    };

    juce::AudioParameterFloat* floatParam (juce::AudioProcessorValueTreeState& state, const char* id)
    {
        return dynamic_cast<juce::AudioParameterFloat*> (state.getParameter (id));
    }

    constexpr std::array<const char*, 18> expectedParameterIds
    {
        "Preset", "Amount", "DryWetMix", "RadioToggle", "RadioCutoffFreq",
        "RadioDistortionAmount", "RadioDemodulationGain", "RadioType", "RadioNoise",
        "ReverbToggle", "ReverbMix", "ReverbDecayTime", "ReverbDamping",
        "DelayToggle", "DelayTime", "DelayFeedback", "DelayMix", "DelayWidth"
    };

    bool isValidParameterSnapshot (const juce::ValueTree& parameters, const juce::Identifier& expectedType)
    {
        if (! parameters.isValid() || parameters.getType() != expectedType
            || parameters.getNumChildren() != (int) expectedParameterIds.size())
            return false;

        for (const auto* parameterId : expectedParameterIds)
        {
            int matches = 0;
            juce::ValueTree found;
            for (int i = 0; i < parameters.getNumChildren(); ++i)
            {
                auto candidate = parameters.getChild (i);
                if (candidate.getType() == juce::Identifier ("PARAM")
                    && candidate.getProperty ("id").toString() == parameterId)
                {
                    ++matches;
                    found = candidate;
                }
            }
            if (matches != 1 || ! found.hasProperty ("value"))
                return false;
            const double value = (double) found.getProperty ("value");
            if (! std::isfinite (value) || value < 0.0 || value > 1.0)
                return false;
        }
        return true;
    }

    bool isValidUserPresetTree (const juce::ValueTree& presets, const juce::Identifier& parameterTreeType)
    {
        if (! presets.isValid() || presets.getType() != juce::Identifier ("USER_PRESETS")
            || presets.getNumChildren() > 8)
            return false;

        std::array<bool, 8> seenSlots {};
        for (int i = 0; i < presets.getNumChildren(); ++i)
        {
            auto slot = presets.getChild (i);
            const auto tag = slot.getType().toString();
            if (! tag.startsWith ("SLOT_") || tag.length() != 6
                || tag[5] < '0' || tag[5] > '7' || slot.getNumChildren() != 1
                || slot.getProperty ("name").toString().length() > 24
                || ! isValidParameterSnapshot (slot.getChild (0), parameterTreeType))
                return false;
            const int slotNumber = tag[5] - '0';
            if (seenSlots[(size_t) slotNumber])
                return false;
            seenSlots[(size_t) slotNumber] = true;
        }
        return true;
    }
}

VoiceCallOfDutyAudioProcessor::VoiceCallOfDutyAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout VoiceCallOfDutyAudioProcessor::createParameterLayout()
{
    using APF = juce::AudioParameterFloat;
    using APB = juce::AudioParameterBool;
    using APC = juce::AudioParameterChoice;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    p.push_back (std::make_unique<APC> ("Preset", "Preset", juce::StringArray
        { "Field Ops", "Safe House", "Casino Heist", "Sabotage", "Gala", "Redacted" }, 0));
    p.push_back (std::make_unique<APF> ("Amount", "Amount", juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f));
    p.push_back (std::make_unique<APF> ("DryWetMix", "Dry / Wet", juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f));

    p.push_back (std::make_unique<APB> ("RadioToggle", "Radio", true));
    p.push_back (std::make_unique<APF> ("RadioCutoffFreq", "Radio Cutoff", juce::NormalisableRange<float> (600.0f, 5000.0f, 1.0f, 0.35f), 1300.0f));
    p.push_back (std::make_unique<APF> ("RadioDistortionAmount", "Radio Distortion", juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 25.0f));
    p.push_back (std::make_unique<APF> ("RadioDemodulationGain", "Radio Gain", juce::NormalisableRange<float> (0.0f, 36.0f, 0.1f), 0.0f));
    p.push_back (std::make_unique<APC> ("RadioType", "Radio Type", juce::StringArray { "Classic", "Tactical" }, 0));
    p.push_back (std::make_unique<APF> ("RadioNoise", "Radio Noise", juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f));

    p.push_back (std::make_unique<APB> ("ReverbToggle", "Reverb", true));
    p.push_back (std::make_unique<APF> ("ReverbMix", "Reverb Mix", juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 12.0f));
    p.push_back (std::make_unique<APF> ("ReverbDecayTime", "Reverb Decay", juce::NormalisableRange<float> (0.1f, 1.0f, 0.01f), 0.3f));
    p.push_back (std::make_unique<APF> ("ReverbDamping", "Reverb Damping", juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 55.0f));

    p.push_back (std::make_unique<APB> ("DelayToggle", "Echo", false));
    p.push_back (std::make_unique<APF> ("DelayTime", "Echo Time", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f));
    p.push_back (std::make_unique<APF> ("DelayFeedback", "Echo Feedback", juce::NormalisableRange<float> (0.0f, 90.0f, 0.1f), 10.0f));
    p.push_back (std::make_unique<APF> ("DelayMix", "Echo Mix", juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 2.0f));
    p.push_back (std::make_unique<APF> ("DelayWidth", "Echo Stereo Width", juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f));

    return { p.begin(), p.end() };
}

float VoiceCallOfDutyAudioProcessor::getParam (const char* id) const noexcept
{
    if (auto* v = apvts.getRawParameterValue (id))
        return v->load (std::memory_order_relaxed);
    return 0.0f;
}

void VoiceCallOfDutyAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    currentSampleRate = sampleRate;
    // A sample-rate-independent 20 ms one-pole glide for the RADIO filter
    // coefficients; coefficients are advanced once per frame for stereo lock.
    radioFilterSmoothingCoefficient = 1.0f - std::exp (-1.0f / (float) (sampleRate * 0.020));
    // Remove subsonic/DC buildup introduced by asymmetric saturation and noise.
    radioDcBlockCoefficient = std::exp (-juce::MathConstants<float>::twoPi * 18.0f / (float) sampleRate);
    // Smooth audible RADIO controls over 20 ms to avoid zipper noise when
    // parameters are automated or adjusted while audio is running.
    radioDrive.reset (sampleRate, 0.020);
    radioGain.reset (sampleRate, 0.020);
    radioNoise.reset (sampleRate, 0.020);
    radioEnable.reset (sampleRate, 0.020);
    reverbMixSmooth.reset (sampleRate, 0.020);
    reverbRoomSmooth.reset (sampleRate, 0.050);
    reverbDampingSmooth.reset (sampleRate, 0.050);
    globalWetSmooth.reset (sampleRate, 0.020);
    radioEnable.setCurrentAndTargetValue (getParam ("RadioToggle") >= 0.5f ? 1.0f : 0.0f);
    reverbMixSmooth.setCurrentAndTargetValue (getParam ("ReverbToggle") >= 0.5f
        ? getParam ("ReverbMix") * 0.01f * getParam ("Amount") * 0.01f : 0.0f);
    reverbRoomSmooth.setCurrentAndTargetValue (juce::jlimit (0.05f, 0.95f, getParam ("ReverbDecayTime")));
    reverbDampingSmooth.setCurrentAndTargetValue (juce::jlimit (0.0f, 1.0f, getParam ("ReverbDamping") * 0.01f));
    globalWetSmooth.setCurrentAndTargetValue (juce::jlimit (0.0f, 1.0f, getParam ("DryWetMix") * 0.01f));
    radioDrive.setCurrentAndTargetValue (1.0f + getParam ("RadioDistortionAmount") * 0.075f * getParam ("Amount") * 0.01f);
    radioGain.setCurrentAndTargetValue (1.0f + getParam ("RadioDemodulationGain") * 0.02f);
    radioNoise.setCurrentAndTargetValue (getParam ("RadioNoise") * 0.0008f * getParam ("Amount") * 0.01f);
    const auto delayCapacity = juce::jmax (2, (int) std::ceil (sampleRate * 1.1));
    for (auto& channel : delayBuffer)
        channel.assign ((size_t) delayCapacity, 0.0f);
    delayWritePosition = 0;
    echoDampingState.fill (0.0f);
    // A gentle low-pass in the feedback path makes successive repeats darker
    // without dulling the first repeat. Keep the cutoff below Nyquist.
    const float echoDampingCutoff = juce::jmin (7200.0f, (float) sampleRate * 0.40f);
    echoDampingCoefficient = 1.0f - std::exp (-juce::MathConstants<float>::twoPi
        * echoDampingCutoff / (float) sampleRate);
    // Smooth delay controls over 30 ms. Delay time is read fractionally below,
    // so automation does not jump between integer samples.
    delayTimeSamples.reset (sampleRate, 0.030);
    delayFeedbackSmooth.reset (sampleRate, 0.030);
    delayMixSmooth.reset (sampleRate, 0.030);
    delayWidthSmooth.reset (sampleRate, 0.030);
    delayTimeSamples.setCurrentAndTargetValue (juce::jlimit (1.0f, (float) delayCapacity - 1.0f,
        getParam ("DelayTime") * (float) sampleRate));
    delayFeedbackSmooth.setCurrentAndTargetValue (getParam ("DelayFeedback") * 0.01f);
    delayMixSmooth.setCurrentAndTargetValue (getParam ("DelayMix") * 0.01f * getParam ("Amount") * 0.01f);
    delayWidthSmooth.setCurrentAndTargetValue (getParam ("DelayWidth") * 0.01f);

    dryBuffer.setSize (getTotalNumOutputChannels(), juce::jmax (samplesPerBlock, 16384), false, true, false);
    hpPreviousInput.fill (0.0f);
    hpPreviousOutput.fill (0.0f);
    lpState.fill (0.0f);
    radioDcPreviousInput.fill (0.0f);
    radioDcPreviousOutput.fill (0.0f);
    updateRadioFilters();

    juce::Reverb::Parameters rp;
    rp.roomSize = 0.35f;
    rp.damping = getParam ("ReverbDamping") * 0.01f;
    rp.wetLevel = 0.12f;
    rp.dryLevel = 1.0f;
    rp.width = 1.0f;
    rp.freezeMode = 0.0f;
    reverb.setParameters (rp);
    reverb.reset();
}

void VoiceCallOfDutyAudioProcessor::releaseResources()
{
    for (auto& channel : delayBuffer)
        std::fill (channel.begin(), channel.end(), 0.0f);
    reverb.reset();
}

bool VoiceCallOfDutyAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    return in == out && (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo());
}

void VoiceCallOfDutyAudioProcessor::updateRadioFilters()
{
    const float cutoff = juce::jlimit (600.0f, 5000.0f, getParam ("RadioCutoffFreq"));
    const float highCut = getParam ("RadioType") >= 0.5f ? 420.0f : 300.0f;
    const float safeLow = juce::jmin (highCut, (float) currentSampleRate * 0.40f);
    const float sr = (float) juce::jmax (8000.0, currentSampleRate);
    const float nyquistSafeHigh = juce::jmax (safeLow + 20.0f, sr * 0.45f);
    const float boundedHigh = juce::jlimit (safeLow + 20.0f, nyquistSafeHigh,
                                             juce::jlimit (600.0f, 5000.0f, cutoff));
    targetHpAlpha = std::exp (-juce::MathConstants<float>::twoPi * safeLow / sr);
    targetLpAlpha = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * boundedHigh / sr);
}

void VoiceCallOfDutyAudioProcessor::processRadio (juce::AudioBuffer<float>& buffer, int numSamples, float amount)
{
    updateRadioFilters();
    radioDrive.setTargetValue (1.0f + getParam ("RadioDistortionAmount") * 0.075f * amount);
    radioGain.setTargetValue (1.0f + getParam ("RadioDemodulationGain") * 0.02f);
    radioNoise.setTargetValue (getParam ("RadioNoise") * 0.0008f * amount);
    const bool tactical = getParam ("RadioType") >= 0.5f;
    // Advance shared filter coefficients once per sample frame, not once per
    // channel. This keeps left/right cutoff modulation identical in stereo.
    for (int i = 0; i < numSamples; ++i)
    {
        hpAlpha += (targetHpAlpha - hpAlpha) * radioFilterSmoothingCoefficient;
        lpAlpha += (targetLpAlpha - lpAlpha) * radioFilterSmoothingCoefficient;
        const float drive = radioDrive.getNextValue();
        const float gain = radioGain.getNextValue();
        const float noise = radioNoise.getNextValue();
        const float enabledAmount = amount * radioEnable.getNextValue();
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            const float input = data[i];
            const auto channel = (size_t) ch;
            const float hp = hpAlpha * (hpPreviousOutput[channel] + input - hpPreviousInput[channel]);
            hpPreviousInput[channel] = input;
            hpPreviousOutput[channel] = hp;
            lpState[channel] += lpAlpha * (hp - lpState[channel]);
            // Small deterministic xorshift noise generator; no heap allocation or locks in the audio thread.
            noiseState ^= noiseState << 13;
            noiseState ^= noiseState >> 17;
            noiseState ^= noiseState << 5;
            const float noiseSample = ((float) (noiseState & 0x00ffffffu) / 8388607.5f - 1.0f) * noise;
            // Tactical mode adds a restrained asymmetric transfer curve for a
            // denser radio character; Classic remains symmetric and cleaner.
            const float shaped = tactical
                ? std::tanh ((lpState[channel] + 0.035f) * drive) - std::tanh (0.035f * drive)
                : std::tanh (lpState[channel] * drive);
            const float saturated = shaped * gain + noiseSample;
            // First-order DC blocker, state kept independently per channel.
            const float radio = saturated - radioDcPreviousInput[channel]
                + radioDcBlockCoefficient * radioDcPreviousOutput[channel];
            radioDcPreviousInput[channel] = saturated;
            radioDcPreviousOutput[channel] = radio;
            data[i] = input + (radio - input) * enabledAmount;
        }
    }
}

void VoiceCallOfDutyAudioProcessor::processReverb (juce::AudioBuffer<float>& buffer, int numSamples)
{
    const float targetMix = getParam ("ReverbToggle") >= 0.5f
        ? getParam ("ReverbMix") * 0.01f * getParam ("Amount") * 0.01f : 0.0f;
    reverbMixSmooth.setTargetValue (targetMix);
    reverbRoomSmooth.setTargetValue (juce::jlimit (0.05f, 0.95f, getParam ("ReverbDecayTime")));
    reverbDampingSmooth.setTargetValue (juce::jlimit (0.0f, 1.0f, getParam ("ReverbDamping") * 0.01f));

    // JUCE Reverb updates parameters per block. Average smoothed values over
    // short chunks so mix, room character and damping move without zippering.
    // The 50 ms room/damping glide avoids abrupt changes to the reverb network.
    constexpr int rampChunk = 16;
    for (int offset = 0; offset < numSamples; offset += rampChunk)
    {
        const int count = juce::jmin (rampChunk, numSamples - offset);
        float mix = 0.0f;
        float roomSize = 0.0f;
        float damping = 0.0f;
        for (int i = 0; i < count; ++i)
        {
            mix += reverbMixSmooth.getNextValue();
            roomSize += reverbRoomSmooth.getNextValue();
            damping += reverbDampingSmooth.getNextValue();
        }
        const float inverseCount = 1.0f / (float) count;
        mix *= inverseCount;
        roomSize *= inverseCount;
        damping *= inverseCount;

        juce::Reverb::Parameters rp;
        rp.roomSize = roomSize;
        rp.damping = damping;
        rp.wetLevel = mix;
        rp.dryLevel = 1.0f - mix;
        rp.width = 1.0f;
        rp.freezeMode = 0.0f;
        reverb.setParameters (rp);
        if (buffer.getNumChannels() > 1)
            reverb.processStereo (buffer.getWritePointer (0) + offset,
                                  buffer.getWritePointer (1) + offset, count);
        else
            reverb.processMono (buffer.getWritePointer (0) + offset, count);
    }
}

void VoiceCallOfDutyAudioProcessor::processEcho (juce::AudioBuffer<float>& buffer, int numSamples)
{
    if (delayBuffer[0].empty()) return;
    const int capacity = (int) delayBuffer[0].size();
    delayTimeSamples.setTargetValue (juce::jlimit (1.0f, (float) capacity - 1.0f,
        getParam ("DelayTime") * (float) currentSampleRate));
    const bool enabled = getParam ("DelayToggle") >= 0.5f;
    delayFeedbackSmooth.setTargetValue (enabled
        ? juce::jlimit (0.0f, 0.90f, getParam ("DelayFeedback") * 0.01f) : 0.0f);
    delayMixSmooth.setTargetValue (enabled
        ? getParam ("DelayMix") * 0.01f * getParam ("Amount") * 0.01f : 0.0f);
    delayWidthSmooth.setTargetValue (getParam ("DelayWidth") * 0.01f);

    auto* left = buffer.getWritePointer (0);
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    for (int i = 0; i < numSamples; ++i)
    {
        const float delaySamples = delayTimeSamples.getNextValue();
        const float feedback = delayFeedbackSmooth.getNextValue();
        const float mix = delayMixSmooth.getNextValue();
        const float width = delayWidthSmooth.getNextValue();
        float readPosition = (float) delayWritePosition - delaySamples;
        if (readPosition < 0.0f) readPosition += (float) capacity;
        const int read0 = ((int) readPosition) % capacity;
        const int read1 = (read0 + 1) % capacity;
        const float fraction = readPosition - std::floor (readPosition);
        const auto interpolate = [read0, read1, fraction] (const std::vector<float>& data) noexcept
        {
            return data[(size_t) read0] + (data[(size_t) read1] - data[(size_t) read0]) * fraction;
        };

        const float inL = left[i];
        const float delayedL = interpolate (delayBuffer[0]);
        echoDampingState[0] += echoDampingCoefficient * (delayedL - echoDampingState[0]);
        const float feedbackL = echoDampingState[0];
        // When Echo is bypassed, do not keep recording the dry input into the
        // delay line. Otherwise enabling Echo later can replay audio captured
        // while the effect was off. The feedback tail is still allowed to decay
        // naturally as the smoothed feedback value approaches zero.
        delayBuffer[0][(size_t) delayWritePosition] = (enabled ? inL : 0.0f) + feedbackL * feedback;
        left[i] = inL + delayedL * mix;
        if (right != nullptr)
        {
            const float inR = right[i];
            const float delayedR = interpolate (delayBuffer[1]);
            echoDampingState[1] += echoDampingCoefficient * (delayedR - echoDampingState[1]);
            const float feedbackR = echoDampingState[1];
            delayBuffer[1][(size_t) delayWritePosition] = (enabled ? inR : 0.0f) + feedbackR * feedback;
            const float mid = 0.5f * (delayedL + delayedR);
            const float side = 0.5f * (delayedL - delayedR) * width;
            left[i] = inL + (mid + side) * mix;
            right[i] = inR + (mid - side) * mix;
        }
        delayWritePosition = (delayWritePosition + 1) % capacity;
    }
}

void VoiceCallOfDutyAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ignoreUnused (midi);
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int channels = buffer.getNumChannels();
    if (channels == 0) return;

    if (dryBuffer.getNumChannels() != channels || dryBuffer.getNumSamples() < numSamples)
        return; // Host block size exceeded the prepared capacity; avoid allocating in the audio callback.
    for (int ch = 0; ch < channels; ++ch)
        dryBuffer.copyFrom (ch, 0, buffer, ch, 0, numSamples);

    const float amount = getParam ("Amount") * 0.01f;
    radioEnable.setTargetValue (getParam ("RadioToggle") >= 0.5f && amount > 0.0f ? 1.0f : 0.0f);
    processRadio (buffer, numSamples, amount);
    processReverb (buffer, numSamples);
    processEcho (buffer, numSamples);

    globalWetSmooth.setTargetValue (juce::jlimit (0.0f, 1.0f, getParam ("DryWetMix") * 0.01f));
    for (int i = 0; i < numSamples; ++i)
    {
        const float wet = globalWetSmooth.getNextValue();
        for (int ch = 0; ch < channels; ++ch)
        {
            auto* out = buffer.getWritePointer (ch);
            const auto* in = dryBuffer.getReadPointer (ch);
            const float mixed = in[i] + (out[i] - in[i]) * wet;
            // Transparent below -0.45 dBFS; smoothly catches overs instead of
            // hard clipping. This protects hosts and downstream devices from
            // stacked radio gain + reverb + echo peaks without adding latency.
            const float magnitude = std::abs (mixed);
            constexpr float knee = 0.95f;
            constexpr float ceiling = 0.999f;
            if (magnitude <= knee)
                out[i] = mixed;
            else
            {
                const float limited = knee + (ceiling - knee)
                    * std::tanh ((magnitude - knee) / (ceiling - knee));
                out[i] = std::copysign (limited, mixed);
            }
        }
    }
}

double VoiceCallOfDutyAudioProcessor::getTailLengthSeconds() const
{
    double tailSeconds = 0.0;

    if (getParam ("DelayToggle") >= 0.5f && getParam ("DelayMix") > 0.0f)
    {
        const double delaySeconds = juce::jlimit (0.001, 1.0, (double) getParam ("DelayTime"));
        const double feedback = juce::jlimit (0.0, 0.90, (double) getParam ("DelayFeedback") * 0.01);
        // Estimate the time for repeats to fall below -60 dB. With zero
        // feedback only the first repeat remains.
        const double repeats = feedback > 0.0 ? std::log (0.001) / std::log (feedback) : 1.0;
        tailSeconds = delaySeconds * juce::jlimit (1.0, 64.0, repeats);
    }

    if (getParam ("ReverbToggle") >= 0.5f && getParam ("ReverbMix") > 0.0f)
        tailSeconds = juce::jmax (tailSeconds, 1.5 + 8.0 * (double) getParam ("ReverbDecayTime"));

    return juce::jlimit (0.0, 30.0, tailSeconds);
}

int VoiceCallOfDutyAudioProcessor::getCurrentProgram()
{
    return juce::roundToInt (getParam ("Preset"));
}

void VoiceCallOfDutyAudioProcessor::setCurrentProgram (int index)
{
    loadFactoryPreset (juce::jlimit (0, 5, index));
}

const juce::String VoiceCallOfDutyAudioProcessor::getProgramName (int index)
{
    return presetNames[(size_t) juce::jlimit (0, 5, index)];
}

void VoiceCallOfDutyAudioProcessor::loadFactoryPreset (int index)
{
    index = juce::jlimit (0, 5, index);
    auto set = [this] (const char* id, float value)
    {
        if (auto* param = apvts.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
    };

    // Shared values follow the supplied XML presets. Sample and Environment are intentionally omitted.
    set ("Preset", (float) index);
    set ("Amount", 100.0f);
    set ("DryWetMix", 100.0f);
    set ("RadioToggle", 1.0f);
    set ("RadioCutoffFreq", index == 0 ? 1300.0f : 1000.0f);
    set ("RadioDistortionAmount", index == 0 ? 25.0f : 12.0f);
    set ("RadioDemodulationGain", index == 0 ? 0.0f : 24.5f);
    set ("RadioType", index == 0 ? 0.0f : 1.0f);
    set ("RadioNoise", 0.0f);
    set ("ReverbToggle", 1.0f);
    set ("ReverbDecayTime", index == 0 ? 0.3f : (index == 1 ? 0.5f : 0.5f));
    set ("ReverbMix", index == 0 ? 12.0f : 20.0f);
    set ("ReverbDamping", 55.0f);
    set ("DelayToggle", (index == 3 || index == 5) ? 1.0f : 0.0f);
    set ("DelayTime", index == 0 ? 0.0f : (index == 5 ? 0.07f : 0.3f));
    set ("DelayFeedback", index == 0 ? 0.0f : (index == 5 ? 40.0f : 10.0f));
    float delayMix = index == 0 ? 0.0f : 2.0f;
    if (index == 3) delayMix = 4.0f;
    if (index == 4) delayMix = 20.0f;
    if (index == 5) delayMix = 25.0f;
    set ("DelayMix", delayMix);
    set ("DelayWidth", 100.0f);
}

void VoiceCallOfDutyAudioProcessor::saveUserPreset (int slot, const juce::String& name)
{
    slot = juce::jlimit (0, 7, slot);
    const auto slotId = "SLOT_" + juce::String (slot);
    auto snapshot = apvts.copyState();
    auto existing = userPresets.getChildWithName (slotId);
    if (existing.isValid())
        userPresets.removeChild (existing, nullptr);
    juce::ValueTree entry (slotId);
    // Keep names within the same limit enforced by the XML/state validator.
    // This prevents a name accepted by the UI from making a later host-state
    // restore reject the entire preset bank.
    auto safeName = name.trim().substring (0, 24);
    if (safeName.isEmpty())
        safeName = "USER " + juce::String (slot + 1);
    entry.setProperty ("name", safeName, nullptr);
    entry.addChild (snapshot.createCopy(), -1, nullptr);
    userPresets.addChild (entry, -1, nullptr);
}

void VoiceCallOfDutyAudioProcessor::loadUserPreset (int slot)
{
    slot = juce::jlimit (0, 7, slot);
    auto entry = userPresets.getChildWithName ("SLOT_" + juce::String (slot));
    if (! entry.isValid() || entry.getNumChildren() == 0)
        return;
    auto snapshot = entry.getChild (0).createCopy();
    apvts.replaceState (snapshot);
}

juce::String VoiceCallOfDutyAudioProcessor::getUserPresetName (int slot) const
{
    slot = juce::jlimit (0, 7, slot);
    auto entry = userPresets.getChildWithName ("SLOT_" + juce::String (slot));
    if (! entry.isValid())
        return "EMPTY " + juce::String (slot + 1);
    return entry.getProperty ("name", "USER " + juce::String (slot + 1)).toString();
}

bool VoiceCallOfDutyAudioProcessor::exportUserPresets (const juce::File& file) const
{
    if (! file.hasFileExtension (".xml") || file.isDirectory())
        return false;

    if (! isValidUserPresetTree (userPresets, apvts.state.getType()))
        return false;

    auto root = userPresets.createCopy();
    std::unique_ptr<juce::XmlElement> xml (root.createXml());
    return xml != nullptr && file.replaceWithText (xml->toString());
}

bool VoiceCallOfDutyAudioProcessor::importUserPresets (const juce::File& file)
{
    if (! file.existsAsFile() || ! file.hasFileExtension (".xml") || file.getSize() <= 0 || file.getSize() > 256 * 1024)
        return false;

    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr || ! xml->hasTagName ("USER_PRESETS"))
        return false;

    auto imported = juce::ValueTree::fromXml (*xml);
    if (! imported.isValid() || imported.getType() != juce::Identifier ("USER_PRESETS") || imported.getNumChildren() > 8)
        return false;

    if (! isValidUserPresetTree (imported, apvts.state.getType()))
        return false;

    userPresets = imported.createCopy();
    return true;
}

void VoiceCallOfDutyAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.addChild (userPresets.createCopy(), -1, nullptr);
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    if (xml != nullptr)
        copyXmlToBinary (*xml, destData);
    else
        destData.reset();
}

void VoiceCallOfDutyAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Host state is untrusted input too: reject oversized/corrupt snapshots
    // before replacing APVTS, and validate user slots independently.
    constexpr int maxStateBytes = 1024 * 1024;
    if (data == nullptr || sizeInBytes <= 0 || sizeInBytes > maxStateBytes)
        return;

    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return;

    auto restored = juce::ValueTree::fromXml (*xml);
    if (! restored.isValid() || restored.getType() != apvts.state.getType())
        return;

    // USER_PRESETS is serialized as an extra child beside the 18 APVTS
    // parameter nodes. Validate that bank separately, then remove it from a
    // copy before validating the strict APVTS snapshot. Validating the whole
    // tree directly would count USER_PRESETS as a 19th parameter child and
    // reject every normal host state restore.
    auto savedPresets = restored.getChildWithName ("USER_PRESETS");
    if (savedPresets.isValid() && ! isValidUserPresetTree (savedPresets, apvts.state.getType()))
        return;

    auto parameterState = restored.createCopy();
    if (savedPresets.isValid())
        parameterState.removeChild (savedPresets, nullptr);

    if (! isValidParameterSnapshot (parameterState, apvts.state.getType()))
        return;

    userPresets = savedPresets.isValid() ? savedPresets.createCopy() : juce::ValueTree ("USER_PRESETS");
    apvts.replaceState (parameterState);
}

juce::AudioProcessorEditor* VoiceCallOfDutyAudioProcessor::createEditor()
{
    return new VoiceCallOfDutyAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VoiceCallOfDutyAudioProcessor();
}
