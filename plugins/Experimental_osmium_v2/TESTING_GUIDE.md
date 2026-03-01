# Osmium Plugin - Testing Guide

**Version:** v0.0.0  
**Last Updated:** 2026-02-14  
**Status:** Ready for Testing

---

## 📋 Pre-Testing Checklist

- [x] Plugin builds without errors
- [x] VST3 installed to system folder
- [x] All source files present
- [x] WebView UI assets embedded
- [x] Parameter bindings configured
- [ ] Tested in DAW
- [ ] UI verified
- [ ] Audio processing verified

---

## 🧪 Test Plan Overview

### Phase 1: Basic Functionality (Critical)
1. Plugin loads in DAW
2. UI appears and is responsive
3. Parameters control audio
4. No crashes or hangs

### Phase 2: UI Integration (High Priority)
1. WebView loads correctly
2. JUCE backend connects
3. Parameter relays work
4. Visual feedback accurate

### Phase 3: Audio Processing (High Priority)
1. DSP chain functions correctly
2. Transient shaping works
3. Saturation adds warmth
4. Limiter prevents clipping

### Phase 4: Edge Cases (Medium Priority)
1. Extreme parameter values
2. Multiple instances
3. Automation
4. Save/load presets

---

## 🎯 Test 1: Plugin Loading

### Objective
Verify plugin loads in DAW without errors.

### Steps
1. Open your DAW (Reaper, FL Studio, Ableton, etc.)
2. Create a new project
3. Add an audio track
4. Search for "Osmium" in plugin list
5. Load Osmium on the track

### Expected Results
- ✅ Plugin appears in plugin list
- ✅ Plugin window opens
- ✅ No error messages
- ✅ DAW remains responsive

### Failure Indicators
- ❌ Plugin not in list → Check installation path
- ❌ Crash on load → Check debug log
- ❌ Black screen → WebView issue
- ❌ DAW freezes → Threading issue

### Debug Steps
1. Check `Documents\Osmium_Debug.log` for errors
2. Try standalone version first
3. Verify WebView2 installed (Windows)
4. Check DAW console for messages

---

## 🎨 Test 2: WebView UI Loading

### Objective
Verify WebView loads and displays UI correctly.

### Steps
1. Load plugin in DAW
2. Observe plugin window
3. Check for UI elements:
   - Header with "OSMIUM" title
   - Large central knob
   - Glow ring around knob
   - Output gain slider
   - Bypass toggle
   - Debug console (bottom)

### Expected Results
- ✅ All UI elements visible
- ✅ Futuristic dark theme applied
- ✅ Knob has cyan glow at 0%
- ✅ Debug console shows initialization messages

### Debug Console Messages
Look for these in the debug console:

```
=== OSMIUM DEBUG SESSION ===
DOM Content Loaded
✓ window.__JUCE__ exists
✓ initialisationData exists
InitData Keys (3): __juce__sliders, __juce__toggles, __juce__comboBoxes
  ✓ Found slider: intensity
  ✓ Found slider: output_gain
  ✓ Found toggle: bypass
✓ getSliderState(intensity) succeeded
  Initial value: 0
```

### Failure Indicators
- ❌ Blank window → Resource provider issue
- ❌ "window.__JUCE__ is UNDEFINED" → Backend not initialized
- ❌ "MISSING slider: intensity" → Parameter relay issue
- ❌ HTML/CSS not loading → Binary data embedding issue

### Debug Steps
1. Check debug console for specific error
2. Verify BinaryData files compiled
3. Check resource provider in PluginEditor.cpp
4. Verify parameter relay initialization order

---

## 🎛️ Test 3: Parameter Interaction

### Objective
Verify UI controls respond and update parameters.

### Test 3A: Intensity Knob
1. Click and drag on central knob upward
2. Observe:
   - Knob rotates
   - Glow ring changes color (cyan → orange)
   - Value display updates
   - Debug console shows value changes

### Expected Results
- ✅ Knob rotates smoothly
- ✅ Color gradient from cyan (0%) to orange (100%)
- ✅ Value display shows percentage
- ✅ Parameter updates in DAW automation lane

### Test 3B: Output Gain Slider
1. Click and drag output gain slider
2. Observe:
   - Slider moves
   - Value display updates
   - Audio level changes

### Expected Results
- ✅ Slider responds to mouse
- ✅ Range: -12dB to +12dB
- ✅ Default: 0dB
- ✅ Audio level changes accordingly

### Test 3C: Bypass Toggle
1. Click bypass button
2. Observe:
   - Button state changes
   - Audio bypasses processing
   - UI indicates bypass state

### Expected Results
- ✅ Toggle responds to click
- ✅ Audio passes through unprocessed
- ✅ Visual feedback (button color/state)

### Failure Indicators
- ❌ Knob doesn't rotate → Mouse event issue
- ❌ No color change → CSS/canvas rendering issue
- ❌ Parameter doesn't update → Relay attachment issue
- ❌ DAW automation doesn't show changes → APVTS issue

---

## 🔊 Test 4: Audio Processing

### Objective
Verify DSP chain processes audio correctly.

### Test Setup
1. Load plugin on audio track
2. Import test audio (drum loop, bass, or full mix)
3. Set intensity to 0%
4. Play audio and listen

### Test 4A: Transient Enhancement
1. Set intensity to 50%
2. Play drum loop
3. Listen for:
   - Sharper attack on kicks/snares
   - More "punch" and presence
   - Transients cutting through

### Expected Results
- ✅ Transients become more pronounced
- ✅ Attack phase enhanced
- ✅ No artifacts or distortion
- ✅ Natural-sounding enhancement

### Test 4B: Saturation/Warmth
1. Set intensity to 70%
2. Play bass or synth
3. Listen for:
   - Added warmth and richness
   - Harmonic content
   - Perceived loudness increase

### Expected Results
- ✅ Warmer, fuller tone
- ✅ Even-order harmonics present
- ✅ Increased perceived loudness
- ✅ No harsh distortion

### Test 4C: Limiting
1. Set intensity to 100%
2. Play loud audio
3. Check for:
   - No clipping/overs
   - Peaks caught by limiter
   - Transparent limiting

### Expected Results
- ✅ No clipping on output
- ✅ Peaks controlled
- ✅ Transparent limiting (no pumping)
- ✅ Maintains dynamics

### Test 4D: Output Gain
1. Set intensity to 50%
2. Adjust output gain from -12dB to +12dB
3. Verify:
   - Volume changes accordingly
   - No clipping at +12dB
   - Clean gain adjustment

### Expected Results
- ✅ Output level changes smoothly
- ✅ No artifacts during adjustment
- ✅ Proper gain staging

### Failure Indicators
- ❌ No audio change → DSP not processing
- ❌ Harsh distortion → Saturation too aggressive
- ❌ Clipping → Limiter not working
- ❌ Pumping/artifacts → Compressor settings wrong

---

## 🔄 Test 5: Bidirectional Parameter Sync

### Objective
Verify parameters sync between UI and DAW.

### Test 5A: UI → DAW
1. Adjust intensity knob in plugin UI
2. Check DAW automation lane
3. Verify value matches

### Expected Results
- ✅ DAW automation shows parameter change
- ✅ Value matches UI display
- ✅ Updates in real-time

### Test 5B: DAW → UI
1. Draw automation in DAW for intensity
2. Play project
3. Watch plugin UI

### Expected Results
- ✅ Knob rotates to match automation
- ✅ Color changes with value
- ✅ Smooth animation
- ✅ No lag or stuttering

### Test 5C: Preset Save/Load
1. Set intensity to 75%
2. Set output gain to -3dB
3. Save preset in DAW
4. Reset parameters to default
5. Load saved preset

### Expected Results
- ✅ Parameters restore to saved values
- ✅ UI updates to match
- ✅ Audio processing matches

---

## ⚡ Test 6: Performance

### Objective
Verify plugin performs efficiently.

### Test 6A: CPU Usage
1. Load plugin on track
2. Check DAW CPU meter
3. Set intensity to various levels
4. Monitor CPU usage

### Expected Results
- ✅ Low CPU usage at idle
- ✅ Reasonable CPU at 100% intensity
- ✅ No CPU spikes
- ✅ Stable performance

### Test 6B: Multiple Instances
1. Load 10 instances of Osmium
2. Play audio through all
3. Monitor CPU and stability

### Expected Results
- ✅ All instances work correctly
- ✅ No crashes
- ✅ Acceptable CPU usage
- ✅ No audio dropouts

### Test 6C: Buffer Size Stress Test
1. Set DAW buffer to minimum (64 samples)
2. Play audio with plugin active
3. Check for dropouts

### Expected Results
- ✅ No audio dropouts
- ✅ Stable playback
- ✅ No glitches

---

## 🐛 Test 7: Edge Cases

### Test 7A: Extreme Parameter Values
1. Set intensity to 0% → Verify bypass-like behavior
2. Set intensity to 100% → Verify no distortion
3. Set output to -12dB → Verify quiet but clean
4. Set output to +12dB → Verify loud but no clipping

### Test 7B: Rapid Parameter Changes
1. Quickly move intensity knob up and down
2. Rapidly toggle bypass on/off
3. Check for:
   - Smooth parameter changes
   - No clicks or pops
   - No crashes

### Test 7C: Sample Rate Changes
1. Test at 44.1kHz
2. Test at 48kHz
3. Test at 96kHz
4. Verify consistent behavior

### Test 7D: Channel Configurations
1. Test mono track
2. Test stereo track
3. Verify proper processing

---

## 📊 Test Results Template

### Test Session Information
- **Date:** ___________
- **Tester:** ___________
- **OS:** ___________
- **DAW:** ___________ (Version: _______)
- **Plugin Version:** v0.0.0
- **Build Date:** 2026-02-14

### Test Results Summary

| Test | Status | Notes |
|------|--------|-------|
| 1. Plugin Loading | ⬜ Pass / ⬜ Fail | |
| 2. WebView UI | ⬜ Pass / ⬜ Fail | |
| 3. Parameter Interaction | ⬜ Pass / ⬜ Fail | |
| 4. Audio Processing | ⬜ Pass / ⬜ Fail | |
| 5. Bidirectional Sync | ⬜ Pass / ⬜ Fail | |
| 6. Performance | ⬜ Pass / ⬜ Fail | |
| 7. Edge Cases | ⬜ Pass / ⬜ Fail | |

### Issues Found

| Issue # | Severity | Description | Steps to Reproduce |
|---------|----------|-------------|-------------------|
| 1 | | | |
| 2 | | | |
| 3 | | | |

**Severity Levels:**
- **Critical:** Crash, data loss, unusable
- **High:** Major functionality broken
- **Medium:** Minor functionality issue
- **Low:** Cosmetic or minor inconvenience

---

## 🔍 Debug Resources

### Log Files
- **Windows:** `C:\Users\[Username]\Documents\Osmium_Debug.log`
- **macOS:** `~/Documents/Osmium_Debug.log`
- **Linux:** `~/Documents/Osmium_Debug.log`

### Debug Console
- Located at bottom of plugin UI
- Shows real-time JUCE backend status
- Logs parameter changes and errors

### DAW Console
- Check DAW's console/log window
- May show JUCE or plugin errors
- Useful for crash diagnostics

---

## ✅ Success Criteria

### Minimum Viable Product (MVP)
- [x] Plugin loads without crashing
- [ ] UI displays correctly
- [ ] All parameters functional
- [ ] Audio processing works
- [ ] No critical bugs

### Ready for Release
- [ ] All tests pass
- [ ] No high-severity bugs
- [ ] Performance acceptable
- [ ] Documentation complete
- [ ] User feedback positive

---

## 🚀 Next Steps After Testing

### If All Tests Pass
1. Update `status.json`: Set `tests_passed: true`
2. Update `IMPLEMENTATION_STATUS.md`
3. Proceed to packaging phase: `/ship Osmium`

### If Tests Fail
1. Document issues in detail
2. Prioritize by severity
3. Fix critical issues first
4. Re-test after fixes
5. Repeat until all tests pass

---

## 📞 Reporting Issues

When reporting issues, include:
1. **Test number and name**
2. **Expected behavior**
3. **Actual behavior**
4. **Steps to reproduce**
5. **Debug log excerpt**
6. **System information**
7. **Screenshots/recordings if applicable**

---

**Happy Testing! 🎵**
