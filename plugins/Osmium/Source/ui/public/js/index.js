import * as Juce from "./juce/index.js";

// Parameter Handling
const getSliderState = Juce.getSliderState;
const getToggleState = Juce.getToggleState;

// Define parameters from spec
const PARAM_INTENSITY = "intensity"; // 0.0 - 1.0
const PARAM_OUTPUT = "output_gain";  // -12.0 - 12.0
const PARAM_BYPASS = "bypass";       // 0 - 1
const DEFAULT_INTENSITY = 0.15;
const DEFAULT_OUTPUT_DB = 0.0;
const OUTPUT_MIN_DB = -12.0;
const OUTPUT_MAX_DB = 12.0;

// ===== DEBUG LOGGING =====
let debugConsole = null;

function log(message, type = 'info') {
    if (!debugConsole) {
        debugConsole = document.getElementById('debug-console');
    }

    if (debugConsole) {
        const line = document.createElement('div');
        line.className = `log-${type}`;
        line.textContent = `[${new Date().toLocaleTimeString()}] ${message}`;
        debugConsole.appendChild(line);
        debugConsole.scrollTop = debugConsole.scrollHeight;
    }

    // Also log to browser console
    console.log(`[OSMIUM] ${message}`);
}

function clamp(value, min, max) {
    return Math.max(min, Math.min(max, value));
}

function toneForValue(value) {
    const t = clamp(value, 0, 1);
    const hue = 205 + ((12 - 205) * t);
    const sat = 34 + (66 * (1 - Math.pow(1 - t, 1.6)));
    const light = 66 + ((50 - 66) * Math.pow(t, 0.9));
    return `hsl(${hue.toFixed(1)}, ${sat.toFixed(1)}%, ${light.toFixed(1)}%)`;
}

function outputDbToNormalised(dbValue) {
    return clamp((dbValue - OUTPUT_MIN_DB) / (OUTPUT_MAX_DB - OUTPUT_MIN_DB), 0, 1);
}

// Initialize debug console after DOM loads
document.addEventListener("DOMContentLoaded", () => {
    debugConsole = document.getElementById('debug-console');
    
    // Debug console toggle functionality
    const debugToggle = document.getElementById('debug-toggle');
    const debugClose = document.getElementById('debug-close');
    
    debugToggle.addEventListener('click', () => {
        debugConsole.classList.toggle('visible');
    });
    
    debugClose.addEventListener('click', () => {
        debugConsole.classList.remove('visible');
    });
    
    log("=== OSMIUM DEBUG SESSION ===", "info");
    log("DOM Content Loaded", "success");

    // Check window.__JUCE__ presence
    if (typeof window.__JUCE__ === 'undefined') {
        log("ERROR: window.__JUCE__ is UNDEFINED!", "error");
        log("JUCE backend may not be initialized yet", "error");
    } else {
        log("✓ window.__JUCE__ exists", "success");

        // Log initialisationData presence
        if (window.__JUCE__.initialisationData) {
            log("✓ initialisationData exists", "success");

            // Log all keys in initialisationData
            const keys = Object.keys(window.__JUCE__.initialisationData);
            log(`InitData Keys (${keys.length}): ${keys.join(', ')}`, "info");

            // Check for our specific parameters in JUCE 8 collection names
            const sliders = window.__JUCE__.initialisationData.__juce__sliders || [];
            const toggles = window.__JUCE__.initialisationData.__juce__toggles || [];
            const combos = window.__JUCE__.initialisationData.__juce__comboBoxes || [];

            [PARAM_INTENSITY, PARAM_OUTPUT].forEach(param => {
                if (sliders.includes(param)) {
                    log(`  ✓ Found slider: ${param}`, "success");
                } else {
                    log(`  ✗ MISSING slider: ${param}`, "error");
                }
            });

            if (toggles.includes(PARAM_BYPASS)) {
                log(`  ✓ Found toggle: ${PARAM_BYPASS}`, "success");
            } else {
                log(`  ✗ MISSING toggle: ${PARAM_BYPASS}`, "error");
            }
        } else {
            log("ERROR: initialisationData is missing!", "error");
        }

        // Try to get slider state for intensity
        try {
            const intensityState = getSliderState(PARAM_INTENSITY);
            if (intensityState) {
                log("✓ getSliderState(intensity) succeeded", "success");

                try {
                    // SliderState uses getNormalisedValue or getScaledValue, not getValue
                    const value = intensityState.getNormalisedValue();
                    log(`  Initial value: ${value}`, "info");
                } catch (e) {
                    log(`  ERROR calling getNormalisedValue(): ${e.message}`, "error");
                }
            } else {
                log("ERROR: getSliderState returned null/undefined", "error");
            }
        } catch (e) {
            log(`ERROR: getSliderState threw exception: ${e.message}`, "error");
        }
    }

    // --- DOM Elements ---
    const knob = document.getElementById('intensity-knob');
    const glowRing = document.getElementById('glow-ring');
    const valueDisplay = document.getElementById('intensity-value');
    const indicator = document.getElementById('knob-indicator');

    const outputSlider = document.getElementById('output_gain');
    const outputDisplay = document.getElementById('output-value-display');
    const bypassBtn = document.getElementById('bypass');

    // --- State ---
    let isDragging = false;
    let lastY = 0;
    let intensityState = null;
    let outputState = null;

    function resetIntensityToDefault() {
        if (intensityState) {
            intensityState.sliderDragStarted();
            intensityState.setNormalisedValue(DEFAULT_INTENSITY);
            intensityState.sliderDragEnded();
        }
        updateKnobVis(DEFAULT_INTENSITY);
    }

    function resetOutputToDefault() {
        const defaultNormalised = outputDbToNormalised(DEFAULT_OUTPUT_DB);

        if (outputState) {
            outputState.sliderDragStarted();
            outputState.setNormalisedValue(defaultNormalised);
            outputState.sliderDragEnded();
        }

        outputSlider.value = DEFAULT_OUTPUT_DB;
        updateOutputDisplay(DEFAULT_OUTPUT_DB);
    }

    // --- JUCE Binding ---

    // 1. Intensity Knob (Custom UI -> JUCE)
    // We need to fetch the initial state and listen for updates
    if (window.__JUCE__) {
        intensityState = getSliderState(PARAM_INTENSITY);

        // Listen for updates from C++ (automation, preset load)
        intensityState.valueChangedEvent.addListener(() => {
            const val = intensityState.getNormalisedValue();
            if (!isNaN(val)) {
                updateKnobVis(val); // Update UI
            }
        });

        // Initialize UI with delay to ensure properties are loaded
        setTimeout(() => {
            const val = intensityState.getNormalisedValue();
            if (!isNaN(val)) {
                updateKnobVis(val);
            } else {
                updateKnobVis(DEFAULT_INTENSITY);
            }
        }, 100);
    } else {
        // Fallback if JUCE not available
        updateKnobVis(DEFAULT_INTENSITY);
    }

    // 2. Output Slider (Input Range -> JUCE)
    if (window.__JUCE__) {
        outputState = getSliderState(PARAM_OUTPUT);

        outputState.valueChangedEvent.addListener(() => {
            const val = outputState.getScaledValue();
            if (!isNaN(val)) {
                outputSlider.value = val;
                updateOutputDisplay(val);
            }
        });

        // Set initial with delay
        setTimeout(() => {
            const initialOutput = outputState.getScaledValue();
            if (!isNaN(initialOutput)) {
                outputSlider.value = initialOutput;
                updateOutputDisplay(initialOutput);
            } else {
                outputSlider.value = DEFAULT_OUTPUT_DB;
                updateOutputDisplay(DEFAULT_OUTPUT_DB);
            }
        }, 100);

        // Bind input event to JUCE
        outputSlider.addEventListener('input', () => {
            const val = parseFloat(outputSlider.value);
            const normalised = outputDbToNormalised(val);
            outputState.setNormalisedValue(normalised);
            updateOutputDisplay(val); // Local update for responsiveness
        });

        outputSlider.addEventListener('change', () => {
            const val = parseFloat(outputSlider.value);
            const normalised = outputDbToNormalised(val);
            outputState.setNormalisedValue(normalised);
        });
    } else {
        outputSlider.value = DEFAULT_OUTPUT_DB;
        updateOutputDisplay(DEFAULT_OUTPUT_DB);
    }

    outputSlider.addEventListener('mousedown', (event) => {
        if (!event.ctrlKey) {
            return;
        }

        event.preventDefault();
        resetOutputToDefault();
    });

    // 3. Bypass Toggle
    if (window.__JUCE__) {
        const bypassState = getToggleState(PARAM_BYPASS);

        bypassState.valueChangedEvent.addListener(() => {
            const val = bypassState.getValue();
            updateBypassVis(val);
        });

        // Initialize with delay
        setTimeout(() => {
            const val = bypassState.getValue();
            updateBypassVis(val);
        }, 100);

        bypassBtn.addEventListener('click', () => {
            // Toggle state
            const current = bypassState.getValue();
            bypassState.setValue(!current);
            // Optimistic UI update
            updateBypassVis(!current);
        });
    } else {
        updateBypassVis(false);
    }

    // --- UI Logic ---

    function updateKnobVis(value) {
        const normalized = clamp(value, 0, 1);
        const percent = Math.round(normalized * 100);
        valueDisplay.innerText = percent + "%";

        const degrees = normalized * 360;
        const color = toneForValue(normalized);

        glowRing.style.background = `conic-gradient(${color} ${degrees}deg, transparent ${degrees}deg)`;
        glowRing.style.boxShadow = `0 0 ${20 + (normalized * 30)}px ${color}`;
        indicator.style.transform = `translateX(-50%) rotate(${degrees}deg)`;
        indicator.style.background = color;
        indicator.style.boxShadow = `0 0 8px ${color}`;

        valueDisplay.style.color = normalized > 0 ? "white" : "var(--text-muted)";
        valueDisplay.style.textShadow = normalized > 0 ? `0 0 10px ${color}` : "none";
    }

    function updateOutputDisplay(val) {
        outputDisplay.innerText = val.toFixed(1) + "dB";
    }

    function updateBypassVis(isBypassed) {
        // Param: 1 = Bypassed, 0 = Active usually? Or vice versa?
        // Spec: Bypass (Bool) 0-1. Usually 1 means "Bypass is ON" -> "No Sound Processing"
        // UI says "Active" when NOT bypassed.

        if (isBypassed) {
            bypassBtn.innerText = "Bypassed";
            bypassBtn.classList.remove("active");
            document.body.style.opacity = "0.5";
        } else {
            bypassBtn.innerText = "Active";
            bypassBtn.classList.add("active");
            document.body.style.opacity = "1";
        }
    }

    // --- Knob Interaction ---
    knob.addEventListener('mousedown', (e) => {
        if (e.ctrlKey) {
            e.preventDefault();
            resetIntensityToDefault();
            return;
        }

        isDragging = true;
        lastY = e.clientY;
        document.body.style.cursor = 'ns-resize';

        if (intensityState) {
            intensityState.sliderDragStarted();
        }
    });

    window.addEventListener('mousemove', (e) => {
        if (!isDragging) return;
        const delta = lastY - e.clientY;
        lastY = e.clientY;

        // Current value from state if possible, else we track locally?
        // Best to use the JUCE state getter
        let current = DEFAULT_INTENSITY;
        if (intensityState) {
            const val = intensityState.getNormalisedValue();
            current = isNaN(val) ? DEFAULT_INTENSITY : val;
        }

        // Sensitivity
        const newValue = clamp(current + (delta * 0.005), 0, 1);

        if (intensityState) {
            intensityState.setNormalisedValue(newValue);
        }
        updateKnobVis(newValue);
    });

    window.addEventListener('mouseup', () => {
        if (isDragging) {
            isDragging = false;
            document.body.style.cursor = 'default';
            if (intensityState) {
                intensityState.sliderDragEnded();
            }
        }
    });
});
