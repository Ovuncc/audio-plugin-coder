import * as Juce from "./juce/index.js";

const getSliderState = Juce.getSliderState;
const getToggleState = Juce.getToggleState;
const getComboBoxState = Juce.getComboBoxState;

const PARAM_INTENSITY = "intensity";
const PARAM_OUTPUT = "output_gain";
const PARAM_BYPASS = "bypass";
const PARAM_OVERSAMPLING = "oversampling_mode";
const PARAM_MODE = "processing_mode";
const PARAM_TIGHT_LOOKAHEAD = "tight_lookahead_mode";

const DEFAULT_INTENSITY = 0.15;
const DEFAULT_OUTPUT_DB = 0.0;

const OUTPUT_MIN_DB = -12.0;
const OUTPUT_MAX_DB = 12.0;

function clamp(value, min, max) {
    return Math.max(min, Math.min(max, value));
}

function toneForValue(value) {
    const t = clamp(value, 0, 1);
    const hue = 196 + ((8 - 196) * t);
    const sat = 62 + 20 * t;
    const light = 62 - 18 * t;
    return `hsl(${hue.toFixed(1)}, ${sat.toFixed(1)}%, ${light.toFixed(1)}%)`;
}

function outputDbToNormalised(dbValue) {
    return clamp((dbValue - OUTPUT_MIN_DB) / (OUTPUT_MAX_DB - OUTPUT_MIN_DB), 0, 1);
}

function setScaledValueToState(state, scaledValue, min, max) {
    if (!state) return;
    const clamped = clamp(scaledValue, min, max);
    const normalised = clamp((clamped - min) / Math.max(1e-9, max - min), 0, 1);
    state.setNormalisedValue(normalised);
}

document.addEventListener("DOMContentLoaded", () => {
    const juceReady = typeof window.__JUCE__ !== "undefined";

    const appRoot = document.getElementById("app-root");
    const screenPanel = document.getElementById("screen-panel");
    const screenGlow = document.getElementById("screen-glow");
    const hoverName = document.getElementById("mode-hover-name");

    const knob = document.getElementById("intensity-knob");
    const knobRing = document.getElementById("knob-ring");
    const indicator = document.getElementById("knob-indicator");
    const valueDisplay = document.getElementById("intensity-value");

    const outputSlider = document.getElementById("output_gain");
    const outputValue = document.getElementById("output-value-display");

    const bypassBtn = document.getElementById("bypass");
    const oversamplingSelect = document.getElementById("oversampling_mode");
    const tightLookaheadSelect = document.getElementById("tight_lookahead_mode");

    const modeLedRow = document.getElementById("mode-led-row");
    const modeLeds = Array.from(modeLedRow.querySelectorAll(".mode-led"));

    const intensityState = juceReady ? getSliderState(PARAM_INTENSITY) : null;
    const outputState = juceReady ? getSliderState(PARAM_OUTPUT) : null;
    const bypassState = juceReady ? getToggleState(PARAM_BYPASS) : null;
    const oversamplingState = juceReady ? getComboBoxState(PARAM_OVERSAMPLING) : null;
    const modeState = juceReady ? getComboBoxState(PARAM_MODE) : null;
    const tightLookaheadState = juceReady ? getComboBoxState(PARAM_TIGHT_LOOKAHEAD) : null;

    let isDraggingKnob = false;
    let lastY = 0;

    function renderKnob(normalised) {
        const value = clamp(normalised, 0, 1);
        const pct = Math.round(value * 100);
        const degrees = value * 360;
        const color = toneForValue(value);
        const glowStrength = 10 + value * 36;

        valueDisplay.textContent = `${pct}%`;
        knobRing.style.background = `conic-gradient(${color} ${degrees}deg, transparent ${degrees}deg)`;
        knobRing.style.boxShadow = `0 0 ${glowStrength}px ${color}`;
        indicator.style.transform = `translateX(-50%) rotate(${degrees}deg)`;
        indicator.style.background = color;
        indicator.style.boxShadow = `0 0 10px ${color}`;

        knob.style.boxShadow = `0 8px 20px rgba(0,0,0,0.4), inset 0 0 ${8 + value * 22}px rgba(255,255,255,${0.06 + value * 0.14})`;
        screenGlow.style.opacity = `${0.08 + value * 0.34}`;
        screenGlow.style.background = `radial-gradient(circle at 50% 42%, ${color}55 0%, transparent 62%)`;
    }

    function renderOutput(db) {
        outputValue.textContent = `${db.toFixed(1)}dB`;
    }

    function renderBypass(isBypassed) {
        if (isBypassed) {
            bypassBtn.textContent = "Bypassed";
            bypassBtn.classList.remove("active");
            appRoot.style.opacity = "0.58";
        } else {
            bypassBtn.textContent = "Active";
            bypassBtn.classList.add("active");
            appRoot.style.opacity = "1";
        }
    }

    function renderMode(modeIdx) {
        const idx = clamp(modeIdx, 0, 2);
        modeLeds.forEach((led, i) => {
            led.classList.toggle("active", i === idx);
            if (i === idx) {
                led.classList.remove("preview");
            }
        });
    }

    function syncOversamplingFromState() {
        if (!oversamplingState) return;
        const idx = clamp(oversamplingState.getChoiceIndex(), 0, oversamplingSelect.options.length - 1);
        oversamplingSelect.selectedIndex = idx;
    }

    function syncModeFromState() {
        if (!modeState) return;
        const idx = clamp(modeState.getChoiceIndex(), 0, 2);
        renderMode(idx);
    }

    function syncTightLookaheadFromState() {
        if (!tightLookaheadState || !tightLookaheadSelect) return;
        const idx = clamp(tightLookaheadState.getChoiceIndex(), 0, tightLookaheadSelect.options.length - 1);
        tightLookaheadSelect.selectedIndex = idx;
    }

    function setPreviewMode(ledEl) {
        const idx = Number.parseInt(ledEl.dataset.mode || "0", 10);
        const activeIdx = modeState ? clamp(modeState.getChoiceIndex(), 0, 2) : 0;
        if (idx === activeIdx) {
            clearPreviewMode();
            return;
        }

        modeLeds.forEach((led) => led.classList.remove("preview"));
        ledEl.classList.add("preview");
        hoverName.textContent = ledEl.textContent || "";
        hoverName.classList.remove("mode-0", "mode-1", "mode-2");
        hoverName.classList.add(`mode-${idx}`);
        hoverName.classList.add("visible");
        screenPanel.classList.add("preview-blur");
    }

    function clearPreviewMode() {
        modeLeds.forEach((led) => led.classList.remove("preview"));
        hoverName.classList.remove("visible");
        hoverName.classList.remove("mode-0", "mode-1", "mode-2");
        hoverName.textContent = "";
        screenPanel.classList.remove("preview-blur");
    }

    function resetCoreToDefault() {
        if (intensityState) {
            intensityState.sliderDragStarted();
            intensityState.setNormalisedValue(DEFAULT_INTENSITY);
            intensityState.sliderDragEnded();
        }
        renderKnob(DEFAULT_INTENSITY);
    }

    function resetOutputToDefault() {
        if (outputState) {
            outputState.sliderDragStarted();
            outputState.setNormalisedValue(outputDbToNormalised(DEFAULT_OUTPUT_DB));
            outputState.sliderDragEnded();
        }
        outputSlider.value = `${DEFAULT_OUTPUT_DB}`;
        renderOutput(DEFAULT_OUTPUT_DB);
    }

    if (intensityState) {
        intensityState.valueChangedEvent.addListener(() => {
            const val = intensityState.getNormalisedValue();
            if (!Number.isNaN(val)) {
                renderKnob(val);
            }
        });

        setTimeout(() => {
            const val = intensityState.getNormalisedValue();
            renderKnob(Number.isNaN(val) ? DEFAULT_INTENSITY : val);
        }, 80);
    } else {
        renderKnob(DEFAULT_INTENSITY);
    }

    if (outputState) {
        outputState.valueChangedEvent.addListener(() => {
            const val = outputState.getScaledValue();
            if (!Number.isNaN(val)) {
                outputSlider.value = `${val}`;
                renderOutput(val);
            }
        });

        setTimeout(() => {
            const val = outputState.getScaledValue();
            const initial = Number.isNaN(val) ? DEFAULT_OUTPUT_DB : val;
            outputSlider.value = `${initial}`;
            renderOutput(initial);
        }, 80);
    } else {
        renderOutput(DEFAULT_OUTPUT_DB);
    }

    if (bypassState) {
        bypassState.valueChangedEvent.addListener(() => renderBypass(bypassState.getValue()));
        setTimeout(() => renderBypass(bypassState.getValue()), 80);
    } else {
        renderBypass(false);
    }

    if (oversamplingState) {
        oversamplingState.valueChangedEvent.addListener(syncOversamplingFromState);
        if (oversamplingState.propertiesChangedEvent) {
            oversamplingState.propertiesChangedEvent.addListener(syncOversamplingFromState);
        }
        setTimeout(syncOversamplingFromState, 80);
    }

    if (modeState) {
        modeState.valueChangedEvent.addListener(syncModeFromState);
        if (modeState.propertiesChangedEvent) {
            modeState.propertiesChangedEvent.addListener(syncModeFromState);
        }
        setTimeout(syncModeFromState, 80);
    } else {
        renderMode(0);
    }

    if (tightLookaheadState) {
        tightLookaheadState.valueChangedEvent.addListener(syncTightLookaheadFromState);
        if (tightLookaheadState.propertiesChangedEvent) {
            tightLookaheadState.propertiesChangedEvent.addListener(syncTightLookaheadFromState);
        }
        setTimeout(syncTightLookaheadFromState, 80);
    }

    knob.addEventListener("mousedown", (e) => {
        if (e.ctrlKey) {
            e.preventDefault();
            resetCoreToDefault();
            return;
        }

        isDraggingKnob = true;
        lastY = e.clientY;
        document.body.style.cursor = "ns-resize";
        if (intensityState) {
            intensityState.sliderDragStarted();
        }
    });

    window.addEventListener("mousemove", (e) => {
        if (!isDraggingKnob) return;

        const delta = lastY - e.clientY;
        lastY = e.clientY;

        let current = DEFAULT_INTENSITY;
        if (intensityState) {
            const v = intensityState.getNormalisedValue();
            current = Number.isNaN(v) ? DEFAULT_INTENSITY : v;
        }

        const next = clamp(current + delta * 0.005, 0, 1);
        if (intensityState) {
            intensityState.setNormalisedValue(next);
        }
        renderKnob(next);
    });

    window.addEventListener("mouseup", () => {
        if (!isDraggingKnob) return;
        isDraggingKnob = false;
        document.body.style.cursor = "default";
        if (intensityState) {
            intensityState.sliderDragEnded();
        }
    });

    outputSlider.addEventListener("input", () => {
        const val = Number.parseFloat(outputSlider.value);
        setScaledValueToState(outputState, val, OUTPUT_MIN_DB, OUTPUT_MAX_DB);
        renderOutput(val);
    });

    outputSlider.addEventListener("change", () => {
        const val = Number.parseFloat(outputSlider.value);
        setScaledValueToState(outputState, val, OUTPUT_MIN_DB, OUTPUT_MAX_DB);
    });

    outputSlider.addEventListener("mousedown", (e) => {
        if (!e.ctrlKey) return;
        e.preventDefault();
        resetOutputToDefault();
    });

    bypassBtn.addEventListener("click", () => {
        if (!bypassState) return;
        const next = !bypassState.getValue();
        bypassState.setValue(next);
        renderBypass(next);
    });

    oversamplingSelect.addEventListener("change", () => {
        if (!oversamplingState) return;
        oversamplingState.setChoiceIndex(oversamplingSelect.selectedIndex);
    });

    if (tightLookaheadSelect) {
        tightLookaheadSelect.addEventListener("change", () => {
            if (!tightLookaheadState) return;
            tightLookaheadState.setChoiceIndex(tightLookaheadSelect.selectedIndex);
        });
    }

    modeLeds.forEach((led) => {
        led.addEventListener("mouseenter", () => setPreviewMode(led));
        led.addEventListener("mouseleave", () => clearPreviewMode());
        led.addEventListener("click", () => {
            const idx = Number.parseInt(led.dataset.mode || "0", 10);
            clearPreviewMode();
            if (modeState) {
                modeState.setChoiceIndex(clamp(idx, 0, 2));
            }
            renderMode(idx);
        });
    });
});
