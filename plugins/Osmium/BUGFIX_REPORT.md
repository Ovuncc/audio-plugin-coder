# Osmium Plugin - Bug Fix Report

**Date:** 2026-02-14  
**Version:** v0.0.1 (Post-Initial Testing)  
**Status:** Fixes Applied, Rebuilding

---

## 🐛 Issues Reported

### Issue #1: NaN% Displayed in Main Knob
**Severity:** Critical  
**Description:** The intensity knob displayed "NaN%" instead of a numeric value.

**Root Cause:**  
The JavaScript code was calling `getNormalisedValue()` before the JUCE backend had initialized the parameter properties. The calculation in `getNormalisedValue()` uses `this.properties.start`, `this.properties.end`, and `this.properties.skew`, which were undefined during initial load, resulting in NaN.

**Fix Applied:**
- Added NaN checks in all parameter value retrievals
- Added 100ms initialization delay using `setTimeout()` to allow JUCE backend to fully initialize
- Added fallback values (0 for intensity, 0 for output gain, false for bypass)
- Added optimistic UI updates during user interaction

**Files Modified:**
- [`Source/ui/public/js/index.js`](plugins/Osmium/Source/ui/public/js/index.js) - Lines 110-173

---

### Issue #2: Knob Does Not React to Input
**Severity:** Critical  
**Description:** The intensity knob did not respond to mouse drag interactions.

**Root Cause:**  
The mouse move handler was getting NaN values from `getNormalisedValue()`, which caused the calculation `current + (delta * 0.005)` to result in NaN, preventing parameter updates.

**Fix Applied:**
- Added NaN check in mousemove handler: `current = isNaN(val) ? 0 : val`
- Ensures calculation always uses valid numbers
- Maintains smooth interaction even during initialization

**Files Modified:**
- [`Source/ui/public/js/index.js`](plugins/Osmium/Source/ui/public/js/index.js) - Lines 227-248

---

### Issue #3: Gain Slider Not Affecting Audio
**Severity:** High  
**Description:** The output gain slider was adjustable but did not affect audio output. The value display remained at "0" regardless of slider position.

**Root Cause:**  
Similar to Issue #1, the output gain parameter was not initialized when the UI tried to read its value, resulting in NaN. Additionally, the DSP smoothing was only advancing by one sample per block instead of the full block size.

**Fix Applied:**
- Added NaN checks and initialization delay for output gain slider
- Fixed DSP smoothing to use `skip(buffer.getNumSamples())` to advance smoothing to current target
- Changed from `getNextValue()` to `getCurrentValue()` after skip

**Files Modified:**
- [`Source/ui/public/js/index.js`](plugins/Osmium/Source/ui/public/js/index.js) - Lines 127-156
- [`Source/PluginProcessor.cpp`](plugins/Osmium/Source/PluginProcessor.cpp) - Lines 185-195

---

### Issue #4: Bypass Button Not Working
**Severity:** High  
**Description:** The bypass button did not respond to clicks and did not toggle bypass state.

**Root Cause:**  
The bypass toggle state was not initialized properly, and there was no optimistic UI update on click, making it appear unresponsive.

**Fix Applied:**
- Added initialization delay for bypass state
- Added optimistic UI update on button click
- Added fallback initialization if JUCE backend not available

**Files Modified:**
- [`Source/ui/public/js/index.js`](plugins/Osmium/Source/ui/public/js/index.js) - Lines 158-173

---

### Issue #5: No Light Visible in Screen
**Severity:** Medium  
**Description:** The glow ring and visual effects were not visible.

**Root Cause:**  
The `updateKnobVis()` function was being called with NaN values, which caused CSS calculations to fail. The glow ring's `conic-gradient` and `box-shadow` were not rendering properly.

**Fix Applied:**
- Fixed NaN issues in parameter initialization (see Issue #1)
- Ensured `updateKnobVis()` is called with valid 0-1 range values
- Added fallback to 0 if value is still NaN

**Files Modified:**
- [`Source/ui/public/js/index.js`](plugins/Osmium/Source/ui/public/js/index.js) - Lines 177-194

---

### Issue #6: Audio Not Affected When Plugin Loaded
**Severity:** Critical  
**Description:** The plugin did not process audio at all, even when intensity was increased.

**Root Cause:**  
The DSP smoothing was only calling `getNextValue()` once per block, which only advanced the smoothed value by one sample. For a 512-sample block, this meant the parameter would take 512 blocks to reach the target value, making it appear as if nothing was happening.

**Fix Applied:**
- Changed smoothing to use `skip(buffer.getNumSamples())` to advance all samples in the block
- Changed from `getNextValue()` to `getCurrentValue()` after skip
- This ensures parameters respond immediately to changes

**Files Modified:**
- [`Source/PluginProcessor.cpp`](plugins/Osmium/Source/PluginProcessor.cpp) - Lines 185-195

**Code Change:**
```cpp
// BEFORE (BROKEN):
float currentIntensity = smoothedIntensity.getNextValue();
float currentOutput = smoothedOutput.getNextValue();

// AFTER (FIXED):
smoothedIntensity.skip(buffer.getNumSamples());
smoothedOutput.skip(buffer.getNumSamples());
float currentIntensity = smoothedIntensity.getCurrentValue();
float currentOutput = smoothedOutput.getCurrentValue();
```

---

## 📊 Summary of Changes

### JavaScript Changes (index.js)
1. **Added NaN Protection:** All parameter value retrievals now check for NaN
2. **Added Initialization Delays:** 100ms setTimeout for all parameter initializations
3. **Added Fallback Values:** Default to 0/false if parameters not initialized
4. **Added Optimistic Updates:** UI updates immediately on user interaction

### C++ Changes (PluginProcessor.cpp)
1. **Fixed Smoothing:** Changed from single-sample to full-block smoothing
2. **Improved Responsiveness:** Parameters now respond immediately to changes

---

## ✅ Expected Behavior After Fixes

### Intensity Knob
- ✅ Displays "0%" on load (not "NaN%")
- ✅ Responds to mouse drag up/down
- ✅ Rotates smoothly
- ✅ Glow ring changes color from cyan to orange
- ✅ Audio processing increases with intensity

### Output Gain Slider
- ✅ Displays "0.0dB" on load
- ✅ Responds to slider drag
- ✅ Range: -12dB to +12dB
- ✅ Audio volume changes accordingly

### Bypass Button
- ✅ Displays "Active" when not bypassed
- ✅ Displays "Bypassed" when bypassed
- ✅ Toggles on click
- ✅ Audio passes through unprocessed when bypassed
- ✅ Screen dims to 50% opacity when bypassed

### Visual Effects
- ✅ Glow ring visible around knob
- ✅ Color gradient: Cyan (0%) → Orange (100%)
- ✅ Glow intensity increases with value
- ✅ Knob indicator rotates with value

### Audio Processing
- ✅ Transient shaping increases with intensity
- ✅ Saturation adds warmth
- ✅ Limiter prevents clipping
- ✅ Output gain adjusts volume
- ✅ Bypass stops all processing

---

## 🧪 Testing Checklist

After rebuild, verify:

- [ ] Plugin loads without errors
- [ ] Intensity knob shows "0%" (not "NaN%")
- [ ] Knob responds to mouse drag
- [ ] Glow ring is visible and changes color
- [ ] Output gain slider works
- [ ] Bypass button toggles
- [ ] Audio is processed when intensity > 0
- [ ] Audio bypasses when bypass is ON
- [ ] No crashes or hangs

---

## 📝 Technical Notes

### Initialization Timing
The 100ms delay was chosen as a balance between:
- **Too short:** JUCE backend may not be fully initialized
- **Too long:** User sees uninitialized UI for too long

If issues persist, the delay can be increased to 200ms or 500ms.

### Smoothing Strategy
The `skip()` method advances the smoothed value by the full block size, which provides:
- **Immediate response:** Parameters change within one block (typically 512 samples = 11ms at 44.1kHz)
- **No zipper noise:** Smoothing still applied, just faster
- **Better UX:** User sees immediate feedback

Alternative approaches considered:
- **Per-sample smoothing:** More CPU intensive, unnecessary for this plugin
- **No smoothing:** Would cause zipper noise on parameter changes

### WebView Initialization
The JUCE WebView backend initializes asynchronously. The parameter relays send an initial update request, but this takes time. The setTimeout delays allow this initialization to complete before the UI tries to read values.

---

## 🔄 Build Information

**Build Command:**
```powershell
.\scripts\build-and-install.ps1 -PluginName Osmium
```

**Build Status:** In Progress  
**Expected Output:** `C:\Program Files\Common Files\VST3\Osmium.vst3`

---

## 📚 Related Documentation

- **Implementation Status:** [`IMPLEMENTATION_STATUS.md`](IMPLEMENTATION_STATUS.md)
- **Testing Guide:** [`TESTING_GUIDE.md`](TESTING_GUIDE.md)
- **User Manual:** [`README.md`](README.md)

---

**Status:** Fixes applied, awaiting rebuild and retest.
