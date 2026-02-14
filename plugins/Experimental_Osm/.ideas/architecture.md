# DSP Architecture Specification: Osmium

## Core Components
- **Input Gain Stage:** Pre-processing level adjustment.
- **Transient Shaper:** Envelope follower based detection to boost attack transients.
- **Tube Saturation:** Wave-shaping algorithm (e.g., asymmetric hyperbolic tangent) with even-order harmonic generation.
- **Brickwall Limiter:** Peak lookahead limiter to catch saturation overshoots.
- **Output Gain Stage:** Post-processing level for makeup gain.

## Processing Chain
```
Input → Input Gain → Split Signal
                     ├→ Transient Shaper (Sidechain) → Gain Mod → Tube Saturation → Limiter → Output Gain → Output
```
*(Note: Transient shaping often modulates the gain before or after saturation. Here, boosting transients into saturation creates more "punch" and grit.)*

## Parameter Mapping

| Parameter | Component | Function | Range |
|-----------|-----------|----------|-------|
| **Intensity (Macro)** | **Transient Shaper** | Attack Boost Amount | 0% to 100% (0dB to +6dB) |
|                     | **Tube Drive** | Input Drive | 0dB to +12dB |
|                     | **Limiter** | Threshold | -0.1dB to -3.0dB |
| Output Gain         | Output Gain | Level | +/- 12dB |

## Complexity Assessment
**Level 2 (Moderate)**
- **Rationale:**
    - Multiple DSP blocks (Transient, Drive, Limiter) but standard algorithms.
    - Single Macro control simplifies the *user* interface but introduces complexity in *parameter mapping* (one knob controlling multiple internal values).
    - No complex spectral processing or FFT.

**Score: 2**
