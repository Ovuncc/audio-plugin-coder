# Morphant Knob Technical Purpose

## DSP Topology
- Modulator input: plugin main input bus (`Sound Source`), typically voice/formant source.
- Carrier input: sidechain bus (`Texture Source`), fallback to main input if sidechain is unavailable.
- Core engine: selectable `Filterbank` vocoder or `Spectral` cross-synthesis engine.
- Band tiers by `Reality`: 96 (stable), 128 (hybrid), 160 (experimental).
- Modulator analysis path includes pre-emphasis for consonant/formant clarity.
- Unvoiced enhancement path injects high-passed noise when fricative content is detected.
- Voiced reinforcement path adds low-level harmonic excitation when pitch confidence is high.
- Pitch follower uses confidence-gated smoothing to reduce jitter on noisy modulators.

## Control Mapping

### `mode` (`Filterbank` / `Spectral`)
- Purpose: choose morph engine topology.
- `Filterbank`: classic channel-vocoder style envelope transfer.
- `Spectral`: FFT-domain magnitude transfer while retaining carrier phase.

### `merge` (0.0-1.0)
- Purpose: envelope imprint depth.
- Implementation: blends each band between flat carrier passthrough and modulator envelope gain.
- Formula: `bandGain = (1 - merge) + merge * envNorm`.

### `glue` (0.0-1.0)
- Purpose: temporal cohesion of envelope transfer.
- Implementation: sets envelope follower attack/release coefficients for all bands.
- Mapping:
  - Attack: ~1.5 ms -> 16 ms.
  - Release: ~35 ms -> 240 ms.
  - Also raises an envelope floor to avoid over-gated output at low levels.

### `pitch_follower` (0.0-1.0)
- Purpose: pitch-coupled retuning of synthesis bands.
- Implementation: blockwise autocorrelation pitch estimate on modulator mono signal, then smooth pitch-scale warping of band centers.
- Mapping: `pitchScale = (pitchHz / 120 Hz)^(pitch_follower * 0.75)` clamped to a safe range.

### `formant_follower` (0.0-1.0)
- Purpose: spectral-envelope tracking emphasis.
- Implementation:
  - Dynamic formant warp in filter center distribution (`sin`-based mild warping).
  - Brightness-derived tilt weighting across bands (low/high rebalance).
- Effect: higher values produce stronger vowel-like/formant movement.

### `focus` (0.0-1.0)
- Purpose: spectral selectivity.
- Implementation:
  - Increases effective resonance/Q of bandpass filters.
  - Adds mid-band emphasis and increases detector scaling.
  - Increases modulator pre-emphasis depth (more high-band articulation).
- Effect: low = broader/smoother imprint, high = sharper intelligibility and detail.

### `reality` (0.0-1.0)
- Purpose: complexity/mode control.
- Implementation:
  - `0.00-0.19`: stable mode, 96 bands.
  - `0.20-0.54`: hybrid mode, 128 bands.
  - `0.55-1.00`: experimental mode, 160 bands plus nonlinear wet warp (`tanh`) and resonance micro-variation.
  - In spectral mode, controls effective transfer intensity/complexity profile.

### `sibilance` (0.0-1.0)
- Purpose: unvoiced consonant enhancement amount.
- Implementation: scales high-passed noise injection driven by unvoiced detector (fricative emphasis).

### `mix` (0.0-1.0)
- Purpose: dry/wet balance.
- Implementation: smoothed linear blend of dry carrier path and vocoded wet path (automation-safe).
- Formula: `out = (1 - mix) * dry + mix * wet`.

### `output_gain` (-18 dB to +18 dB)
- Purpose: final level trim.
- Implementation: smoothed linear post-gain after dry/wet mix.

## Notes
- The current implementation is optimized for realtime safety (no per-sample allocations).
- Filterbank topology is ready for further enhancement (per-band damping curves, consonant/unvoiced path, transient sidechain logic).
