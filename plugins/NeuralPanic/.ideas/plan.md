# NeuralPanic Implementation Plan

## Complexity Score: 5

## UI Framework
- Decision: `webview`
- Rationale: NeuralPanic needs dense control groupings, animated fault telemetry, and an artifact monitor that is faster to iterate in HTML/CSS/JS while DSP remains in C++.
- Strategy: `phased`

## Phased Implementation

### Phase 1 - Core Infrastructure
- [ ] Create plugin skeleton with APVTS parameters from `parameter-spec.md`.
- [ ] Add smoothing, seed routing, and deterministic RNG plumbing.
- [ ] Implement output safety limiter and bypass path.

### Phase 2 - Artifact Engines A
- [ ] Implement neural codec stress stage.
- [ ] Implement packet-loss and PLC stage.
- [ ] Validate against clicks, zippering, and unstable gain.

### Phase 3 - Artifact Engines B
- [ ] Implement vocoder instability stage.
- [ ] Implement compression meltdown stage.
- [ ] Add panic macro routing and per-mode weighting tables.

### Phase 4 - Temporal + Integration
- [ ] Implement temporal smear stage.
- [ ] Integrate full chain ordering and dry/wet/output stage.
- [ ] Tune CPU budget and aliasing behavior.

### Phase 5 - UI and Validation
- [ ] Build WebView UI from `Design/v1-test.html` structure.
- [ ] Bind all controls and state sync (host -> UI -> host).
- [ ] Run pluginval + manual DAW tests.

## Required JUCE Modules
- `juce_audio_basics`
- `juce_audio_processors`
- `juce_audio_plugin_client`
- `juce_dsp`
- `juce_gui_extra` (WebView bridge)
- `juce_events`

## Performance Targets
- Stable at 48 kHz / 128 samples on stereo material.
- No heap allocations in `processBlock`.
- No denormal spikes in high-smear states.
- Safe output ceiling with worst-case parameter sets.

## Risk Assessment

### High Risk
- Coupled nonlinear stages causing level bursts or unstable resonance.
- Packet-loss and jitter simulation producing discontinuities.
- Parameter automation across seeded random systems creating nondeterministic recalls.

### Medium Risk
- Pitch/formant tracker behavior on noisy non-voice input.
- CPU spikes when multiple artifact modules align in high-intensity states.
- Maintaining intelligibility at moderate settings while preserving extreme modes.

### Low Risk
- APVTS serialization.
- Basic WebView control wiring.
- Dry/wet and output gain handling.

## Validation Matrix
1. Speech male/female clean recordings.
2. Noisy location dialogue.
3. Non-speech source (synth, drums) for edge cases.
4. Offline bounce and live playback parity.
5. Preset recall under seeded and unseeded randomness.
