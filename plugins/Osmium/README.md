# Osmium - The Ultimate "Make It Punch" Button

![Status](https://img.shields.io/badge/Status-Testing-yellow)
![Build](https://img.shields.io/badge/Build-Passing-green)
![UI](https://img.shields.io/badge/UI-WebView2-blue)

**Osmium** is a single-knob audio plugin that instantly adds weight, impact, and high-density saturation to any sound source. It combines transient shaping, tube saturation, and limiting into one seamless macro control.

---

## 🎯 Features

### Single Macro Control
- **Osmium Core:** One massive, glowing knob that controls everything
- **0% → 100%:** From clean to maximum punch
- **Visual Feedback:** Color gradient from cyan to orange

### DSP Processing
- **Transient Shaper:** Boosts attack phase for cutting through the mix
- **Tube Saturation:** Even-order harmonics for warmth and perceived loudness
- **Brickwall Limiter:** Transparent peak catching to prevent clipping
- **Auto-Makeup Gain:** Intelligent level compensation

### Additional Controls
- **Output Gain:** -12dB to +12dB post-processing compensation
- **Bypass:** Complete signal bypass for A/B comparison

---

## 🎨 User Interface

### Futuristic Design
- **Theme:** Dark brushed metal with neon accents
- **Central Knob:** Large rotatable control with glow ring
- **Color Coding:** 
  - Cyan (0%) = Clean
  - Orange (100%) = Maximum punch
- **Minimal Layout:** Focus on the core control

### Interaction
- **Mouse Drag:** Rotate knob up/down
- **Scroll Wheel:** Fine adjustment
- **Double-Click:** Reset to default
- **Right-Click:** Enter precise value

---

## 🔧 Technical Specifications

### Parameters

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| **Osmium Core** | 0.0 - 1.0 | 0.0 | Main macro control |
| **Output Gain** | -12dB - +12dB | 0dB | Post-processing volume |
| **Bypass** | On/Off | Off | Signal bypass |

### Internal Mappings

As **Osmium Core** increases from 0% to 100%:

| Component | Parameter | Range |
|-----------|-----------|-------|
| Transient Shaper | Compression Threshold | 0dB → -24dB |
| Tube Drive | Input Gain | 0dB → +12dB |
| Limiter | Threshold | -0.1dB → -1.0dB |
| Auto-Makeup | Gain Compensation | 0dB → -6dB |

### DSP Chain

```
Input → Drive → Saturation → Transient Shaping → Limiter → Output
```

1. **Input Drive:** Increases signal level into saturation
2. **Tube Saturation:** Hyperbolic tangent waveshaping
3. **Transient Shaping:** Compressor with slow attack (30ms)
4. **Brickwall Limiter:** Peak limiting with fast release (100ms)
5. **Output Gain:** Auto-makeup + manual compensation

---

## 🎵 Use Cases

### Electronic Music Production
- **Drums:** Make kicks and snares punch through dense mixes
- **Bass:** Add weight and sub-harmonic content
- **Synths:** Increase presence and impact

### Sound Design
- **Instant Banger:** Quick "make it loud" processing
- **Layering:** Add punch to layered sounds
- **Transient Enhancement:** Sharpen attacks

### Mixing
- **Bus Processing:** Add glue to drum buses
- **Master Chain:** Final impact enhancement
- **Parallel Processing:** Blend with dry signal for control

---

## 💻 System Requirements

### Supported Platforms
- **Windows:** 10/11 (64-bit)
- **macOS:** 10.13+ (Intel/Apple Silicon)
- **Linux:** Ubuntu 20.04+ (or equivalent)

### Plugin Formats
- **VST3:** All platforms
- **Standalone:** All platforms

### DAW Compatibility
- Reaper
- FL Studio
- Ableton Live
- Studio One
- Cubase/Nuendo
- Pro Tools
- Logic Pro (macOS)
- And more...

---

## 📦 Installation

### Windows
1. Download `Osmium_v1.0_Windows.zip`
2. Extract the archive
3. Copy `Osmium.vst3` to:
   - `C:\Program Files\Common Files\VST3\`
4. Rescan plugins in your DAW

### macOS
1. Download `Osmium_v1.0_macOS.zip`
2. Extract the archive
3. Copy `Osmium.vst3` to:
   - `/Library/Audio/Plug-Ins/VST3/`
4. Rescan plugins in your DAW

### Linux
1. Download `Osmium_v1.0_Linux.tar.gz`
2. Extract the archive
3. Copy `Osmium.vst3` to:
   - `~/.vst3/`
4. Rescan plugins in your DAW

---

## 🎓 Quick Start Guide

### Basic Usage
1. **Load Plugin:** Insert Osmium on your track
2. **Play Audio:** Start playback
3. **Turn Up Intensity:** Rotate the Osmium Core knob
4. **Listen:** Hear the punch increase
5. **Adjust Output:** Compensate for loudness if needed

### Tips & Tricks
- **Start Low:** Begin at 20-30% and increase gradually
- **A/B Compare:** Use bypass to compare processed vs. dry
- **Parallel Processing:** Blend with dry signal for more control
- **Automation:** Automate intensity for dynamic impact
- **Stacking:** Use multiple instances on different elements

### Sweet Spots
- **Drums:** 40-60% for natural punch
- **Bass:** 30-50% for warmth and weight
- **Full Mix:** 20-40% for glue and impact
- **Sound Design:** 60-100% for aggressive processing

---

## 🐛 Troubleshooting

### Plugin Not Appearing in DAW
- Verify VST3 is in correct folder
- Rescan plugins in DAW preferences
- Check DAW supports VST3 format
- Restart DAW after installation

### UI Not Loading
- Ensure WebView2 is installed (Windows)
- Update graphics drivers
- Try standalone version first
- Check DAW console for errors

### Audio Artifacts
- Reduce intensity if distortion occurs
- Check input levels aren't too hot
- Adjust output gain for headroom
- Verify sample rate matches project

### Performance Issues
- Increase buffer size in DAW
- Disable other CPU-intensive plugins
- Freeze/bounce tracks when possible
- Update to latest plugin version

---

## 📚 Documentation

- **Implementation Status:** [`IMPLEMENTATION_STATUS.md`](IMPLEMENTATION_STATUS.md)
- **Creative Brief:** [`.ideas/creative-brief.md`](.ideas/creative-brief.md)
- **Parameter Spec:** [`.ideas/parameter-spec.md`](.ideas/parameter-spec.md)
- **Architecture:** [`.ideas/architecture.md`](.ideas/architecture.md)
- **UI Spec:** [`Design/v1-ui-spec.md`](Design/v1-ui-spec.md)

---

## 🔄 Version History

### v0.0.0 (Current - Testing Phase)
- Initial implementation
- Core DSP chain complete
- WebView UI implemented
- Build system configured
- Awaiting testing and validation

---

## 🤝 Support

### Reporting Issues
If you encounter any issues:
1. Check troubleshooting section above
2. Verify system requirements
3. Update to latest version
4. Report issue with:
   - OS and version
   - DAW and version
   - Plugin version
   - Steps to reproduce
   - Error messages/logs

### Debug Logging
Debug logs are written to:
- **Windows:** `Documents\Osmium_Debug.log`
- **macOS:** `~/Documents/Osmium_Debug.log`
- **Linux:** `~/Documents/Osmium_Debug.log`

---

## 📄 License

Copyright © 2026. All rights reserved.

---

## 🎯 Target Audience

- **Electronic Music Producers:** Needing quick impact
- **Sound Designers:** Looking for "instant banger" processing
- **Mix Engineers:** Wanting secret sauce for drums and bass
- **Bedroom Producers:** Seeking professional punch
- **Live Performers:** Needing reliable impact enhancement

---

## 🚀 Roadmap

### Future Enhancements (Post v1.0)
- [ ] Preset system
- [ ] Additional saturation algorithms
- [ ] Stereo width control
- [ ] Sidechain input
- [ ] MIDI learn
- [ ] Undo/redo
- [ ] Resizable UI
- [ ] Additional color themes

---

**Made with JUCE 8 | WebView2 UI | Windows 11**
