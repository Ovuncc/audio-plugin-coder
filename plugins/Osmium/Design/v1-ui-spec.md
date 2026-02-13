# UI Specification v1: Osmium

## Layout
- **Window:** 500px (width) x 400px (height)
- **Theme:** Futuristic Industrial Minimal
- **Grid:**
    - **Header (Top):** Plugin Name ("OSMIUM"), Bypass Toggle.
    - **Center (Hero):** "The Core" - Massive central rotary control.
    - **Footer (Bottom):** Output Gain slider (horizontal) or small knob.

## Controls
| Parameter | ID | Type | Position | Range | Valid Values |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Osmium Core** | `intensity` | Large Rotary Knob | Center (200px size) | 0.0 - 1.0 | 0% - 100% |
| **Output** | `output_gain` | Horizontal Slider | Bottom Center | -12.0 - +12.0 | dB |
| **Bypass** | `bypass` | Toggle Button | Top Right | 0 - 1 | On/Off |

## Color Palette
- **Background:** `#1a1a1a` (Dark brushed metal texture equivalent)
- **Primary (Glow/Active):** `#00ffff` (Cyan) shifting to `#ff4400` (Orange) at high intensity.
- **Secondary (Inactive/UI):** `#444444` (Dark Grey)
- **Text:** `#e0e0e0` (Off-white)

## Style Notes
- "The Core" should glow. The glow intensity/radius increases with the knob value.
- Minimal text. Modern, sans-serif font (e.g., Rajdhani or Orbitron if available, otherwise Roboto).
- No separate meters; the knob's glow acts as the visual feedback for "power".
