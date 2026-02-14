import * as Juce from "./juce/index.js";

// Parameter Handling
const getSliderState = Juce.getSliderState;
const getToggleState = Juce.getToggleState;

// Define parameters from spec
const PARAM_INTENSITY = "intensity"; // 0.0 - 1.0
const PARAM_OUTPUT = "output_gain";  // -12.0 - 12.0
const PARAM_BYPASS = "bypass";       // 0 - 1

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

    // --- JUCE Binding ---

    // 1. Intensity Knob (Custom UI -> JUCE)
    // We need to fetch the initial state and listen for updates
    if (window.__JUCE__) {
        const intensityState = getSliderState(PARAM_INTENSITY);

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
                // Fallback to 0 if still not initialized
                updateKnobVis(0);
            }
        }, 100);
    } else {
        // Fallback if JUCE not available
        updateKnobVis(0);
    }

    // 2. Output Slider (Input Range -> JUCE)
    if (window.__JUCE__) {
        const outputState = getSliderState(PARAM_OUTPUT);

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
                outputSlider.value = 0;
                updateOutputDisplay(0);
            }
        }, 100);

        // Bind input event to JUCE
        outputSlider.addEventListener('input', () => {
            const val = parseFloat(outputSlider.value);
            // Convert slider value to normalised 0-1 range
            const normalised = (val - (-12)) / (12 - (-12));
            outputState.setNormalisedValue(normalised);
            updateOutputDisplay(val); // Local update for responsiveness
        });

        outputSlider.addEventListener('change', () => {
            const val = parseFloat(outputSlider.value);
            const normalised = (val - (-12)) / (12 - (-12));
            outputState.setNormalisedValue(normalised);
        });
    } else {
        outputSlider.value = 0;
        updateOutputDisplay(0);
    }

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
        // Value is 0.0 - 1.0
        const percent = Math.round(value * 100);
        valueDisplay.innerText = percent + "%";

        const degrees = value * 360;

        // Color interpolation
        let color = "var(--cyan)";
        if (value > 0.6) color = "var(--orange)";

        glowRing.style.background = `conic-gradient(${color} ${degrees}deg, transparent ${degrees}deg)`;
        glowRing.style.boxShadow = `0 0 ${20 + (value * 30)}px ${color}`;
        indicator.style.transform = `translateX(-50%) rotate(${degrees}deg)`;

        valueDisplay.style.color = value > 0 ? "white" : "var(--text-muted)";
        valueDisplay.style.textShadow = value > 0 ? `0 0 10px ${color}` : "none";
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
        isDragging = true;
        lastY = e.clientY;
        document.body.style.cursor = 'ns-resize';

        if (window.__JUCE__) {
            getSliderState(PARAM_INTENSITY).sliderDragStarted();
        }
    });

    window.addEventListener('mousemove', (e) => {
        if (!isDragging) return;
        const delta = lastY - e.clientY;
        lastY = e.clientY;

        // Current value from state if possible, else we track locally?
        // Best to use the JUCE state getter
        let current = 0;
        if (window.__JUCE__) {
            const val = getSliderState(PARAM_INTENSITY).getNormalisedValue();
            current = isNaN(val) ? 0 : val;
        }

        // Sensitivity
        let newValue = current + (delta * 0.005);
        newValue = Math.max(0, Math.min(1, newValue));

        if (window.__JUCE__) {
            getSliderState(PARAM_INTENSITY).setNormalisedValue(newValue);
            // We rely on the listener to update UI, or do optimistic update:
            updateKnobVis(newValue);
        }
    });

    window.addEventListener('mouseup', () => {
        if (isDragging) {
            isDragging = false;
            document.body.style.cursor = 'default';
            if (window.__JUCE__) {
                getSliderState(PARAM_INTENSITY).sliderDragEnded();
            }
        }
    });
});
