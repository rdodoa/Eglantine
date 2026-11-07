// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdlib>
#include <optional>
#include <utility>
#include <vector>

#include "shader_recompiler/frontend/ir/ir_emitter.h"
#include "shader_recompiler/frontend/ir/program.h"

namespace Shader::Optimization {

// TEMPORARY opt-in visual workaround for Ocarina of Time, not an emulator fix.
inline std::optional<size_t> ApplyOotVisualWorkaround(IR::Program& program, u64 title_id,
                                                     u64 shader_hash, bool enabled) {
    if (!enabled || title_id != 0x0400486024DEC000ULL) {
        return std::nullopt;
    }
    const bool fog = shader_hash == 0x780c0536b89fc89eULL && program.stage == Stage::Compute;
    const bool exposure = shader_hash == 0xbded5ce917183214ULL && program.stage == Stage::Fragment;
    if (!fog && !exposure) {
        return std::nullopt;
    }
    const auto has_opcode = [](const IR::Value& value, IR::Opcode opcode) {
        return !value.IsImmediate() && value.InstRecursive()->GetOpcode() == opcode;
    };
    const auto negative_exp = [&](const IR::Value& value) {
        if (has_opcode(value, IR::Opcode::FPNeg32)) {
            return has_opcode(value.InstRecursive()->Arg(0), IR::Opcode::FPExp2);
        }
        if (!has_opcode(value, IR::Opcode::BitCastF32U32)) {
            return false;
        }
        const auto bits = value.InstRecursive()->Arg(0);
        if (!has_opcode(bits, IR::Opcode::BitwiseXor32)) {
            return false;
        }
        const auto* flip = bits.InstRecursive();
        const auto source = flip->Arg(0);
        return flip->Arg(1).IsImmediate() && flip->Arg(1).U32() == 0x80000000U &&
               has_opcode(source, IR::Opcode::BitCastU32F32) &&
               has_opcode(source.InstRecursive()->Arg(0), IR::Opcode::FPExp2);
    };
    // Tunable from nx2_switches.txt: EDEN_NX2_OOT_FOG_SCALE (fog slice distance, default 0.25),
    // EDEN_NX2_OOT_EXPOSURE_SCALE (exposure c[3][0x38], default 0.65) and
    // EDEN_NX2_OOT_BLOOM_EXP_SCALE (the bloom exponent c[3][0x34], default 1: higher values
    // weaken the bloom below 1.0, where most of it is).
    const auto scale_from = [](const char* name, float fallback) {
        const char* const value = std::getenv(name);
        if (value == nullptr) {
            return fallback;
        }
        const float parsed = std::strtof(value, nullptr);
        return parsed > 0.0f && parsed < 16.0f ? parsed : fallback;
    };
    const float fog_scale = scale_from("EDEN_NX2_OOT_FOG_SCALE", 0.25f);
    const float exposure_scale = scale_from("EDEN_NX2_OOT_EXPOSURE_SCALE", 0.65f);
    const float bloom_exp_scale = scale_from("EDEN_NX2_OOT_BLOOM_EXP_SCALE", 1.0f);
    struct Match {
        IR::Block* block;
        IR::Inst* inst;
        float scale;
    };
    std::vector<Match> matches;
    size_t exposure_reads = 0;
    for (IR::Block* block : program.blocks) {
        for (IR::Inst& inst : block->Instructions()) {
            if (fog && inst.GetOpcode() == IR::Opcode::FPAdd32 &&
                ((negative_exp(inst.Arg(0)) && has_opcode(inst.Arg(1), IR::Opcode::FPExp2)) ||
                 (negative_exp(inst.Arg(1)) && has_opcode(inst.Arg(0), IR::Opcode::FPExp2)))) {
                matches.push_back({block, &inst, fog_scale});
            }
            if (exposure && inst.GetOpcode() == IR::Opcode::GetCbufF32 &&
                inst.Arg(0).IsImmediate() && inst.Arg(0).U32() == 3 &&
                inst.Arg(1).IsImmediate()) {
                if (inst.Arg(1).U32() == 0x38) {
                    matches.push_back({block, &inst, exposure_scale});
                    ++exposure_reads;
                } else if (inst.Arg(1).U32() == 0x34 && bloom_exp_scale != 1.0f) {
                    matches.push_back({block, &inst, bloom_exp_scale});
                }
            }
        }
    }
    if ((fog && matches.size() != 1) || (exposure && (exposure_reads == 0 || exposure_reads > 3))) {
        return 0;
    }
    for (const auto& [block, inst, scale] : matches) {
        const auto insertion = block->Instructions().iterator_to(*inst);
        const auto original = block->PrependNewInst(insertion, *inst);
        IR::IREmitter ir{*block, insertion};
        const auto scaled = ir.FPMul(IR::F32{IR::Value{&*original}}, ir.Imm32(scale));
        inst->ReplaceUsesWith(scaled);
    }
    return matches.size();
}

} // namespace Shader::Optimization
