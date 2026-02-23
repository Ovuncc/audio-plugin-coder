# NeuralPanic UI Specification v1

## Design Summary
NeuralPanic v1 uses an "Emergency Console" visual language: a dark Morphant-style shell, cyan/orange accents, and live artifact telemetry.  
The interface should feel like a voice system under diagnostic failure, not a standard music plugin skin.

## Window and Layout
- Desktop window: `980 x 620 px`
- Mobile preview behavior: single-column stacking below `900 px` width
- Framework target: WebView (HTML/CSS/JS)
- Top-level structure:
  1. Header bar (brand + mode + bypass + panic alarm)
  2. Main split:
     - Left: live monitor panel (`artifact-scope` canvas + fault log)
     - Right: control wall with module cards
  3. Footer status strip (severity, mode, jitter readout)

## Layout Grid
- Global shell padding: `16 px`
- Main split: `minmax(340px, 1.1fr)` and `minmax(420px, 1fr)`
- Control wall grid: `2 columns`, card gap `12 px`
- Control cards min height: `116 px`

## Control Representation

| Parameter | UI Type | Element ID | Position |
|-----------|---------|------------|----------|
| `mode` | Segmented select | `mode` | Header left |
| `panic` | Macro slider | `panic` | Header center |
| `bypass` | Toggle button | `bypass` | Header right |
| `codec_bypass` | Module toggle | `codec_bypass` | Card: Codec header |
| `bandwidth` | Vertical slider | `bandwidth` | Card: Codec |
| `collapse` | Vertical slider | `collapse` | Card: Codec |
| `packet_bypass` | Module toggle | `packet_bypass` | Card: Timing header |
| `jitter` | Vertical slider | `jitter` | Card: Timing |
| `identity_bypass` | Module toggle | `identity_bypass` | Card: Identity header |
| `pitch_drift` | Vertical slider | `pitch_drift` | Card: Identity |
| `formant_melt` | Vertical slider | `formant_melt` | Card: Identity |
| `dynamics_bypass` | Module toggle | `dynamics_bypass` | Card: Dynamics header |
| `broadcast` | Vertical slider | `broadcast` | Card: Dynamics |
| `temporal_bypass` | Module toggle | `temporal_bypass` | Card: Temporal header |
| `smear` | Vertical slider | `smear` | Card: Temporal |
| `seed` | Step slider | `seed` | Card: Temporal |
| `mix` | Horizontal slider | `mix` | Card: Output |
| `output_gain` | Horizontal slider | `output_gain` | Card: Output |

## Monitor Panel Requirements
- Canvas `#artifact-scope` must animate at about 60 fps.
- Overlay scanline and dropout indicators should respond to `panic` and `collapse`.
- Fault log (`#fault-log`) should rotate brief event messages and use severity colors.
- A compact meter strip (`#band-meter`) should show pseudo mel-band instability bars.

## Color Palette
- `--bg-0`: `#121214` (deep background)
- `--bg-1`: `#1c1c1f` (panel base)
- `--bg-2`: `#141418` (secondary surface)
- `--ink`: `#ffffff` (primary text)
- `--muted`: `#666666` (secondary text)
- `--alarm`: `#ff3300` (critical)
- `--corrode`: `#00f3ff` (active controls)
- `--caution`: `#ff7a33` (warning)
- `--signal`: `#00f3ff` (telemetry)
- `--line`: `#2a2a2f` (panel boundaries)

## Typography
- Primary: `"Bahnschrift SemiCondensed", "Arial Narrow", "Segoe UI", sans-serif`
- Numeric readouts: `"Consolas", "Courier New", monospace`
- Header title: `26 px`, 700
- Labels: `11 px`, uppercase, 0.12em tracking
- Values: `12 px` monospace

## Motion and Interaction
1. Page load stagger: cards reveal in sequence (40 ms step).
2. Scope animation: continuous jitter influenced by `panic`, `collapse`, `smear`.
3. Alarm pulse: when panic exceeds threshold, header alarm badge pulses.
4. Slider interaction: smooth value updates with no abrupt readout jumps.
5. Module bypass toggles: each module card has an `Active/Bypassed` button to disable only that engine stage.

## Accessibility
- Minimum text contrast target: WCAG AA.
- Visible focus ring for all inputs and buttons.
- Do not rely on color alone for alarm state; include text state label.

## Responsive Rules
- At widths below `900 px`, switch to stacked panels.
- At widths below `560 px`, collapse control wall to one column.
- Preserve full functionality in mobile preview (no hidden controls).

## Implementation Notes
- Keep IDs exactly as in `parameter-spec.md`.
- `v1-test.html` serves as the structural template for `Source/ui/public/index.html`.
- During `/impl`, replace preview-only random telemetry with JUCE relay data.
