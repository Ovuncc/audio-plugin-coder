import * as Juce from "./juce/index.js";

const getSliderState = Juce.getSliderState;
const getToggleState = Juce.getToggleState;

const PARAM_INTENSITY = "intensity";
const PARAM_OUTPUT = "output_gain";
const PARAM_BYPASS = "bypass";
const PARAM_EXP_MANUAL = "exp_manual_mode";
const PARAM_MUTE_LOW = "exp_mute_low_band";
const PARAM_MUTE_HIGH = "exp_mute_high_band";

const EXP_PARAM_IDS = [
    "exp_crossover_hz",
    "exp_low_attack_boost_db",
    "exp_low_attack_time_ms",
    "exp_low_post_comp_db",
    "exp_low_comp_time_ms",
    "exp_low_body_lift_db",
    "exp_low_sat_mix",
    "exp_low_sat_drive",
    "exp_high_attack_boost_db",
    "exp_high_attack_time_ms",
    "exp_high_post_comp_db",
    "exp_high_comp_time_ms",
    "exp_high_drive_db",
    "exp_high_sat_mix",
    "exp_high_sat_drive",
    "exp_high_trim_db",
    "exp_limiter_threshold_db",
    "exp_limiter_release_ms"
];

const PARAM_LOG_THROTTLE_MS = 120;
const paramLogTimes = new Map();
let debugConsole = null;

function log(message, type = "info") {
    if (!debugConsole) {
        debugConsole = document.getElementById("debug-console");
    }

    if (debugConsole) {
        const line = document.createElement("div");
        line.className = `log-${type}`;
        line.textContent = `[${new Date().toLocaleTimeString()}] ${message}`;
        debugConsole.appendChild(line);
        debugConsole.scrollTop = debugConsole.scrollHeight;
    }

    console.log(`[EXPERIMENTAL_OSM] ${message}`);
}

function logParamThrottled(paramId, valueText) {
    const now = performance.now();
    const last = paramLogTimes.get(paramId) || 0;
    if (now - last >= PARAM_LOG_THROTTLE_MS) {
        log(`${paramId} -> ${valueText}`, "info");
        paramLogTimes.set(paramId, now);
    }
}

function clamp(value, min, max) {
    return Math.max(min, Math.min(max, value));
}

function stateBounds(state, fallbackMin, fallbackMax) {
    const props = state?.properties || {};
    const start = Number.isFinite(props.start) ? props.start : fallbackMin;
    const end = Number.isFinite(props.end) ? props.end : fallbackMax;
    const low = Math.min(start, end);
    const high = Math.max(start, end);
    const skew = Number.isFinite(props.skew) && props.skew > 0 ? props.skew : 1;
    return { start, end, low, high, skew };
}

function setScaledValueToState(state, scaledValue, fallbackMin, fallbackMax) {
    if (!state) return;
    const { start, end, low, high, skew } = stateBounds(state, fallbackMin, fallbackMax);
    const clamped = clamp(scaledValue, low, high);
    const span = Math.max(1.0e-9, end - start);
    const ratio = clamp((clamped - start) / span, 0, 1);
    state.setNormalisedValue(Math.pow(ratio, skew));
}

function formatById(paramId, value) {
    if (paramId.includes("_db")) return `${value.toFixed(1)}`;
    if (paramId.includes("_ms")) return `${value.toFixed(1)}`;
    if (paramId.endsWith("_mix")) return `${value.toFixed(2)}`;
    if (paramId.endsWith("_drive")) return `${value.toFixed(2)}`;
    if (paramId.endsWith("_hz")) return `${value.toFixed(0)}`;
    return `${value.toFixed(2)}`;
}

function attachSliderGestureEvents(slider, state) {
    if (!slider || !state) return;
    let dragging = false;

    const start = () => {
        if (dragging) return;
        dragging = true;
        state.sliderDragStarted();
    };

    const end = () => {
        if (!dragging) return;
        dragging = false;
        state.sliderDragEnded();
    };

    slider.addEventListener("pointerdown", start);
    slider.addEventListener("pointerup", end);
    slider.addEventListener("pointercancel", end);
    slider.addEventListener("change", end);
    window.addEventListener("pointerup", end);
}

document.addEventListener("DOMContentLoaded", () => {
    debugConsole = document.getElementById("debug-console");
    const debugToggle = document.getElementById("debug-toggle");
    debugToggle.addEventListener("click", () => {
        debugConsole.classList.toggle("visible");
    });

    const juceReady = typeof window.__JUCE__ !== "undefined";
    log("Experimental UI loaded", "success");
    log(`JUCE bridge: ${juceReady ? "ready" : "unavailable"}`, juceReady ? "success" : "error");

    const knob = document.getElementById("intensity-knob");
    const glowRing = document.getElementById("glow-ring");
    const valueDisplay = document.getElementById("intensity-value");
    const indicator = document.getElementById("knob-indicator");
    const outputSlider = document.getElementById(PARAM_OUTPUT);
    const outputDisplay = document.getElementById("output-value-display");
    const bypassBtn = document.getElementById("bypass");
    const lowMuteBtn = document.getElementById("low-mute");
    const highMuteBtn = document.getElementById("high-mute");

    const expMenu = document.getElementById("exp-menu");
    document.getElementById("exp-toggle").addEventListener("click", () => expMenu.classList.toggle("visible"));
    document.getElementById("exp-close-btn").addEventListener("click", () => expMenu.classList.remove("visible"));

    const modeTag = document.getElementById("mode-tag");
    const useCoreBtn = document.getElementById("use-core-btn");

    const outputState = juceReady ? getSliderState(PARAM_OUTPUT) : null;
    const bypassState = juceReady ? getToggleState(PARAM_BYPASS) : null;
    const manualState = juceReady ? getToggleState(PARAM_EXP_MANUAL) : null;
    const lowMuteState = juceReady ? getToggleState(PARAM_MUTE_LOW) : null;
    const highMuteState = juceReady ? getToggleState(PARAM_MUTE_HIGH) : null;

    function updateKnobLockedVis() {
        valueDisplay.textContent = "LOCK";
        glowRing.style.background = "conic-gradient(var(--accent) 48deg, transparent 48deg)";
        glowRing.style.boxShadow = "0 0 22px var(--accent)";
        indicator.style.transform = "translateX(-50%) rotate(48deg)";
        knob.classList.add("locked");
    }

    function updateOutputVis(value) {
        outputDisplay.textContent = `${value.toFixed(1)} dB`;
    }

    function updateBypassVis(isBypassed) {
        if (isBypassed) {
            bypassBtn.textContent = "Bypassed";
            bypassBtn.classList.remove("active");
            document.getElementById("app-root").style.opacity = "0.6";
        } else {
            bypassBtn.textContent = "Active";
            bypassBtn.classList.add("active");
            document.getElementById("app-root").style.opacity = "1";
        }
    }

    function updateManualLockedVis() {
        modeTag.textContent = "Manual Locked";
        modeTag.classList.add("manual");
        knob.classList.add("locked");
    }

    function updateMuteButton(button, muted, prefix) {
        if (!button) return;
        if (muted) {
            button.classList.add("active");
            button.textContent = `${prefix} Muted`;
        } else {
            button.classList.remove("active");
            button.textContent = `Mute ${prefix}`;
        }
    }

    function enforceManualMode() {
        if (!manualState) return;
        if (!manualState.getValue()) {
            manualState.setValue(true);
            log("Manual mode forced ON (Core is locked)", "success");
        }
        updateManualLockedVis();
    }

    function bindSlider(paramId, logChanges = true) {
        const slider = document.getElementById(paramId);
        const valueLabel = document.getElementById(`${paramId}_value`);
        if (!slider) return;

        const htmlMin = parseFloat(slider.min);
        const htmlMax = parseFloat(slider.max);
        const state = juceReady ? getSliderState(paramId) : null;

        const renderValue = (value) => {
            if (valueLabel) {
                valueLabel.textContent = formatById(paramId, value);
            }
        };

        const applyStateProperties = () => {
            if (!state) return;
            const props = state.properties || {};
            if (Number.isFinite(props.start)) slider.min = `${props.start}`;
            if (Number.isFinite(props.end)) slider.max = `${props.end}`;
            if (Number.isFinite(props.interval) && props.interval > 0) slider.step = `${props.interval}`;
        };

        const pushToState = (value) => {
            setScaledValueToState(state, value, htmlMin, htmlMax);
        };

        if (state) {
            state.valueChangedEvent.addListener(() => {
                const scaled = state.getScaledValue();
                if (!Number.isNaN(scaled)) {
                    slider.value = scaled;
                    renderValue(scaled);
                }
            });

            state.propertiesChangedEvent.addListener(() => {
                applyStateProperties();
                const scaled = state.getScaledValue();
                if (!Number.isNaN(scaled)) {
                    slider.value = scaled;
                    renderValue(scaled);
                }
            });

            setTimeout(() => {
                applyStateProperties();
                const initial = state.getScaledValue();
                if (!Number.isNaN(initial)) {
                    slider.value = initial;
                    renderValue(initial);
                } else {
                    renderValue(parseFloat(slider.value));
                }
            }, 100);

            attachSliderGestureEvents(slider, state);
        } else {
            renderValue(parseFloat(slider.value));
        }

        slider.addEventListener("input", () => {
            const value = parseFloat(slider.value);
            pushToState(value);
            renderValue(value);
            enforceManualMode();
            if (logChanges) {
                logParamThrottled(paramId, formatById(paramId, value));
            }
        });

        slider.addEventListener("change", () => {
            const value = parseFloat(slider.value);
            pushToState(value);
            renderValue(value);
        });
    }

    updateKnobLockedVis();
    updateManualLockedVis();

    if (manualState) {
        manualState.valueChangedEvent.addListener(() => {
            enforceManualMode();
        });
        setTimeout(() => enforceManualMode(), 120);
    }

    useCoreBtn.addEventListener("click", () => {
        log("Core macro is disabled in Experimental_Osm", "info");
    });

    // Output
    if (outputState) {
        outputState.valueChangedEvent.addListener(() => {
            const val = outputState.getScaledValue();
            if (!Number.isNaN(val)) {
                outputSlider.value = val;
                updateOutputVis(val);
            }
        });
        setTimeout(() => {
            const val = outputState.getScaledValue();
            updateOutputVis(Number.isNaN(val) ? 0 : val);
            outputSlider.value = Number.isNaN(val) ? 0 : val;
        }, 100);
        attachSliderGestureEvents(outputSlider, outputState);
    } else {
        updateOutputVis(0);
    }

    outputSlider.addEventListener("input", () => {
        const value = parseFloat(outputSlider.value);
        setScaledValueToState(outputState, value, -12, 12);
        updateOutputVis(value);
        logParamThrottled(PARAM_OUTPUT, `${value.toFixed(1)} dB`);
    });
    outputSlider.addEventListener("change", () => {
        const value = parseFloat(outputSlider.value);
        setScaledValueToState(outputState, value, -12, 12);
    });

    // Bypass
    if (bypassState) {
        bypassState.valueChangedEvent.addListener(() => updateBypassVis(bypassState.getValue()));
        setTimeout(() => updateBypassVis(bypassState.getValue()), 100);
    } else {
        updateBypassVis(false);
    }
    bypassBtn.addEventListener("click", () => {
        if (!bypassState) return;
        const next = !bypassState.getValue();
        bypassState.setValue(next);
        updateBypassVis(next);
        log(`Bypass -> ${next ? "ON" : "OFF"}`, "info");
    });

    // Band mute buttons
    if (lowMuteState) {
        lowMuteState.valueChangedEvent.addListener(() => updateMuteButton(lowMuteBtn, lowMuteState.getValue(), "Low"));
        setTimeout(() => updateMuteButton(lowMuteBtn, lowMuteState.getValue(), "Low"), 100);
    } else {
        updateMuteButton(lowMuteBtn, false, "Low");
    }

    if (highMuteState) {
        highMuteState.valueChangedEvent.addListener(() => updateMuteButton(highMuteBtn, highMuteState.getValue(), "High"));
        setTimeout(() => updateMuteButton(highMuteBtn, highMuteState.getValue(), "High"), 100);
    } else {
        updateMuteButton(highMuteBtn, false, "High");
    }

    lowMuteBtn.addEventListener("click", () => {
        if (!lowMuteState) return;
        const next = !lowMuteState.getValue();
        lowMuteState.setValue(next);
        updateMuteButton(lowMuteBtn, next, "Low");
        log(`Low band mute -> ${next ? "ON" : "OFF"}`, "info");
    });

    highMuteBtn.addEventListener("click", () => {
        if (!highMuteState) return;
        const next = !highMuteState.getValue();
        highMuteState.setValue(next);
        updateMuteButton(highMuteBtn, next, "High");
        log(`High band mute -> ${next ? "ON" : "OFF"}`, "info");
    });

    // Experimental sliders
    for (const id of EXP_PARAM_IDS) {
        bindSlider(id, true);
    }

    // Keep a read-only binding for the core parameter so automation still reflects in logs if needed.
    if (juceReady) {
        const intensityState = getSliderState(PARAM_INTENSITY);
        intensityState.valueChangedEvent.addListener(() => {
            const val = intensityState.getNormalisedValue();
            if (!Number.isNaN(val)) {
                logParamThrottled(PARAM_INTENSITY, `${Math.round(val * 100)}% (locked)`);
            }
        });
    }
});
