// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <string_view>

#include "common/settings.h"

namespace Settings {

inline constexpr u64 MarioKartWorldProgramId = 0x0400C3F00006E000ULL;

inline void SetMarioKartWorldRecommendedSettings(Values& settings, bool global) {
    const auto session = [global](auto& setting, auto value) {
        setting.SetGlobal(global);
        setting.SetValue(value);
    };
    session(settings.renderer_backend, RendererBackend::Vulkan);
    session(settings.resolution_setup, ResolutionSetup::Res1X);
    session(settings.gpu_accuracy, GpuAccuracy::Low);
    session(settings.use_asynchronous_shaders, true);
    session(settings.astc_recompression, AstcRecompression::Bc3);
    session(settings.vram_usage_mode, VramUsageMode::Conservative);
    session(settings.use_reactive_flushing, false);
    session(settings.vsync_mode, VSyncMode::FifoRelaxed);
    session(settings.use_docked_mode, ConsoleMode::Handheld);
    session(settings.scaling_filter, ScalingFilter::Bilinear);
    session(settings.anti_aliasing, AntiAliasing::Fxaa);
}

inline bool ApplyMarioKartWorldPreset(Values& settings, u64 program_id, bool accepted) {
    if (!accepted || program_id != MarioKartWorldProgramId) {
        return false;
    }
    SetMarioKartWorldRecommendedSettings(settings, false);
    return true;
}

// ---- Hortensia smart presets ----------------------------------------------------------------
// Applied at boot, for the session only (the saved configuration is not changed), when the
// "optimized settings" option is on. Values come from what the forks Hortensia gathers recommend.

inline constexpr u64 OcarinaOfTimeProgramId = 0x0400486024DEC000ULL;
inline constexpr u64 DonkeyKongBananzaProgramId = 0x040074A01BF12000ULL;
inline constexpr u64 RaidersProgramId = 0x0400380020056000ULL;
inline constexpr u64 PokopiaProgramId = 0x0400A3A01EE4C000ULL;

enum class GpuVendor { Unknown, Nvidia, Amd, Intel, Apple, Qualcomm };

/// The vendor of a Vulkan device from its name ("AMD Radeon RX 6700 XT", "NVIDIA GeForce ...").
inline GpuVendor GpuVendorFromName(std::string_view name) {
    const auto has = [name](std::string_view word) {
        if (word.size() > name.size()) {
            return false;
        }
        for (size_t i = 0; i + word.size() <= name.size(); ++i) {
            bool equal = true;
            for (size_t j = 0; j < word.size() && equal; ++j) {
                const char a = name[i + j];
                const char lower = a >= 'A' && a <= 'Z' ? static_cast<char>(a - 'A' + 'a') : a;
                equal = lower == word[j];
            }
            if (equal) {
                return true;
            }
        }
        return false;
    };
    if (has("nvidia") || has("geforce") || has("quadro") || has("rtx")) {
        return GpuVendor::Nvidia;
    }
    if (has("amd") || has("radeon") || has("radv")) {
        return GpuVendor::Amd;
    }
    if (has("intel")) {
        return GpuVendor::Intel;
    }
    if (has("apple")) {
        return GpuVendor::Apple;
    }
    if (has("adreno") || has("qualcomm")) {
        return GpuVendor::Qualcomm;
    }
    return GpuVendor::Unknown;
}

inline const char* GpuVendorName(GpuVendor vendor) {
    switch (vendor) {
    case GpuVendor::Nvidia:
        return "NVIDIA";
    case GpuVendor::Amd:
        return "AMD";
    case GpuVendor::Intel:
        return "Intel";
    case GpuVendor::Apple:
        return "Apple";
    case GpuVendor::Qualcomm:
        return "Qualcomm";
    default:
        return "unknown";
    }
}

/// Applies the optimized settings of a supported game for this session. Returns a short summary
/// for the log, or an empty string when the game has no preset.
inline std::string ApplyHortensiaSmartPreset(Values& settings, u64 program_id, GpuVendor vendor) {
    const auto session = [](auto& setting, auto value) {
        setting.SetGlobal(false);
        setting.SetValue(value);
    };
    // Integrated GPUs (Intel, most Apple and Qualcomm) render at half resolution.
    const bool integrated = vendor == GpuVendor::Intel || vendor == GpuVendor::Apple ||
                            vendor == GpuVendor::Qualcomm;
    const auto resolution = integrated ? ResolutionSetup::Res1_2X : ResolutionSetup::Res1X;
    switch (program_id) {
    case MarioKartWorldProgramId:
        // Lichii's recommended Mario Kart World settings, plus SmileyTatsu's dynamic FPS.
        SetMarioKartWorldRecommendedSettings(settings, false);
        session(settings.resolution_setup, resolution);
        session(settings.mkw_dynamic_fps, true);
        // Selevet's tuned Mario Kart World settings (55.5 fps median at 1x on a Ryzen 5 5500 and
        // an RTX 3050): Unsafe CPU and DMA accuracy, Immediate GPU fences, no memory operation
        // syncs.
        session(settings.cpu_accuracy, CpuAccuracy::Unsafe);
        session(settings.dma_accuracy, DmaAccuracy::Unsafe);
        session(settings.gpu_fence_behavior, GpuFenceBehavior::Immediate);
        session(settings.sync_memory_operations, false);
        if (!integrated) {
            // Selevet: BC3 recompression darkened the track's ground; discrete GPUs have the
            // memory for uncompressed ASTC.
            session(settings.astc_recompression, AstcRecompression::Uncompressed);
        }
        return integrated ? "Mario Kart World: Lichii's recommended settings, dynamic FPS, half "
                            "resolution"
                          : "Mario Kart World: Lichii's recommended settings, dynamic FPS, "
                            "uncompressed ASTC";
    case OcarinaOfTimeProgramId:
        session(settings.renderer_backend, RendererBackend::Vulkan);
        session(settings.resolution_setup, resolution);
        session(settings.use_asynchronous_shaders, true);
        return "Ocarina of Time: Vulkan, asynchronous shaders";
    case DonkeyKongBananzaProgramId:
        session(settings.renderer_backend, RendererBackend::Vulkan);
        session(settings.resolution_setup, resolution);
        session(settings.use_asynchronous_shaders, true);
        session(settings.dk_dynamic_fps, true);
        // Orchid (measured on AMD): with GPU accuracy High the split GPU thread stays off and the
        // game slows down below 30 fps; Low without reactive flushing keeps it at full speed. With
        // the Boost GPU clock it stays at 1080p and mostly at interval 1.
        session(settings.gpu_accuracy, GpuAccuracy::Low);
        session(settings.use_reactive_flushing, false);
        session(settings.gpu_clock, GpuClock::Boost);
        return "Donkey Kong Bananza: Vulkan, asynchronous shaders, dynamic FPS, GPU accuracy Fast, "
               "reactive flushing off, Boost GPU clock";
    case RaidersProgramId:
        session(settings.renderer_backend, RendererBackend::Vulkan);
        session(settings.resolution_setup, resolution);
        session(settings.use_asynchronous_shaders, true);
        return "Splatoon Raiders: Vulkan, asynchronous shaders";
    case PokopiaProgramId:
        session(settings.renderer_backend, RendererBackend::Vulkan);
        session(settings.resolution_setup, resolution);
        session(settings.use_asynchronous_shaders, true);
        return "Pokemon Pokopia: Vulkan, asynchronous shaders";
    default:
        if (!CurrentProgramIsSwitch2()) {
            return {};
        }
        // Any other Switch 2 title (Pokemon Pokopia and others).
        session(settings.renderer_backend, RendererBackend::Vulkan);
        session(settings.use_asynchronous_shaders, true);
        if (integrated) {
            session(settings.resolution_setup, resolution);
        }
        return "Switch 2 title: Vulkan, asynchronous shaders";
    }
}

} // namespace Settings
