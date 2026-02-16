import * as Juce from "./juce/index.js";

const getSliderState = Juce.getSliderState;
const getComboBoxState = Juce.getComboBoxState;

const defaults = {
  merge: 0.90,
  glue: 0.35,
  pitch_follower: 0.70,
  formant_follower: 0.55,
  focus: 0.50,
  reality: 0.00,
  sibilance: 0.15,
  mix: 1.00,
  output_gain: 0.0,
  mode_index: 0
};

const knobIds = ["merge", "glue", "pitch_follower", "formant_follower", "focus", "reality"];

function clamp(value, min, max) {
  return Math.max(min, Math.min(max, value));
}

function toPercent(value) {
  return Math.round(value * 100) + "%";
}

function toneForValue(value) {
  const t = clamp(value, 0, 1);
  const hue = 205 + ((12 - 205) * t);
  const sat = 34 + (66 * (1 - Math.pow(1 - t, 1.6)));
  const light = 66 + ((50 - 66) * Math.pow(t, 0.9));
  return "hsl(" + hue.toFixed(1) + ", " + sat.toFixed(1) + "%, " + light.toFixed(1) + "%)";
}

function updateKnobVisual(id, normalizedValue) {
  const knob = document.getElementById(id);
  if (!knob) return;

  const normalized = clamp(normalizedValue, 0, 1);
  const sweepAngle = (normalized * 270).toFixed(2) + "deg";
  const indicatorAngle = ((normalized * 270) - 135).toFixed(2) + "deg";

  knob.style.setProperty("--sweep-angle", sweepAngle);
  knob.style.setProperty("--indicator-angle", indicatorAngle);
  knob.style.setProperty("--glow", toneForValue(normalized));

  const valueEl = document.getElementById(id + "-value");
  if (valueEl) valueEl.textContent = toPercent(normalized);
}

function updateSliderVisuals(mixNorm, sibilanceNorm, outputScaled, modeText) {
  const mixValue = document.getElementById("mix-value");
  const sibilanceValue = document.getElementById("sibilance-value");
  const outputValue = document.getElementById("output_gain-value");
  const modeValue = document.getElementById("mode-value");

  if (mixValue) mixValue.textContent = toPercent(clamp(mixNorm, 0, 1));
  if (sibilanceValue) sibilanceValue.textContent = toPercent(clamp(sibilanceNorm, 0, 1));
  if (outputValue) outputValue.textContent = outputScaled.toFixed(1) + " dB";
  if (modeValue) modeValue.textContent = modeText || "Filterbank";
}

function updateRealityWord(realityNorm) {
  const value = clamp(realityNorm, 0, 1);
  const word = document.getElementById("reality-word");
  const prefix = document.getElementById("reality-prefix");
  const sub = document.getElementById("reality-sub");

  if (!word || !prefix || !sub) return;

  const prefixWidth = value <= 0.02 ? 0 : Math.min(2, value * 2.0);
  prefix.style.width = prefixWidth.toFixed(2) + "ch";
  prefix.style.marginRight = prefixWidth > 0 ? "0.08em" : "0";

  const glitch = value <= 0.35 ? 0 : (value - 0.35) / 0.65;
  const px = (glitch * 1.8).toFixed(2);
  word.style.textShadow = "-" + px + "px 0 0 rgba(0,243,255,0.85), " + px + "px 0 0 rgba(255,51,0,0.75)";

  if (glitch > 0.02) {
    word.classList.add("glitching");
  } else {
    word.classList.remove("glitching");
  }

  if (value < 0.2) sub.textContent = "stable mode";
  else if (value < 0.55) sub.textContent = "hybrid mode";
  else sub.textContent = "experimental mode";
}

function createFallbackState(initialNorm, min, max) {
  let norm = clamp(initialNorm, 0, 1);
  const listeners = [];

  return {
    valueChangedEvent: {
      addListener(fn) {
        listeners.push(fn);
      }
    },
    getNormalisedValue() {
      return norm;
    },
    setNormalisedValue(next) {
      norm = clamp(next, 0, 1);
      listeners.forEach((fn) => fn());
    },
    getScaledValue() {
      return min + (max - min) * norm;
    },
    sliderDragStarted() {},
    sliderDragEnded() {}
  };
}

function createFallbackChoiceState(initialIndex, choices) {
  let index = clamp(initialIndex, 0, Math.max(choices.length - 1, 0));
  const valueListeners = [];
  const propListeners = [];

  return {
    properties: { choices: choices.slice() },
    valueChangedEvent: {
      addListener(fn) {
        valueListeners.push(fn);
      }
    },
    propertiesChangedEvent: {
      addListener(fn) {
        propListeners.push(fn);
      }
    },
    getChoiceIndex() {
      return index;
    },
    setChoiceIndex(next) {
      index = clamp(next, 0, Math.max(choices.length - 1, 0));
      valueListeners.forEach((fn) => fn());
    },
    notifyPropertiesChanged() {
      propListeners.forEach((fn) => fn());
    }
  };
}

function bindKnob(id, sliderState) {
  const knob = document.getElementById(id);
  if (!knob || !sliderState) return;

  let dragging = false;
  let startY = 0;
  let startValue = 0;

  function applyByY(clientY) {
    const delta = startY - clientY;
    const sensitivity = 1.0 / 210.0;
    const next = clamp(startValue + (delta * sensitivity), 0, 1);
    sliderState.setNormalisedValue(next);
  }

  function startDrag(clientY) {
    dragging = true;
    startY = clientY;
    startValue = sliderState.getNormalisedValue();
    sliderState.sliderDragStarted();
    document.body.style.cursor = "ns-resize";
  }

  function stopDrag() {
    if (!dragging) return;
    dragging = false;
    sliderState.sliderDragEnded();
    document.body.style.cursor = "default";
  }

  knob.addEventListener("mousedown", function (event) {
    event.preventDefault();
    startDrag(event.clientY);
  });

  window.addEventListener("mousemove", function (event) {
    if (!dragging) return;
    applyByY(event.clientY);
  });

  window.addEventListener("mouseup", function () {
    stopDrag();
  });

  knob.addEventListener("touchstart", function (event) {
    if (!event.touches || !event.touches.length) return;
    event.preventDefault();
    startDrag(event.touches[0].clientY);
  }, { passive: false });

  window.addEventListener("touchmove", function (event) {
    if (!dragging || !event.touches || !event.touches.length) return;
    event.preventDefault();
    applyByY(event.touches[0].clientY);
  }, { passive: false });

  window.addEventListener("touchend", function () {
    stopDrag();
  });

  knob.addEventListener("wheel", function (event) {
    event.preventDefault();
    const current = sliderState.getNormalisedValue();
    const direction = event.deltaY > 0 ? -1 : 1;
    sliderState.setNormalisedValue(clamp(current + (direction * 0.02), 0, 1));
  }, { passive: false });
}

function bindSliders(mixState, sibilanceState, outputState) {
  const mix = document.getElementById("mix");
  const sibilance = document.getElementById("sibilance");
  const output = document.getElementById("output_gain");

  if (mix && mixState) {
    mix.addEventListener("input", function () {
      mixState.setNormalisedValue(parseFloat(mix.value));
    });
  }

  if (sibilance && sibilanceState) {
    sibilance.addEventListener("input", function () {
      sibilanceState.setNormalisedValue(parseFloat(sibilance.value));
    });
  }

  if (output && outputState) {
    output.addEventListener("input", function () {
      const scaled = parseFloat(output.value);
      const normalized = clamp((scaled - (-18.0)) / 36.0, 0, 1);
      outputState.setNormalisedValue(normalized);
    });
  }
}

function bindModeSelector(modeState, onModeUpdated) {
  const modeEl = document.getElementById("mode");
  if (!modeEl || !modeState) return;

  function getChoices() {
    const rawChoices = modeState.properties && Array.isArray(modeState.properties.choices)
      ? modeState.properties.choices
      : ["Filterbank", "Spectral"];

    return rawChoices.length > 0 ? rawChoices : ["Filterbank", "Spectral"];
  }

  function refreshMode() {
    const choices = getChoices();
    const idx = clamp(modeState.getChoiceIndex(), 0, choices.length - 1);

    if (modeEl.options.length !== choices.length) {
      modeEl.innerHTML = "";
      choices.forEach(function (choice, i) {
        const option = document.createElement("option");
        option.value = String(i);
        option.textContent = choice;
        modeEl.appendChild(option);
      });
    }

    modeEl.value = String(idx);
    if (onModeUpdated) onModeUpdated(choices[idx]);
  }

  modeEl.addEventListener("change", function () {
    const idx = parseInt(modeEl.value, 10);
    modeState.setChoiceIndex(Number.isFinite(idx) ? idx : 0);
  });

  modeState.valueChangedEvent.addListener(refreshMode);
  if (modeState.propertiesChangedEvent && typeof modeState.propertiesChangedEvent.addListener === "function") {
    modeState.propertiesChangedEvent.addListener(refreshMode);
  }

  refreshMode();
}

function init() {
  const hasJuceInterop = typeof window.__JUCE__ !== "undefined";

  const controls = {
    merge: hasJuceInterop ? getSliderState("merge") : createFallbackState(defaults.merge, 0, 1),
    glue: hasJuceInterop ? getSliderState("glue") : createFallbackState(defaults.glue, 0, 1),
    pitch_follower: hasJuceInterop ? getSliderState("pitch_follower") : createFallbackState(defaults.pitch_follower, 0, 1),
    formant_follower: hasJuceInterop ? getSliderState("formant_follower") : createFallbackState(defaults.formant_follower, 0, 1),
    focus: hasJuceInterop ? getSliderState("focus") : createFallbackState(defaults.focus, 0, 1),
    reality: hasJuceInterop ? getSliderState("reality") : createFallbackState(defaults.reality, 0, 1),
    sibilance: hasJuceInterop ? getSliderState("sibilance") : createFallbackState(defaults.sibilance, 0, 1),
    mix: hasJuceInterop ? getSliderState("mix") : createFallbackState(defaults.mix, 0, 1),
    output_gain: hasJuceInterop ? getSliderState("output_gain") : createFallbackState((defaults.output_gain + 18.0) / 36.0, -18, 18),
    mode: hasJuceInterop ? getComboBoxState("mode") : createFallbackChoiceState(defaults.mode_index, ["Filterbank", "Spectral"])
  };

  let modeLabel = "Filterbank";

  knobIds.forEach(function (id) {
    controls[id].valueChangedEvent.addListener(function () {
      const value = controls[id].getNormalisedValue();
      updateKnobVisual(id, value);
      if (id === "reality") updateRealityWord(value);
    });

    updateKnobVisual(id, controls[id].getNormalisedValue());
    bindKnob(id, controls[id]);
  });

  controls.sibilance.valueChangedEvent.addListener(function () {
    const sibilanceNorm = controls.sibilance.getNormalisedValue();
    const sibilanceEl = document.getElementById("sibilance");
    if (sibilanceEl) sibilanceEl.value = sibilanceNorm.toFixed(3);

    updateSliderVisuals(
      controls.mix.getNormalisedValue(),
      sibilanceNorm,
      controls.output_gain.getScaledValue(),
      modeLabel
    );
  });

  controls.mix.valueChangedEvent.addListener(function () {
    const mixNorm = controls.mix.getNormalisedValue();
    const mixEl = document.getElementById("mix");
    if (mixEl) mixEl.value = mixNorm.toFixed(3);

    updateSliderVisuals(
      mixNorm,
      controls.sibilance.getNormalisedValue(),
      controls.output_gain.getScaledValue(),
      modeLabel
    );
  });

  controls.output_gain.valueChangedEvent.addListener(function () {
    const outputScaled = controls.output_gain.getScaledValue();
    const outputEl = document.getElementById("output_gain");
    if (outputEl) outputEl.value = outputScaled.toFixed(1);

    updateSliderVisuals(
      controls.mix.getNormalisedValue(),
      controls.sibilance.getNormalisedValue(),
      outputScaled,
      modeLabel
    );
  });

  bindModeSelector(controls.mode, function (label) {
    modeLabel = label || "Filterbank";
    updateSliderVisuals(
      controls.mix.getNormalisedValue(),
      controls.sibilance.getNormalisedValue(),
      controls.output_gain.getScaledValue(),
      modeLabel
    );
  });

  const mixEl = document.getElementById("mix");
  if (mixEl) mixEl.value = controls.mix.getNormalisedValue().toFixed(3);

  const sibilanceEl = document.getElementById("sibilance");
  if (sibilanceEl) sibilanceEl.value = controls.sibilance.getNormalisedValue().toFixed(3);

  const outputEl = document.getElementById("output_gain");
  if (outputEl) outputEl.value = controls.output_gain.getScaledValue().toFixed(1);

  bindSliders(controls.mix, controls.sibilance, controls.output_gain);
  updateSliderVisuals(
    controls.mix.getNormalisedValue(),
    controls.sibilance.getNormalisedValue(),
    controls.output_gain.getScaledValue(),
    modeLabel
  );
  updateRealityWord(controls.reality.getNormalisedValue());

  window.__MORPHANT_UI_READY__ = true;
}

if (document.readyState === "loading") {
  document.addEventListener("DOMContentLoaded", init);
} else {
  init();
}
