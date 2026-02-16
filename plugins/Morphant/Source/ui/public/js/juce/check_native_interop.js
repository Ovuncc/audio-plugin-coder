/**
 * Checks if the native JUCE interop is available.
 * Returns true if available, false otherwise.
 */
export function isNativeInteropAvailable() {
    return window.__JUCE__ !== undefined && window.__JUCE__.backend !== undefined;
}

/**
 * Validates that the JUCE frontend library is loaded correctly.
 */
export function validateJuceInterop() {
    if (!isNativeInteropAvailable()) {
        console.warn("JUCE Native Interop not available. Running in browser mode?");
        return false;
    }
    return true;
}
