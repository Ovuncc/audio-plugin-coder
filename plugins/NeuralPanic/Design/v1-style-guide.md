# NeuralPanic Style Guide v1

## Visual Direction
- Theme: emergency diagnostics station in a dark Morphant-style chassis with cyan/orange fault accents.
- Emotional tone: procedural panic, unstable system identity.
- Avoided style: bright industrial paper surfaces and flat monochrome panels.

## Design Tokens

```css
:root {
  --bg-0: #121214;
  --bg-1: #1c1c1f;
  --bg-2: #141418;
  --surface: #1c1c1f;
  --surface-2: #141418;
  --panel: #1c1c1f;
  --ink: #ffffff;
  --muted: #666666;
  --alarm: #ff3300;
  --corrode: #00f3ff;
  --signal: #00f3ff;
  --caution: #ff7a33;
  --line: #2a2a2f;
  --focus: #00f3ff;

  --radius-sm: 6px;
  --radius-md: 10px;
  --radius-lg: 14px;

  --shadow-soft: 0 8px 16px rgba(0, 0, 0, 0.36);
  --shadow-panel: 0 1px 0 rgba(255, 255, 255, 0.04), 0 10px 18px rgba(0, 0, 0, 0.34);
}
```

## Typography

```css
--font-ui: "Bahnschrift SemiCondensed", "Arial Narrow", "Segoe UI", sans-serif;
--font-mono: "Consolas", "Courier New", monospace;

--size-title: 26px;
--size-section: 13px;
--size-label: 11px;
--size-value: 12px;
--size-body: 14px;

--weight-heavy: 700;
--weight-medium: 600;
--weight-normal: 400;
```

## Surface Rules
1. Use layered surfaces, not flat blocks.
2. Apply subtle glow and scanline overlays for monitor sections.
3. Keep borders at `--line` to preserve Morphant's modular panel language.
4. Reserve `--alarm` for high-severity states only; default emphasis should stay cyan.

## Control Rules
- Sliders:
  - Track: `--line`
  - Fill: `--corrode` (`#00f3ff`) by default
  - Fill in high-risk modules may shift toward `--alarm` as value rises
- Select menus:
  - Background `--surface-2`, border `--line`
  - Hover border `--signal`
- Toggle button (`bypass`):
  - OFF: neutral dark panel
  - ON: `--alarm` fill with bright text
- Module toggles (`codec_bypass`, `packet_bypass`, `identity_bypass`, `dynamics_bypass`, `temporal_bypass`):
  - `packet_bypass` now gates the timing/jitter stage (no packet-loss or PLC controls in v1).
  - OFF: label `Active`, neutral dark panel
  - ON: label `Bypassed`, caution tint (`--caution`) and border emphasis

## Motion Rules
1. `page-load`: card opacity/translate stagger over 280 ms.
2. `alarm-pulse`: 1.2 s infinite pulse only when panic threshold exceeded.
3. `scanline`: slow vertical drift animation for monitor panel.
4. Avoid excessive micro-animations on all controls at once.

## Spacing Rules
- Outer shell padding: `16px`
- Section padding: `12px`
- Card internal padding: `10px`
- Gaps:
  - Large region gap: `12px`
  - Small inline gap: `8px`

## Component States
- `normal`: idle, low contrast surface.
- `active`: control touched or automation write, stronger border and shadow.
- `warning`: value beyond nominal operating zone.
- `critical`: alarm color + pulse badge.

## Iconography and Labeling
- Use uppercase short labels (`PACKET LOSS`, `FORMANT MELT`).
- Keep value text monospace for fast scanning.
- Pair severity labels with numeric readings.

## Responsive Guidelines
- Maintain two-column control wall down to `560 px`.
- Switch to one column below `560 px`.
- Ensure canvas and cards remain readable on narrow widths.

## Implementation Bridge Notes
- In `/impl`, map each control ID directly to JUCE relays.
- Keep monitor telemetry update loop independent from control rendering.
- Seed display should echo current deterministic random seed from DSP state.
