import * as Juce from "./juce/index.js";

const getSliderState = Juce.getSliderState;
const getToggleState = Juce.getToggleState;
const getComboBoxState = Juce.getComboBoxState;
const getNativeFunction = Juce.getNativeFunction;

const PARAM_INTENSITY = "intensity";
const PARAM_OUTPUT = "output_gain";
const PARAM_BYPASS = "bypass";
const PARAM_CLEAN_LOW = "clean_low_end";
const PARAM_MANUAL_MODE = "exp_manual_mode";
const PARAM_MUTE_LOW = "exp_mute_low_band";
const PARAM_MUTE_HIGH = "exp_mute_high_band";
const PARAM_PROCESSING_MODE = "exp_processing_mode";

const PARAM_OVERSAMPLING = "exp_oversampling_mode";
const PARAM_LOW_WAVESHAPER = "exp_low_waveshaper_type";
const PARAM_HIGH_WAVESHAPER = "exp_high_waveshaper_type";

const PARAM_BYPASS_LOW_TRANSIENT = "exp_bypass_low_transient";
const PARAM_BYPASS_LOW_SATURATION = "exp_bypass_low_saturation";
const PARAM_BYPASS_LOW_LIMITER = "exp_bypass_low_limiter";
const PARAM_BYPASS_HIGH_TRANSIENT = "exp_bypass_high_transient";
const PARAM_BYPASS_HIGH_SATURATION = "exp_bypass_high_saturation";
const PARAM_BYPASS_TIGHTNESS = "exp_bypass_tightness";
const PARAM_BYPASS_OUTPUT_LIMITER = "exp_bypass_output_limiter";

const MODE_LABELS = ["Osmium", "Tight", "Chaotic"];
const DEFAULT_INTENSITY = 0.15;
const DEFAULT_OUTPUT_DB = 0.0;
const OUTPUT_MIN_DB = -30.0;
const OUTPUT_MAX_DB = 0.0;
const isMacPlatform = /mac/i.test(navigator.platform || "");

function isResetModifier(event) {
    return isMacPlatform ? event.metaKey : event.ctrlKey;
}

const EXP_SLIDER_IDS = [
    "exp_crossover_hz",
    "exp_low_attack_boost_db",
    "exp_low_attack_time_ms",
    "exp_low_post_comp_db",
    "exp_low_comp_time_ms",
    "exp_low_body_lift_db",
    "exp_low_sat_mix",
    "exp_low_sat_drive",
    "exp_low_fast_attack_ms",
    "exp_low_fast_release_ms",
    "exp_low_slow_release_ms",
    "exp_low_attack_gain_smooth_ms",
    "exp_low_comp_attack_ratio",
    "exp_low_comp_gain_smooth_ms",
    "exp_low_transient_sensitivity",
    "exp_low_limiter_threshold_db",
    "exp_low_limiter_release_ms",
    "exp_high_attack_boost_db",
    "exp_high_attack_time_ms",
    "exp_high_post_comp_db",
    "exp_high_comp_time_ms",
    "exp_high_drive_db",
    "exp_high_sat_mix",
    "exp_high_sat_drive",
    "exp_high_trim_db",
    "exp_high_fast_attack_ms",
    "exp_high_fast_release_ms",
    "exp_high_slow_release_ms",
    "exp_high_attack_gain_smooth_ms",
    "exp_high_comp_attack_ratio",
    "exp_high_comp_gain_smooth_ms",
    "exp_high_transient_sensitivity",
    "exp_tight_sustain_depth_db",
    "exp_tight_sustain_attack_ms",
    "exp_tight_release_ms",
    "exp_tight_bell_freq_hz",
    "exp_tight_bell_cut_db",
    "exp_tight_bell_q",
    "exp_tight_high_shelf_cut_db",
    "exp_tight_high_shelf_freq_hz",
    "exp_tight_high_shelf_q",
    "exp_tight_fast_attack_ms",
    "exp_tight_fast_release_ms",
    "exp_tight_slow_attack_ms",
    "exp_tight_slow_release_ms",
    "exp_tight_program_attack_ms",
    "exp_tight_program_release_ms",
    "exp_tight_transient_sensitivity",
    "exp_tight_threshold_offset_db",
    "exp_tight_threshold_range_db",
    "exp_tight_threshold_floor_db",
    "exp_tight_threshold_ceil_db",
    "exp_tight_sustain_curve",
    "exp_output_auto_makeup_db",
    "exp_output_body_hold_db",
    "exp_limiter_threshold_db",
    "exp_limiter_release_ms"
];

const TIGHT_SLIDER_IDS = [
    "exp_tight_sustain_depth_db",
    "exp_tight_sustain_attack_ms",
    "exp_tight_release_ms",
    "exp_tight_bell_freq_hz",
    "exp_tight_bell_cut_db",
    "exp_tight_bell_q",
    "exp_tight_high_shelf_cut_db",
    "exp_tight_high_shelf_freq_hz",
    "exp_tight_high_shelf_q",
    "exp_tight_fast_attack_ms",
    "exp_tight_fast_release_ms",
    "exp_tight_slow_attack_ms",
    "exp_tight_slow_release_ms",
    "exp_tight_program_attack_ms",
    "exp_tight_program_release_ms",
    "exp_tight_transient_sensitivity",
    "exp_tight_threshold_offset_db",
    "exp_tight_threshold_range_db",
    "exp_tight_threshold_floor_db",
    "exp_tight_threshold_ceil_db",
    "exp_tight_sustain_curve"
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

    console.log(`[EXPERIMENTAL_OSMIUM_V3] ${message}`);
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
    if (paramId === PARAM_INTENSITY) return `${Math.round(value * 100)}%`;
    if (paramId.includes("_db")) return `${value.toFixed(1)}`;
    if (paramId.includes("_ms")) return `${value.toFixed(2)}`;
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

function bindChoiceSelector(selectId, choiceState, onUpdated) {
    const selectEl = document.getElementById(selectId);
    if (!selectEl || !choiceState) return;

    function getChoices() {
        const rawChoices = choiceState.properties && Array.isArray(choiceState.properties.choices)
            ? choiceState.properties.choices
            : [];

        if (rawChoices.length > 0) {
            return rawChoices;
        }

        return Array.from(selectEl.options).map((opt) => opt.textContent);
    }

    function refreshChoice() {
        const choices = getChoices();
        const maxIdx = Math.max(choices.length - 1, 0);
        const idx = clamp(choiceState.getChoiceIndex(), 0, maxIdx);

        if (selectEl.options.length !== choices.length) {
            selectEl.innerHTML = "";
            choices.forEach((choice, i) => {
                const option = document.createElement("option");
                option.value = String(i);
                option.textContent = choice;
                selectEl.appendChild(option);
            });
        }

        selectEl.selectedIndex = idx;
        if (onUpdated) onUpdated(choices[idx] || "");
    }

    selectEl.addEventListener("change", () => {
        const idx = clamp(selectEl.selectedIndex, 0, Math.max(selectEl.options.length - 1, 0));
        choiceState.setChoiceIndex(idx);
        const text = selectEl.options[idx] ? selectEl.options[idx].textContent : "";
        if (onUpdated && text) onUpdated(text);
        if (document.activeElement && typeof document.activeElement.blur === "function") {
            document.activeElement.blur();
        }
    });

    choiceState.valueChangedEvent.addListener(refreshChoice);
    if (choiceState.propertiesChangedEvent && typeof choiceState.propertiesChangedEvent.addListener === "function") {
        choiceState.propertiesChangedEvent.addListener(refreshChoice);
    }

    refreshChoice();
}

function bindSlider(paramId, options = {}) {
    const { juceReady, onRender = null, logChanges = true } = options;
    const slider = document.getElementById(paramId);
    const valueLabel = document.getElementById(`${paramId}_value`);
    if (!slider) return;

    const htmlMin = parseFloat(slider.min);
    const htmlMax = parseFloat(slider.max);
    const state = juceReady ? getSliderState(paramId) : null;

    const render = (value) => {
        if (valueLabel) {
            valueLabel.textContent = formatById(paramId, value);
        }
        if (onRender) {
            onRender(value);
        }
    };

    const applyStateProperties = () => {
        if (!state) return;
        const props = state.properties || {};
        if (Number.isFinite(props.start)) slider.min = `${props.start}`;
        if (Number.isFinite(props.end)) slider.max = `${props.end}`;
        if (Number.isFinite(props.interval) && props.interval > 0) slider.step = `${props.interval}`;
    };

    if (state) {
        state.valueChangedEvent.addListener(() => {
            const scaled = state.getScaledValue();
            if (!Number.isNaN(scaled)) {
                slider.value = scaled;
                render(scaled);
            }
        });

        state.propertiesChangedEvent.addListener(() => {
            applyStateProperties();
            const scaled = state.getScaledValue();
            if (!Number.isNaN(scaled)) {
                slider.value = scaled;
                render(scaled);
            }
        });

        setTimeout(() => {
            applyStateProperties();
            const scaled = state.getScaledValue();
            if (!Number.isNaN(scaled)) {
                slider.value = scaled;
                render(scaled);
            } else {
                render(parseFloat(slider.value));
            }
        }, 100);

        attachSliderGestureEvents(slider, state);
    } else {
        render(parseFloat(slider.value));
    }

    slider.addEventListener("input", () => {
        const value = parseFloat(slider.value);
        setScaledValueToState(state, value, htmlMin, htmlMax);
        render(value);
        if (logChanges) {
            logParamThrottled(paramId, formatById(paramId, value));
        }
    });

    slider.addEventListener("change", () => {
        const value = parseFloat(slider.value);
        setScaledValueToState(state, value, htmlMin, htmlMax);
        render(value);
    });
}

function bindBypassButton(juceReady, buttonId, paramId, label) {
    const button = document.getElementById(buttonId);
    if (!button) return;

    const state = juceReady ? getToggleState(paramId) : null;
    const render = (isBypassed) => {
        button.classList.toggle("active", !isBypassed);
        button.classList.toggle("bypassed", isBypassed);
        button.textContent = `${label}: ${isBypassed ? "Bypassed" : "Active"}`;
    };

    if (state) {
        state.valueChangedEvent.addListener(() => render(state.getValue()));
        setTimeout(() => render(state.getValue()), 100);
    } else {
        render(false);
    }

    button.addEventListener("click", () => {
        if (!state) return;
        const next = !state.getValue();
        state.setValue(next);
        render(next);
        if (document.activeElement && typeof document.activeElement.blur === "function") {
            document.activeElement.blur();
        }
    });
}

document.addEventListener("DOMContentLoaded", () => {
    document.addEventListener("contextmenu", (event) => event.preventDefault());

    debugConsole = document.getElementById("debug-console");
    const debugToggle = document.getElementById("debug-toggle");
    if (debugToggle && debugConsole) {
        debugToggle.addEventListener("click", () => debugConsole.classList.toggle("visible"));
    }

    const juceReady = typeof window.__JUCE__ !== "undefined";
    log("Experimental UI loaded", "success");
    log(`JUCE bridge: ${juceReady ? "ready" : "unavailable"}`, juceReady ? "success" : "error");

    const appRoot = document.getElementById("app-root");
    const expMenu = document.getElementById("exp-menu");
    const modeTag = document.getElementById("mode-tag");

    const bypassBtn = document.getElementById("bypass");
    const cleanLowBtn = document.getElementById("clean_low_end");
    const lowMuteBtn = document.getElementById("low-mute");
    const highMuteBtn = document.getElementById("high-mute");
    const manualModeBtn = document.getElementById("manual-mode");
    const intensitySlider = document.getElementById(PARAM_INTENSITY);
    const outputSlider = document.getElementById(PARAM_OUTPUT);

    const bypassState = juceReady ? getToggleState(PARAM_BYPASS) : null;
    const cleanLowState = juceReady ? getToggleState(PARAM_CLEAN_LOW) : null;
    const manualModeState = juceReady ? getToggleState(PARAM_MANUAL_MODE) : null;
    const lowMuteState = juceReady ? getToggleState(PARAM_MUTE_LOW) : null;
    const highMuteState = juceReady ? getToggleState(PARAM_MUTE_HIGH) : null;
    const processingModeState = juceReady ? getComboBoxState(PARAM_PROCESSING_MODE) : null;
    const intensityState = juceReady ? getSliderState(PARAM_INTENSITY) : null;
    const outputState = juceReady ? getSliderState(PARAM_OUTPUT) : null;

    function releaseFocus() {
        const active = document.activeElement;
        if (active && typeof active.blur === "function") {
            active.blur();
        }
        if (typeof window.blur === "function") {
            window.blur();
        }

        // Notify native side to unfocus for DAW Spacebar support
        if (juceReady) {
            try {
                const focusHost = Juce.getNativeFunction("focusHost");
                if (typeof focusHost === "function") {
                    focusHost();
                }
            } catch (e) {
                console.warn("focusHost native function not available", e);
            }
        }
    }

    document.querySelectorAll("button, select, input[type=\"range\"]").forEach((el) => {
        if (typeof el.setAttribute === "function") {
            el.setAttribute("tabindex", "-1");
        }
    });
    document.addEventListener("keydown", releaseFocus, true);
    document.addEventListener("pointerdown", () => setTimeout(releaseFocus, 0), true);
    document.addEventListener("pointerup", releaseFocus, true);

    const modeButtons = [
        document.getElementById("mode-osmium"),
        document.getElementById("mode-tight"),
        document.getElementById("mode-chaotic")
    ];

    const tightSections = [
        document.getElementById("tight-controls-section"),
        document.getElementById("tight-detector-section")
    ];

    const setTightSectionsEnabled = (enabled) => {
        tightSections.forEach((section) => {
            if (!section) return;
            section.classList.toggle("disabled", !enabled);
            section.querySelectorAll("input, select").forEach((control) => {
                control.disabled = !enabled;
            });
        });
    };

    const refreshModeVisual = () => {
        let idx = 0;
        if (processingModeState) {
            idx = clamp(processingModeState.getChoiceIndex(), 0, 2);
        }

        modeButtons.forEach((btn, i) => {
            if (!btn) return;
            btn.classList.toggle("active", i === idx);
        });

        const choiceLabels = processingModeState?.properties?.choices;
        const modeLabel = Array.isArray(choiceLabels) && choiceLabels[idx]
            ? choiceLabels[idx]
            : MODE_LABELS[idx];
        if (modeTag) modeTag.textContent = `Mode: ${modeLabel}`;
        setTightSectionsEnabled(idx === 1);
    };

    modeButtons.forEach((button, idx) => {
        if (!button) return;
        button.addEventListener("click", () => {
            if (processingModeState) {
                processingModeState.setChoiceIndex(idx);
            }
            refreshModeVisual();
            releaseFocus();
        });
    });

    if (processingModeState) {
        processingModeState.valueChangedEvent.addListener(refreshModeVisual);
        if (processingModeState.propertiesChangedEvent) {
            processingModeState.propertiesChangedEvent.addListener(refreshModeVisual);
        }
    }
    setTimeout(refreshModeVisual, 100);

    const refreshManualModeVisual = () => {
        if (!manualModeBtn) return;
        const manual = manualModeState ? manualModeState.getValue() : false;
        manualModeBtn.classList.toggle("active", manual);
        manualModeBtn.textContent = manual ? "Manual On" : "Core Link";
    };

    const renderCleanLow = (enabled) => {
        if (!cleanLowBtn) return;
        cleanLowBtn.classList.toggle("active", enabled);
        cleanLowBtn.textContent = enabled ? "Clean Low On" : "Clean Low";
    };

    if (manualModeState) {
        manualModeState.valueChangedEvent.addListener(refreshManualModeVisual);
        setTimeout(refreshManualModeVisual, 100);
    } else {
        refreshManualModeVisual();
    }

    if (manualModeBtn) {
        manualModeBtn.addEventListener("click", () => {
            if (!manualModeState) return;
            manualModeState.setValue(!manualModeState.getValue());
            refreshManualModeVisual();
            releaseFocus();
        });
    }

    if (cleanLowState) {
        cleanLowState.valueChangedEvent.addListener(() => renderCleanLow(cleanLowState.getValue()));
        setTimeout(() => renderCleanLow(cleanLowState.getValue()), 100);
    } else {
        renderCleanLow(false);
    }

    if (cleanLowBtn) {
        cleanLowBtn.addEventListener("click", () => {
            if (!cleanLowState) return;
            const next = !cleanLowState.getValue();
            cleanLowState.setValue(next);
            renderCleanLow(next);
            releaseFocus();
        });
    }

    bindSlider(PARAM_INTENSITY, {
        juceReady,
        logChanges: true,
        onRender: (value) => {
            const label = document.getElementById("intensity_value");
            if (label) label.textContent = `${Math.round(clamp(value, 0, 1) * 100)}%`;
        }
    });

    bindSlider(PARAM_OUTPUT, {
        juceReady,
        logChanges: true,
        onRender: (value) => {
            const outputDisplay = document.getElementById("output-value-display");
            if (outputDisplay) outputDisplay.textContent = `${value.toFixed(1)} dB`;
        }
    });

    if (intensitySlider) {
        intensitySlider.addEventListener("mousedown", (event) => {
            if (!isResetModifier(event)) return;
            event.preventDefault();
            setScaledValueToState(intensityState, DEFAULT_INTENSITY, 0.0, 1.0);
            intensitySlider.value = `${DEFAULT_INTENSITY}`;
            intensitySlider.dispatchEvent(new Event("input", { bubbles: true }));
            intensitySlider.dispatchEvent(new Event("change", { bubbles: true }));
            releaseFocus();
        });
    }

    if (outputSlider) {
        outputSlider.addEventListener("mousedown", (event) => {
            if (!isResetModifier(event)) return;
            event.preventDefault();
            setScaledValueToState(outputState, DEFAULT_OUTPUT_DB, OUTPUT_MIN_DB, OUTPUT_MAX_DB);
            outputSlider.value = `${DEFAULT_OUTPUT_DB}`;
            outputSlider.dispatchEvent(new Event("input", { bubbles: true }));
            outputSlider.dispatchEvent(new Event("change", { bubbles: true }));
            releaseFocus();
        });
    }

    for (const id of EXP_SLIDER_IDS) {
        bindSlider(id, { juceReady, logChanges: true });
    }

    bindChoiceSelector(PARAM_OVERSAMPLING, juceReady ? getComboBoxState(PARAM_OVERSAMPLING) : null, (label) => {
        if (label) logParamThrottled(PARAM_OVERSAMPLING, label);
    });
    bindChoiceSelector(PARAM_LOW_WAVESHAPER, juceReady ? getComboBoxState(PARAM_LOW_WAVESHAPER) : null, (label) => {
        if (label) logParamThrottled(PARAM_LOW_WAVESHAPER, label);
    });
    bindChoiceSelector(PARAM_HIGH_WAVESHAPER, juceReady ? getComboBoxState(PARAM_HIGH_WAVESHAPER) : null, (label) => {
        if (label) logParamThrottled(PARAM_HIGH_WAVESHAPER, label);
    });

    bindBypassButton(juceReady, "bypass-low-transient", PARAM_BYPASS_LOW_TRANSIENT, "Low Transient");
    bindBypassButton(juceReady, "bypass-low-saturation", PARAM_BYPASS_LOW_SATURATION, "Low Saturation");
    bindBypassButton(juceReady, "bypass-low-limiter", PARAM_BYPASS_LOW_LIMITER, "Low Limiter");
    bindBypassButton(juceReady, "bypass-high-transient", PARAM_BYPASS_HIGH_TRANSIENT, "High Transient");
    bindBypassButton(juceReady, "bypass-high-saturation", PARAM_BYPASS_HIGH_SATURATION, "High Saturation");
    bindBypassButton(juceReady, "bypass-tightness", PARAM_BYPASS_TIGHTNESS, "Tightness");
    bindBypassButton(juceReady, "bypass-output-limiter", PARAM_BYPASS_OUTPUT_LIMITER, "Output Limiter");

    const updateBypassVis = (isBypassed) => {
        if (!bypassBtn || !appRoot) return;
        if (isBypassed) {
            bypassBtn.textContent = "Bypassed";
            bypassBtn.classList.remove("active");
            appRoot.style.opacity = "0.6";
        } else {
            bypassBtn.textContent = "Active";
            bypassBtn.classList.add("active");
            appRoot.style.opacity = "1";
        }
    };

    const updateMuteButton = (button, muted, prefix) => {
        if (!button) return;
        if (muted) {
            button.classList.add("active");
            button.textContent = `${prefix} Muted`;
        } else {
            button.classList.remove("active");
            button.textContent = `Mute ${prefix}`;
        }
    };

    if (bypassState) {
        bypassState.valueChangedEvent.addListener(() => updateBypassVis(bypassState.getValue()));
        setTimeout(() => updateBypassVis(bypassState.getValue()), 100);
    } else {
        updateBypassVis(false);
    }

    if (bypassBtn) {
        bypassBtn.addEventListener("click", () => {
            if (!bypassState) return;
            const next = !bypassState.getValue();
            bypassState.setValue(next);
            updateBypassVis(next);
            releaseFocus();
        });
    }

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

    if (lowMuteBtn) {
        lowMuteBtn.addEventListener("click", () => {
            if (!lowMuteState) return;
            lowMuteState.setValue(!lowMuteState.getValue());
            releaseFocus();
        });
    }

    if (highMuteBtn) {
        highMuteBtn.addEventListener("click", () => {
            if (!highMuteState) return;
            highMuteState.setValue(!highMuteState.getValue());
            releaseFocus();
        });
    }

    const setPlaygroundOpen = (isOpen) => {
        if (!expMenu) return;
        expMenu.classList.toggle("visible", isOpen);
        if (appRoot) {
            appRoot.classList.toggle("playground-open", isOpen);
        }
    };

    const expToggle = document.getElementById("exp-toggle");
    if (expToggle && expMenu) {
        expToggle.addEventListener("click", () => {
            const shouldOpen = !expMenu.classList.contains("visible");
            setPlaygroundOpen(shouldOpen);
            releaseFocus();
        });
    }

    const expCloseButton = document.getElementById("exp-close-btn");
    if (expCloseButton && expMenu) {
        expCloseButton.addEventListener("click", () => {
            setPlaygroundOpen(false);
            releaseFocus();
        });
    }

    const resetButton = document.getElementById("reset-all-btn");
    const resetAllFn = juceReady ? getNativeFunction("resetAllParameters") : null;
    if (resetButton) {
        resetButton.addEventListener("click", async () => {
            if (!resetAllFn) {
                log("Reset unavailable (bridge not connected)", "error");
                return;
            }

            resetButton.disabled = true;
            try {
                await resetAllFn();
                log("All parameters reset to defaults", "success");
            } catch (err) {
                log(`Reset failed: ${err}`, "error");
            } finally {
                resetButton.disabled = false;
            }
            releaseFocus();
        });
    }

    const inputFill = document.getElementById("meter-input-fill");
    const outputFill = document.getElementById("meter-output-fill");
    const reductionFill = document.getElementById("meter-reduction-fill");
    const inputValue = document.getElementById("meter-input-value");
    const outputValue = document.getElementById("meter-output-value");
    const reductionValue = document.getElementById("meter-reduction-value");

    const dbToPercent = (db, minDb, maxDb) => {
        const clamped = clamp(db, minDb, maxDb);
        return ((clamped - minDb) / (maxDb - minDb)) * 100;
    };

    const updateMeters = (payload) => {
        const inputDb = Number(payload?.inputDb ?? -100);
        const outputDb = Number(payload?.outputDb ?? -100);
        const reductionDb = Number(payload?.reductionDb ?? 0);

        if (inputFill) inputFill.style.width = `${dbToPercent(inputDb, -60, 6).toFixed(2)}%`;
        if (outputFill) outputFill.style.width = `${dbToPercent(outputDb, -60, 6).toFixed(2)}%`;
        if (reductionFill) reductionFill.style.width = `${dbToPercent(reductionDb, 0, 24).toFixed(2)}%`;

        if (inputValue) inputValue.textContent = `${inputDb.toFixed(1)} dB`;
        if (outputValue) outputValue.textContent = `${outputDb.toFixed(1)} dB`;
        if (reductionValue) reductionValue.textContent = `${Math.max(0, reductionDb).toFixed(1)} dB`;
    };

    updateMeters(null);
    if (juceReady && window.__JUCE__ && window.__JUCE__.backend) {
        window.__JUCE__.backend.addEventListener("exp_meter_update", updateMeters);
    }
});
