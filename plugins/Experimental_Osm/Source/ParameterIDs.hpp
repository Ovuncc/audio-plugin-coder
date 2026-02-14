#pragma once

namespace ParameterIDs
{
    static constexpr const char* intensity = "intensity"; // 0.0 - 1.0 (Range)
    static constexpr const char* outputGain = "output_gain"; // -12.0 - +12.0 dB
    static constexpr const char* bypass = "bypass"; // 0 - 1 (Bool)

    // Experimental mode
    static constexpr const char* expManualMode = "exp_manual_mode"; // 0 - 1 (Bool)
    static constexpr const char* expCrossoverHz = "exp_crossover_hz"; // 80 - 600 Hz

    // Low band controls
    static constexpr const char* expLowAttackBoostDb = "exp_low_attack_boost_db"; // 0 - 12 dB
    static constexpr const char* expLowAttackTimeMs = "exp_low_attack_time_ms"; // 2 - 40 ms
    static constexpr const char* expLowPostCompDb = "exp_low_post_comp_db"; // 0 - 12 dB
    static constexpr const char* expLowCompTimeMs = "exp_low_comp_time_ms"; // 5 - 120 ms
    static constexpr const char* expLowBodyLiftDb = "exp_low_body_lift_db"; // -3 - 12 dB
    static constexpr const char* expLowSatMix = "exp_low_sat_mix"; // 0 - 1
    static constexpr const char* expLowSatDrive = "exp_low_sat_drive"; // 1 - 3

    // High band controls
    static constexpr const char* expHighAttackBoostDb = "exp_high_attack_boost_db"; // 0 - 16 dB
    static constexpr const char* expHighAttackTimeMs = "exp_high_attack_time_ms"; // 2 - 40 ms
    static constexpr const char* expHighPostCompDb = "exp_high_post_comp_db"; // 0 - 12 dB
    static constexpr const char* expHighCompTimeMs = "exp_high_comp_time_ms"; // 5 - 120 ms
    static constexpr const char* expHighDriveDb = "exp_high_drive_db"; // 0 - 24 dB
    static constexpr const char* expHighSatMix = "exp_high_sat_mix"; // 0 - 1
    static constexpr const char* expHighSatDrive = "exp_high_sat_drive"; // 1 - 3
    static constexpr const char* expHighTrimDb = "exp_high_trim_db"; // -24 - 0 dB

    // Output protection controls
    static constexpr const char* expLimiterThresholdDb = "exp_limiter_threshold_db"; // -6 - 0 dB
    static constexpr const char* expLimiterReleaseMs = "exp_limiter_release_ms"; // 10 - 300 ms
    static constexpr const char* expMuteLowBand = "exp_mute_low_band"; // 0 - 1 (Bool)
    static constexpr const char* expMuteHighBand = "exp_mute_high_band"; // 0 - 1 (Bool)
}
