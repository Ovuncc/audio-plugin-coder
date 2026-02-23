import * as Juce from "./juce/index.js";

const rangeParamIds = [
    "panic",
    "bandwidth",
    "collapse",
    "jitter",
    "pitch_drift",
    "formant_melt",
    "broadcast",
    "smear",
    "seed",
    "mix",
    "output_gain",
];

const choiceParamIds = ["mode"];
const sliderParamIds = [...choiceParamIds, ...rangeParamIds];
const moduleToggleIds = [
    "codec_bypass",
    "packet_bypass",
    "identity_bypass",
    "dynamics_bypass",
    "temporal_bypass",
];
const toggleParamIds = ["bypass", ...moduleToggleIds];

const controls = Object.fromEntries(
    sliderParamIds.map((id) => [id, document.getElementById(id)])
);

const dom = Object.fromEntries(
    [
        "bypass",
        "alarm-chip",
        "shell",
        "scope-state",
        "seed-label",
        "severity-text",
        "mode-text",
        "jitter-text",
        "fault-log",
        "band-meter",
        "artifact-scope",
        "codec_bypass",
        "packet_bypass",
        "identity_bypass",
        "dynamics_bypass",
        "temporal_bypass",
    ].map((id) => [id, document.getElementById(id)])
);

const readoutNode = (id) => document.getElementById(`value-${id}`);

const hasBackend = !!(window.__JUCE__ && window.__JUCE__.backend);
const sliderStates = {};
const toggleStates = {};

const ctx = dom["artifact-scope"].getContext("2d");

const clamp = (value, min, max) => Math.max(min, Math.min(max, value));

function toNumber(control) {
    if (!control) return 0;
    return Number(control.value);
}

function updateReadout(id) {
    const node = readoutNode(id);
    const control = controls[id];

    if (!node || !control) return;

    const value = Number(control.value);

    switch (id) {
        case "bandwidth": {
            const kbps = 6 + (value / 100) * 42;
            node.textContent = `${kbps.toFixed(1)} kbps`;
            break;
        }
        case "jitter": {
            const ms = (value / 100) * 40;
            node.textContent = `${ms.toFixed(1)} ms`;
            break;
        }
        case "output_gain": {
            node.textContent = `${value >= 0 ? "+" : ""}${value.toFixed(1)} dB`;
            break;
        }
        case "seed": {
            const seed = Math.round(value);
            node.textContent = String(seed);
            dom["seed-label"].textContent = `Seed ${seed}`;
            break;
        }
        default:
            node.textContent = `${Math.round(value)}%`;
            break;
    }
}

function severityScore() {
    const panic = toNumber(controls.panic);
    const collapse = toNumber(controls.collapse);
    const jitter = toNumber(controls.jitter);
    const drift = toNumber(controls.pitch_drift);
    const smear = toNumber(controls.smear);
    return panic * 0.35 + collapse * 0.25 + jitter * 0.2 + drift * 0.1 + smear * 0.1;
}

function severityName(score) {
    if (score >= 70) return "CRITICAL";
    if (score >= 42) return "ELEVATED";
    return "LOW";
}

function updateToggleVisual(toggleId, isPressed) {
    const button = dom[toggleId];
    if (!button) return;

    button.setAttribute("aria-pressed", isPressed ? "true" : "false");

    if (toggleId === "bypass") {
        button.textContent = isPressed ? "Bypass On" : "Bypass";
        return;
    }

    button.textContent = isPressed ? "Bypassed" : "Active";
}

function updateStatus() {
    const score = severityScore();
    const level = severityName(score);

    dom["severity-text"].textContent = level;
    dom["mode-text"].textContent = controls.mode.options[controls.mode.selectedIndex].text.toUpperCase();
    dom["jitter-text"].textContent = `${((toNumber(controls.jitter) / 100) * 40).toFixed(1)} MS`;
    dom["scope-state"].textContent = `SEVERITY ${level}`;

    dom["alarm-chip"].classList.remove("warning", "critical");
    dom.shell.classList.remove("alarm");

    if (level === "ELEVATED") {
        dom["alarm-chip"].classList.add("warning");
        dom["alarm-chip"].textContent = "Elevated";
    } else if (level === "CRITICAL") {
        dom["alarm-chip"].classList.add("critical");
        dom["alarm-chip"].textContent = "Critical";
        dom.shell.classList.add("alarm");
    } else {
        dom["alarm-chip"].textContent = "Nominal";
    }
}

function ensureMeterBars() {
    if (dom["band-meter"].children.length > 0) return;

    for (let i = 0; i < 16; i += 1) {
        const bar = document.createElement("div");
        bar.className = "meter-bar";
        bar.style.height = "8px";
        dom["band-meter"].appendChild(bar);
    }
}

function addFaultLine() {
    const level = severityName(severityScore());
    const modeName = controls.mode.options[controls.mode.selectedIndex].text;

    const pool = {
        LOW: [
            `[${modeName}] Signal path is stable and pretending to be responsible.`,
            "Jitter clock is sipping coffee and behaving itself.",
            "Latency is calm and filing paperwork in triplicate.",
        ],
        ELEVATED: [
            "Codec tried to compress charisma and ran out of bits.",
            "Jitter buffer switched to jazz mode. No two bars are alike.",
            "Formants are drifting like they forgot where they parked.",
            "Limiter is yelling, but in a supportive way.",
        ],
        CRITICAL: [
            "Frame 42 escaped containment and started a podcast.",
            "Pitch tracker has entered a spiritual journey.",
            "Codebook collapsed into interpretive dance.",
            "Noise filed a complaint about additional noise.",
            "Emergency mode engaged: have you tried turning reality off and on?",
        ],
    };

    const line = document.createElement("li");
    const messages = pool[level];
    line.textContent = `${new Date().toLocaleTimeString()}  ${messages[Math.floor(Math.random() * messages.length)]}`;

    if (level === "ELEVATED") line.classList.add("warning");
    if (level === "CRITICAL") line.classList.add("critical");

    dom["fault-log"].prepend(line);

    while (dom["fault-log"].children.length > 6) {
        dom["fault-log"].removeChild(dom["fault-log"].lastChild);
    }
}

function updateBandMeter(tick) {
    const score = severityScore();
    const bars = Array.from(dom["band-meter"].children);

    bars.forEach((bar, i) => {
        const energy = 0.15 + Math.abs(Math.sin((tick * 0.03) + i * 0.34));
        const chaos = 0.2 + score / 100;
        const height = 8 + Math.floor((energy * 46 * chaos) + Math.random() * 8);
        bar.style.height = `${height}px`;

        bar.classList.remove("warning", "critical");
        if (height > 40) bar.classList.add("warning");
        if (height > 52) {
            bar.classList.remove("warning");
            bar.classList.add("critical");
        }
    });
}

let tick = 0;
function drawScope() {
    const w = dom["artifact-scope"].width;
    const h = dom["artifact-scope"].height;
    const panic = toNumber(controls.panic) / 100;
    const collapse = toNumber(controls.collapse) / 100;
    const jitter = toNumber(controls.jitter) / 100;
    const smear = toNumber(controls.smear) / 100;
    const drift = toNumber(controls.pitch_drift) / 100;

    ctx.clearRect(0, 0, w, h);
    ctx.fillStyle = "#111216";
    ctx.fillRect(0, 0, w, h);

    ctx.strokeStyle = "rgba(255, 255, 255, 0.10)";
    ctx.lineWidth = 1;
    for (let y = 20; y < h; y += 20) {
        ctx.beginPath();
        ctx.moveTo(0, y);
        ctx.lineTo(w, y);
        ctx.stroke();
    }

    const baseFreq = 0.011 + drift * 0.035;
    const amp = 32 + panic * 64;
    const jitterAmp = 2 + collapse * 22;

    ctx.beginPath();
    ctx.lineWidth = 2;
    ctx.strokeStyle = panic > 0.6 ? "#ff3300" : "#00f3ff";

    for (let x = 0; x < w; x += 2) {
        const jitterNoise = (Math.random() - 0.5) * jitterAmp;
        const wave = Math.sin((x * baseFreq) + tick * 0.015) * amp;
        const comb = Math.sin((x * 0.045) + tick * 0.03) * (12 + smear * 26);
        const y = h * 0.5 + wave * 0.35 + comb * 0.2 + jitterNoise;
        if (x === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
    }
    ctx.stroke();

    if (jitter > 0.12) {
        const bursts = 1 + Math.floor(jitter * 10);
        const streakAlpha = 0.12 + jitter * 0.18;
        for (let i = 0; i < bursts; i += 1) {
            const x = Math.floor(Math.random() * w);
            const width = 4 + Math.floor(Math.random() * (10 + jitter * 16));
            ctx.fillStyle = `rgba(255, 51, 0, ${streakAlpha.toFixed(3)})`;
            ctx.fillRect(x, 0, width, h);
        }
    }

    if (smear > 0.5) {
        ctx.fillStyle = "rgba(0, 243, 255, 0.16)";
        for (let i = 0; i < 7; i += 1) {
            const y = Math.random() * h;
            const width = 20 + Math.random() * 120;
            ctx.fillRect(Math.random() * w, y, width, 1.3);
        }
    }
}

function animate() {
    tick += 1;
    drawScope();
    if ((tick % 2) === 0) {
        updateBandMeter(tick);
    }
    requestAnimationFrame(animate);
}

function setSliderScaledValue(paramId, scaled) {
    const state = sliderStates[paramId];
    const control = controls[paramId];
    if (!state || !control) return;

    const startRaw = Number(state.properties.start);
    const endRaw = Number(state.properties.end);
    const start = Number.isFinite(startRaw) ? startRaw : 0;
    const end = Number.isFinite(endRaw) ? endRaw : 1;
    const span = Math.max(1.0e-6, end - start);

    if (choiceParamIds.includes(paramId)) {
        const clampedScaled = clamp(scaled, start, end);
        const normalisedChoice = clamp((clampedScaled - start) / span, 0, 1);
        state.setNormalisedValue(normalisedChoice);
        return;
    }

    const uiMinRaw = Number(control.min ?? 0);
    const uiMaxRaw = Number(control.max ?? 1);
    const uiMin = Number.isFinite(uiMinRaw) ? uiMinRaw : 0;
    const uiMax = Number.isFinite(uiMaxRaw) ? uiMaxRaw : 1;
    const uiSpan = Math.max(1.0e-6, uiMax - uiMin);
    const uiT = clamp((scaled - uiMin) / uiSpan, 0, 1);
    const scaledForState = start + uiT * span;
    const normalised = clamp((scaledForState - start) / span, 0, 1);
    state.setNormalisedValue(normalised);
}

function syncControlFromState(paramId) {
    const state = sliderStates[paramId];
    const control = controls[paramId];
    if (!state || !control) return;

    const scaled = state.getScaledValue();

    if (choiceParamIds.includes(paramId)) {
        const index = clamp(Math.round(scaled), 0, control.options.length - 1);
        control.selectedIndex = index;
    } else {
        const uiMinRaw = Number(control.min ?? 0);
        const uiMaxRaw = Number(control.max ?? 1);
        const uiMin = Number.isFinite(uiMinRaw) ? uiMinRaw : 0;
        const uiMax = Number.isFinite(uiMaxRaw) ? uiMaxRaw : 1;
        const uiSpan = Math.max(1.0e-6, uiMax - uiMin);

        const startRaw = Number(state.properties.start);
        const endRaw = Number(state.properties.end);
        const start = Number.isFinite(startRaw) ? startRaw : 0;
        const end = Number.isFinite(endRaw) ? endRaw : 1;
        const span = Math.max(1.0e-6, end - start);

        const t = clamp((scaled - start) / span, 0, 1);
        const uiValue = uiMin + t * uiSpan;
        control.value = String(uiValue);
        updateReadout(paramId);
    }
}

function syncFromBackend() {
    if (!hasBackend) return;

    sliderParamIds.forEach((id) => syncControlFromState(id));

    toggleParamIds.forEach((id) => {
        const state = toggleStates[id];
        if (state) updateToggleVisual(id, Boolean(state.getValue()));
    });

    updateStatus();
}

function bindInputs() {
    rangeParamIds.forEach((id) => {
        const input = controls[id];
        if (!input) return;

        input.addEventListener("pointerdown", () => {
            const state = sliderStates[id];
            if (state) state.sliderDragStarted();
        });

        input.addEventListener("input", () => {
            if (sliderStates[id]) {
                setSliderScaledValue(id, Number(input.value));
            }
            updateReadout(id);
            updateStatus();
        });

        const dragEnd = () => {
            const state = sliderStates[id];
            if (state) state.sliderDragEnded();
        };
        input.addEventListener("pointerup", dragEnd);
        input.addEventListener("change", dragEnd);

        updateReadout(id);
    });

    choiceParamIds.forEach((id) => {
        const input = controls[id];
        if (!input) return;

        input.addEventListener("change", () => {
            const scaled = input.selectedIndex;
            if (sliderStates[id]) {
                setSliderScaledValue(id, scaled);
            }
            updateStatus();
        });
    });

    toggleParamIds.forEach((id) => {
        const button = dom[id];
        if (!button) return;

        button.addEventListener("click", () => {
            const state = toggleStates[id];
            if (state) {
                state.setValue(!state.getValue());
                updateToggleVisual(id, Boolean(state.getValue()));
            } else {
                const pressed = button.getAttribute("aria-pressed") === "true";
                updateToggleVisual(id, !pressed);
            }
        });
    });
}

function bindBackend() {
    if (!hasBackend) return;

    sliderParamIds.forEach((id) => {
        sliderStates[id] = Juce.getSliderState(id);
        sliderStates[id].valueChangedEvent.addListener(() => {
            syncControlFromState(id);
            updateStatus();
        });
    });

    toggleParamIds.forEach((id) => {
        const state = Juce.getToggleState(id);
        toggleStates[id] = state;
        state.valueChangedEvent.addListener(() => {
            updateToggleVisual(id, Boolean(state.getValue()));
        });
    });

    // Initial updates from backend arrive asynchronously.
    window.setTimeout(syncFromBackend, 70);
    window.setTimeout(syncFromBackend, 220);
}

function init() {
    ensureMeterBars();
    bindBackend();
    bindInputs();

    toggleParamIds.forEach((id) => updateToggleVisual(id, false));
    updateStatus();
    addFaultLine();
    window.setInterval(addFaultLine, 1700);
    animate();
}

if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", init);
} else {
    init();
}
