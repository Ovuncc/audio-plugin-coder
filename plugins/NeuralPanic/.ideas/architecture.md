# NeuralPanic DSP Architecture

## Core Components
1. Input conditioning and speech-centric pre-emphasis.
2. Neural codec stress emulator.
3. Packet-loss and PLC engine.
4. Vocoder instability stage (pitch/formant identity drift).
5. Compression meltdown stage.
6. Temporal smear stage.
7. Mix/output/safety stage.

## Processing Chain

```
Input
  -> Speech Pre-Conditioning
  -> Neural Codec Stress
  -> Packet Loss + PLC
  -> Vocoder Instability
  -> Compression Meltdown
  -> Temporal Smear
  -> Mix + Output Gain + Safety Limiter
  -> Output
```

## Module Notes

### Neural Codec Stress
- Downsample and bandwidth constrain per block.
- Frame segmentation (20-40 ms) with pseudo-codebook quantization.
- Probabilistic frame corruption tied to `collapse`.
- Short anti-alias and reconstruction smoothing to avoid pure bitcrusher tone.

### Packet Loss + PLC
- Burst-loss model with per-frame state.
- PLC modes:
  - Hold: repeat last valid frame.
  - Interp: cross-fade predicted frame.
  - Noise: shaped noise substitution.
- Jitter simulation by variable read pointer with bounded latency.

### Vocoder Instability
- Pitch tracking with confidence.
- Confidence-weighted jitter and octave mistakes.
- Formant envelope warping with wrong voiced/unvoiced switching events.

### Compression Meltdown
- Fast compressor with modulated threshold.
- Soft clipper stage.
- Optional 2-5 kHz emphasis to emulate comms panic grit.

### Temporal Smear
- Micro-grain circular buffer (about 30-120 ms grains).
- Random overlap, occasional reverse bursts.
- Freeze bursts with fast decay release.

## Parameter Mapping

| Parameter | Component | Function | Range |
|-----------|-----------|----------|-------|
| `mode` | All | Changes event rates and module weighting | 3-state choice |
| `panic` | Global Macro | Raises instability and modulation depth | 0.0-1.0 |
| `bandwidth` | Codec Stress | Controls downsample/quality target | 0.0-1.0 |
| `collapse` | Codec Stress | Controls codebook corruption probability | 0.0-1.0 |
| `packet_loss` | Packet Engine | Frame drop and burst-loss severity | 0.0-1.0 |
| `jitter` | Packet Engine | Latency modulation window | 0.0-1.0 |
| `plc_mode` | Packet Engine | Concealment strategy | 3-state choice |
| `pitch_drift` | Vocoder Instability | Pitch tracker perturbation depth | 0.0-1.0 |
| `formant_melt` | Vocoder Instability | Envelope/formant misalignment | 0.0-1.0 |
| `broadcast` | Meltdown | Compression and clipping aggression | 0.0-1.0 |
| `smear` | Temporal Smear | Grain overlap/reverse/freeze intensity | 0.0-1.0 |
| `seed` | Stochastic Engine | Repeatable random sequence selection | 0-9999 |
| `mix` | Output | Dry/wet blend | 0.0-1.0 |
| `output_gain` | Output | Final trim | -18 to +18 dB |
| `bypass` | Output | Hard bypass | off/on |

## Complexity Assessment
**Score: 5/5**

### Rationale
1. Multi-domain DSP: combines time-domain frames, pitch/formant analysis, and spectral-style smearing.
2. Stochastic state machines: burst-loss, PLC behavior, and seeded chaos must remain musical and deterministic when requested.
3. Strong module coupling: macro control (`panic`) drives several nonlinear subsystems at once without becoming unusable.
4. Real-time constraints: all behavior must run allocation-free in `processBlock` across host buffer sizes.
5. QA/test burden: extreme parameter combinations can fail audibly (clicks, runaway gain, denormals), so hard safety and profiling are required.
