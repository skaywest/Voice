#!/usr/bin/env python3
"""Fast source-integrity checks for Voice_Call_OF_DUTY (does not compile JUCE)."""
from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parents[1]
processor = (root / 'Source' / 'PluginProcessor.cpp').read_text(encoding='utf-8')
header = (root / 'Source' / 'PluginProcessor.h').read_text(encoding='utf-8')
editor = (root / 'Source' / 'PluginEditor.cpp').read_text(encoding='utf-8')
cmake = (root / 'CMakeLists.txt').read_text(encoding='utf-8')

errors = []
def require(condition, message):
    if not condition:
        errors.append(message)

# Required effect parameters and controls.
required = [
    'Preset', 'Amount', 'DryWetMix', 'RadioToggle', 'RadioCutoffFreq',
    'RadioDistortionAmount', 'RadioDemodulationGain', 'RadioType', 'RadioNoise',
    'ReverbToggle', 'ReverbMix', 'ReverbDecayTime', 'ReverbDamping',
    'DelayToggle', 'DelayTime', 'DelayFeedback', 'DelayMix', 'DelayWidth',
]
for parameter in required:
    require(f'"{parameter}"' in processor, f'missing parameter/reference: {parameter}')

# Sample and Environment engines are deliberately out of scope.
for forbidden in ('SelectedSample', 'SampleGain', 'SampleSignalDrift', 'SampleToggle', 'EnvironmentToggle'):
    require(forbidden not in processor and forbidden not in header and forbidden not in editor,
            f'out-of-scope parameter/reference found: {forbidden}')

for effect_method in ('processRadio', 'processReverb', 'processEcho'):
    require(f'::{effect_method} (' in processor, f'missing DSP method: {effect_method}')

for preset in ('Field Ops', 'Safe House', 'Casino Heist', 'Sabotage', 'Gala', 'Redacted'):
    require(preset in processor, f'missing factory preset: {preset}')

require('FORMATS VST3' in cmake, 'CMake is not configured for VST3')
require('exportUserPresets' in processor and 'importUserPresets' in processor,
        'user preset file import/export methods missing')
require('exportUserPresetsButton' in editor and 'importUserPresetsButton' in editor,
        'user preset import/export UI controls missing')
require('XmlDocument::parse' in processor and '256 * 1024' in processor,
        'preset XML validation / size guard missing')
require('expectedParameterIds' in processor and 'std::isfinite (value)' in processor,
        'preset XML parameter snapshot validation missing')
require('maxStateBytes = 1024 * 1024' in processor and 'isValidParameterSnapshot (parameterState' in processor,
        'host state size/snapshot validation missing')
require('isValidUserPresetTree (savedPresets' in processor,
        'embedded user preset bank validation missing')
require('name.trim().substring (0, 24)' in processor and 'safeName.isEmpty()' in processor,
        'saved user preset names are not normalized to the validator limit')
require('isValidUserPresetTree (userPresets' in processor,
        'XML export does not validate the in-memory user preset bank')
require('if (xml != nullptr)\n        copyXmlToBinary' in processor,
        'host-state XML serialization lacks null-result guard')
require('auto parameterState = restored.createCopy();' in processor
        and processor.index('parameterState.removeChild (savedPresets, nullptr);') < processor.index('isValidParameterSnapshot (parameterState'),
        'host-state USER_PRESETS child is not detached before strict APVTS snapshot validation')
require('juce::juce_audio_utils' in cmake and 'juce::juce_dsp' in cmake,
        'required JUCE modules are not linked')
require('delayTimeSamples' in header and 'delayFeedbackSmooth' in header and 'std::floor (readPosition)' in processor,
        'smoothed fractional delay automation missing')
require('radioDcBlockCoefficient' in processor and 'radioDcPreviousOutput' in header, 'RADIO DC blocker missing')
require('globalWetSmooth' in processor and 'globalWetSmooth' in header, 'global Dry/Wet smoothing missing')
require('(enabled ? inL : 0.0f)' in processor and '(enabled ? inR : 0.0f)' in processor, 'Echo bypass still records dry input into delay buffer')
require('asymmetric transfer curve' in processor, 'Tactical RADIO saturation profile missing')
require('FIFINE' not in processor + header + editor, 'FIFINE integration found')
require('Stream Deck' not in processor + header + editor, 'Stream Deck integration found')

if errors:
    print('SOURCE CHECK FAILED')
    for error in errors:
        print(f' - {error}')
    sys.exit(1)

print('SOURCE CHECK PASSED')
print(f' - {len(required)} required parameter IDs present')
print(' - all 3 DSP paths present')
print(' - all 6 factory presets present')
print(' - user preset XML import/export and validation present')
print(' - Sample/Environment and button-device integrations absent')
print(' - VST3 and required JUCE modules configured')
print('NOTE: static source check only; this does not replace a Windows compile or host test.')
