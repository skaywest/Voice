# Factory presets

Values transcribed from the supplied XML presets. Parameters belonging to Sample and Environment are intentionally excluded.

| Preset | Radio cutoff (Hz) | Distortion (%) | Demod gain | Radio type | Reverb mix (%) | Reverb decay | Echo enabled | Echo time (s) | Echo feedback (%) | Echo mix (%) |
|---|---:|---:|---:|---:|---:|---:|---|---:|---:|---:|
| Field Ops | 1300 | 25 | 0 | 0 | 12 | 0.3 | No | 0.30 | 10 | 0 |
| Safe House | 1000 | 12 | 24.5 | 1 | 20 | 0.5 | No | 0.30 | 10 | 2 |
| Casino Heist | 1000 | 12 | 24.5 | 1 | 20 | 0.5 | No | 0.30 | 10 | 2 |
| Sabotage | 1000 | 12 | 24.5 | 1 | 20 | 0.5 | Yes | 0.30 | 10 | 4 |
| Gala | 1000 | 12 | 24.5 | 1 | 20 | 0.5 | No | 0.30 | 10 | 20 |
| Redacted | 1000 | 12 | 24.5 | 1 | 20 | 0.5 | Yes | 0.07 | 40 | 25 |

All six presets enable Radio and Reverb and use 100% global Dry/Wet in the source XML. `Amount` is a new global control for this independent implementation and defaults to 100%; it is not an XML parameter.

## DSP scope and limitations

- Radio is a practical band-limited saturation/voicing effect. It is not a bit-exact reproduction of the proprietary radio DSP.
- `RadioPitchShift` is not implemented in this version; it is deliberately not exposed as a nonfunctional control.
- Reverb decay values are mapped approximately to JUCE Reverb's `roomSize` parameter, not interpreted as literal seconds.
- Echo is a feedback delay with a preallocated stereo ring buffer.


## Пользовательские пресеты (v0.6)

В интерфейсе доступны 8 слотов. Выберите слот, введите имя, нажмите SAVE, чтобы записать текущие настройки. Нажмите LOAD, чтобы восстановить сохранённую конфигурацию. Слоты сохраняются внутри состояния плагина хостом.


## Added v0.9 controls

All factory presets start with Radio Noise 0%, Reverb Damping 55%, and Echo Stereo Width 100%. These are implementation defaults, not values recovered from the source XML.
