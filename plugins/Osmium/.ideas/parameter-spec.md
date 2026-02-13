# Parameter Spec: Osmium

| ID | Name | Type | Range | Default | Unit | Description |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `intensity` | Osmium Core | Float | 0.0 - 1.0 | 0.0 | % | Main Macro. Controls Attack, Drive, and Ceiling simultaneously. |
| `output_gain` | Output | Float | -12.0 - +12.0 | 0.0 | dB | Post-processing volume compensation. |
| `bypass` | Bypass | Bool | 0 - 1 | 0 | - | Complete signal bypass. |

## Internal Mappings (Hidden from User)
As `intensity` goes 0.0 -> 1.0:
- **Transient Attack:** 0% -> +100% (Logarithmic curve)
- **Tube Drive:** 0% -> 60% (Saturation amount)
- **Limiter Threshold:** -0.1dB -> -3.0dB (Auto-makeup gain active)
