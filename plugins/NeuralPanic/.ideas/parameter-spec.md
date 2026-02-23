# NeuralPanic Parameter Specification

| ID | Name | Type | Range | Default | Unit |
|:---|:-----|:-----|:------|:--------|:-----|
| `mode` | Engine Mode | Choice | `Agent`, `Dropout`, `Collapse` | `Agent` | enum |
| `panic` | Panic | Float | 0.0 - 1.0 | 0.45 | % |
| `bandwidth` | Bandwidth | Float | 0.0 - 1.0 | 0.62 | mapped kbps |
| `collapse` | Latent Collapse | Float | 0.0 - 1.0 | 0.30 | % |
| `jitter` | Jitter | Float | 0.0 - 1.0 | 0.25 | mapped ms |
| `pitch_drift` | Pitch Drift | Float | 0.0 - 1.0 | 0.35 | % |
| `formant_melt` | Formant Melt | Float | 0.0 - 1.0 | 0.40 | % |
| `broadcast` | Broadcast Panic | Float | 0.0 - 1.0 | 0.25 | % |
| `smear` | Temporal Smear | Float | 0.0 - 1.0 | 0.35 | % |
| `seed` | Chaos Seed | Int | 0 - 9999 | 1337 | int |
| `mix` | Mix | Float | 0.0 - 1.0 | 1.00 | % |
| `output_gain` | Output | Float | -18.0 - 18.0 | 0.0 | dB |
| `bypass` | Bypass | Bool | off/on | off | bool |

## Derived Internal Mappings
- `bandwidth`: maps to target transport quality tiers (about 6-48 kbps equivalent).
- `jitter`: maps to variable latency swing (about 0-40 ms).
- `panic`: macro bias into `collapse`, `jitter`, `pitch_drift`, and limiter aggression.
- `mode`: changes random event rates and module weighting presets.
