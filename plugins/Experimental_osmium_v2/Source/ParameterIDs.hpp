#pragma once

namespace ParameterIDs
{
    static constexpr const char* intensity = "intensity"; // 0.0 - 1.0 (Range)
    static constexpr const char* outputGain = "output_gain"; // -12.0 - +12.0 dB
    static constexpr const char* bypass = "bypass"; // 0 - 1 (Bool)

    // Experimental manual mode
    static constexpr const char* expManualMode = "exp_manual_mode"; // 0 - 1 (Bool)
    static constexpr const char* expMuteLowBand = "exp_mute_low_band"; // 0 - 1 (Bool)
    static constexpr const char* expMuteHighBand = "exp_mute_high_band"; // 0 - 1 (Bool)

    // Global controls
    static constexpr const char* expCrossoverHz = "exp_crossover_hz"; // 220 - 600 Hz
    static constexpr const char* expOversamplingMode = "exp_oversampling_mode"; // Off / 4x / 8x

    // Low band controls
    static constexpr const char* expLowAttackBoostDb = "exp_low_attack_boost_db"; // 0 - 11.9 dB
    static constexpr const char* expLowAttackTimeMs = "exp_low_attack_time_ms"; // 14 - 30.8 ms
    static constexpr const char* expLowPostCompDb = "exp_low_post_comp_db"; // 0 - 6.9 dB
    static constexpr const char* expLowCompTimeMs = "exp_low_comp_time_ms"; // 24 - 68.5 ms
    static constexpr const char* expLowBodyLiftDb = "exp_low_body_lift_db"; // 0 - 11.9 dB
    static constexpr const char* expLowSatMix = "exp_low_sat_mix"; // 0 - 1.0
    static constexpr const char* expLowSatDrive = "exp_low_sat_drive"; // 1 - 5.0
    static constexpr const char* expLowWaveShaperType = "exp_low_waveshaper_type"; // Choice

    // High band controls
    static constexpr const char* expHighAttackBoostDb = "exp_high_attack_boost_db"; // 0 - 15.9 dB
    static constexpr const char* expHighAttackTimeMs = "exp_high_attack_time_ms"; // 6 - 12.7 ms
    static constexpr const char* expHighPostCompDb = "exp_high_post_comp_db"; // 0 - 7.4 dB
    static constexpr const char* expHighCompTimeMs = "exp_high_comp_time_ms"; // 14 - 38.1 ms
    static constexpr const char* expHighDriveDb = "exp_high_drive_db"; // 0 - 9.5 dB
    static constexpr const char* expHighSatMix = "exp_high_sat_mix"; // 0 - 1.0
    static constexpr const char* expHighSatDrive = "exp_high_sat_drive"; // 1 - 5.0
    static constexpr const char* expHighTrimDb = "exp_high_trim_db"; // -4.0 - -0.1 dB
    static constexpr const char* expHighWaveShaperType = "exp_high_waveshaper_type"; // Choice

    // Tightness controls
    static constexpr const char* expTightSustainDepthDb = "exp_tight_sustain_depth_db"; // 0 - 18 dB
    static constexpr const char* expTightReleaseMs = "exp_tight_release_ms"; // 20 - 220 ms
    static constexpr const char* expTightBellFreqHz = "exp_tight_bell_freq_hz"; // 200 - 600 Hz
    static constexpr const char* expTightBellCutDb = "exp_tight_bell_cut_db"; // 0 - 12 dB
    static constexpr const char* expTightBellQ = "exp_tight_bell_q"; // 0.4 - 1.8
    static constexpr const char* expTightHighShelfCutDb = "exp_tight_high_shelf_cut_db"; // 0 - 12 dB
    static constexpr const char* expTightHighShelfFreqHz = "exp_tight_high_shelf_freq_hz"; // 2500 - 12000 Hz

    // Output limiter controls
    static constexpr const char* expLimiterThresholdDb = "exp_limiter_threshold_db"; // -0.1 dB (fixed)
    static constexpr const char* expLimiterReleaseMs = "exp_limiter_release_ms"; // 80 ms (fixed)
}
