#pragma once

namespace ParameterIDs
{
    static constexpr const char* intensity = "intensity"; // 0.0 - 1.0 (Range)
    static constexpr const char* outputGain = "output_gain"; // -12.0 - +12.0 dB
    static constexpr const char* bypass = "bypass"; // 0 - 1 (Bool)
    static constexpr const char* oversamplingMode = "oversampling_mode"; // Off / 4x / 8x
    static constexpr const char* processingMode = "processing_mode"; // Osmium / Tight / Chaotic
}
