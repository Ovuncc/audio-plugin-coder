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
    static constexpr const char* expProcessingMode = "exp_processing_mode"; // Osmium / Tight / Chaotic

    // Module bypass controls
    static constexpr const char* expBypassLowTransient = "exp_bypass_low_transient"; // 0 - 1 (Bool)
    static constexpr const char* expBypassLowSaturation = "exp_bypass_low_saturation"; // 0 - 1 (Bool)
    static constexpr const char* expBypassLowLimiter = "exp_bypass_low_limiter"; // 0 - 1 (Bool)
    static constexpr const char* expBypassHighTransient = "exp_bypass_high_transient"; // 0 - 1 (Bool)
    static constexpr const char* expBypassHighSaturation = "exp_bypass_high_saturation"; // 0 - 1 (Bool)
    static constexpr const char* expBypassTightness = "exp_bypass_tightness"; // 0 - 1 (Bool)
    static constexpr const char* expBypassOutputLimiter = "exp_bypass_output_limiter"; // 0 - 1 (Bool)

    // Low band core controls
    static constexpr const char* expLowAttackBoostDb = "exp_low_attack_boost_db"; // 0 - 11.9 dB
    static constexpr const char* expLowAttackTimeMs = "exp_low_attack_time_ms"; // 14 - 30.8 ms
    static constexpr const char* expLowPostCompDb = "exp_low_post_comp_db"; // 0 - 6.9 dB
    static constexpr const char* expLowCompTimeMs = "exp_low_comp_time_ms"; // 24 - 68.5 ms
    static constexpr const char* expLowBodyLiftDb = "exp_low_body_lift_db"; // 0 - 11.9 dB
    static constexpr const char* expLowSatMix = "exp_low_sat_mix"; // 0 - 1.0
    static constexpr const char* expLowSatDrive = "exp_low_sat_drive"; // 1 - 6.0
    static constexpr const char* expLowWaveShaperType = "exp_low_waveshaper_type"; // Choice

    // Low band detector/process controls
    static constexpr const char* expLowFastAttackMs = "exp_low_fast_attack_ms"; // 0.1 - 10 ms
    static constexpr const char* expLowFastReleaseMs = "exp_low_fast_release_ms"; // 1 - 80 ms
    static constexpr const char* expLowSlowReleaseMs = "exp_low_slow_release_ms"; // 20 - 400 ms
    static constexpr const char* expLowAttackGainSmoothMs = "exp_low_attack_gain_smooth_ms"; // 0.1 - 20 ms
    static constexpr const char* expLowCompAttackRatio = "exp_low_comp_attack_ratio"; // 0.05 - 1.0
    static constexpr const char* expLowCompGainSmoothMs = "exp_low_comp_gain_smooth_ms"; // 0.1 - 20 ms
    static constexpr const char* expLowTransientSensitivity = "exp_low_transient_sensitivity"; // 0.25 - 6.0

    // Low band limiter controls
    static constexpr const char* expLowLimiterThresholdDb = "exp_low_limiter_threshold_db"; // -12 - 0 dB
    static constexpr const char* expLowLimiterReleaseMs = "exp_low_limiter_release_ms"; // 10 - 400 ms

    // High band core controls
    static constexpr const char* expHighAttackBoostDb = "exp_high_attack_boost_db"; // 0 - 16.0 dB
    static constexpr const char* expHighAttackTimeMs = "exp_high_attack_time_ms"; // 6 - 12.7 ms
    static constexpr const char* expHighPostCompDb = "exp_high_post_comp_db"; // 0 - 7.5 dB
    static constexpr const char* expHighCompTimeMs = "exp_high_comp_time_ms"; // 14 - 38.1 ms
    static constexpr const char* expHighDriveDb = "exp_high_drive_db"; // 4.0 - 9.5 dB
    static constexpr const char* expHighSatMix = "exp_high_sat_mix"; // 0 - 1.0
    static constexpr const char* expHighSatDrive = "exp_high_sat_drive"; // 1 - 9.0
    static constexpr const char* expHighTrimDb = "exp_high_trim_db"; // -4.0 - 0.0 dB
    static constexpr const char* expHighWaveShaperType = "exp_high_waveshaper_type"; // Choice

    // High band detector/process controls
    static constexpr const char* expHighFastAttackMs = "exp_high_fast_attack_ms"; // 0.1 - 10 ms
    static constexpr const char* expHighFastReleaseMs = "exp_high_fast_release_ms"; // 1 - 80 ms
    static constexpr const char* expHighSlowReleaseMs = "exp_high_slow_release_ms"; // 20 - 400 ms
    static constexpr const char* expHighAttackGainSmoothMs = "exp_high_attack_gain_smooth_ms"; // 0.1 - 20 ms
    static constexpr const char* expHighCompAttackRatio = "exp_high_comp_attack_ratio"; // 0.05 - 1.0
    static constexpr const char* expHighCompGainSmoothMs = "exp_high_comp_gain_smooth_ms"; // 0.1 - 20 ms
    static constexpr const char* expHighTransientSensitivity = "exp_high_transient_sensitivity"; // 0.25 - 6.0

    // Tightness core controls
    static constexpr const char* expTightSustainDepthDb = "exp_tight_sustain_depth_db"; // 0 - 18 dB
    static constexpr const char* expTightSustainAttackMs = "exp_tight_sustain_attack_ms"; // 0.2 - 30 ms
    static constexpr const char* expTightReleaseMs = "exp_tight_release_ms"; // 20 - 220 ms
    static constexpr const char* expTightBellFreqHz = "exp_tight_bell_freq_hz"; // 200 - 600 Hz
    static constexpr const char* expTightBellCutDb = "exp_tight_bell_cut_db"; // 0 - 12 dB
    static constexpr const char* expTightBellQ = "exp_tight_bell_q"; // 0.4 - 1.8
    static constexpr const char* expTightHighShelfCutDb = "exp_tight_high_shelf_cut_db"; // 0 - 12 dB
    static constexpr const char* expTightHighShelfFreqHz = "exp_tight_high_shelf_freq_hz"; // 2500 - 12000 Hz
    static constexpr const char* expTightHighShelfQ = "exp_tight_high_shelf_q"; // 0.3 - 2.0

    // Tightness detector controls
    static constexpr const char* expTightFastAttackMs = "exp_tight_fast_attack_ms"; // 0.1 - 10 ms
    static constexpr const char* expTightFastReleaseMs = "exp_tight_fast_release_ms"; // 1 - 120 ms
    static constexpr const char* expTightSlowAttackMs = "exp_tight_slow_attack_ms"; // 1 - 80 ms
    static constexpr const char* expTightSlowReleaseMs = "exp_tight_slow_release_ms"; // 20 - 600 ms
    static constexpr const char* expTightProgramAttackMs = "exp_tight_program_attack_ms"; // 10 - 600 ms
    static constexpr const char* expTightProgramReleaseMs = "exp_tight_program_release_ms"; // 100 - 4000 ms
    static constexpr const char* expTightTransientSensitivity = "exp_tight_transient_sensitivity"; // 0.25 - 6.0
    static constexpr const char* expTightThresholdOffsetDb = "exp_tight_threshold_offset_db"; // -30 - 0 dB
    static constexpr const char* expTightThresholdRangeDb = "exp_tight_threshold_range_db"; // 3 - 36 dB
    static constexpr const char* expTightThresholdFloorDb = "exp_tight_threshold_floor_db"; // -96 - -12 dB
    static constexpr const char* expTightThresholdCeilDb = "exp_tight_threshold_ceil_db"; // -24 - 0 dB
    static constexpr const char* expTightSustainCurve = "exp_tight_sustain_curve"; // 0.2 - 3.0

    // Output shaping controls
    static constexpr const char* expOutputAutoMakeupDb = "exp_output_auto_makeup_db"; // -18 - +18 dB
    static constexpr const char* expOutputBodyHoldDb = "exp_output_body_hold_db"; // -6 - +6 dB

    // Output limiter controls
    static constexpr const char* expLimiterThresholdDb = "exp_limiter_threshold_db"; // -8 - 0 dB
    static constexpr const char* expLimiterReleaseMs = "exp_limiter_release_ms"; // 10 - 400 ms
}
