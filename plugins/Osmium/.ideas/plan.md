# Implementation Plan: Osmium

## Complexity Score: 2 (Moderate)

## Implementation Strategy

### Single-Pass Implementation
Since the components are standard and well-understood, we will implement the core DSP and UI in a continuous workflow.

1.  **Project Setup:** Initialize data structures and boilerplate.
2.  **DSP Core:**
    - Implement `TransientShaper` class.
    - Implement `TubeSaturator` class.
    - Implement `Limiter` (or use JUCE `dsp::Limiter`).
    - Wire `Intensity` parameter to control all three.
3.  **UI Implementation:**
    - Setup WebView2 bridge.
    - Create 'Glowing Core' knob component in HTML/CSS/JS.
    - Bind knob to `Intensity`.

## Dependencies
**Required JUCE Modules:**
- `juce_audio_basics`
- `juce_audio_processors`
- `juce_dsp` (For Limiter and basic math)
- `juce_gui_extra` (For WebView2)

## Risk Assessment
**Medium Risk:**
- **Parameter Smoothing:** Controlling 3 diverse processors with one knob might cause "zipper noise" if not smoothed correctly at the parameter level.
- **Gain Staging:** Boosting transients + Drive into a limiter can easily crush dynamics too much. Careful tuning of the internal ranges is critical.

**Low Risk:**
- **DSP Algorithms:** Standard implementations available.
