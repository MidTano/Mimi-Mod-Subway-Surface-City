#include "inline_hook.hpp"

#include <sys/mman.h>
#include <unistd.h>

#include <cstdint>
#include <cstring>

namespace inline_hook {

#if defined(__aarch64__)

namespace {

constexpr std::uint32_t kLDR_X16_PC8 = 0x58000050u;
constexpr std::uint32_t kBR_X16      = 0xD61F0200u;

constexpr std::uint32_t kADRP_MASK   = 0x9F000000u;
constexpr std::uint32_t kADRP_VAL    = 0x90000000u;
constexpr std::uint32_t kADR_MASK    = 0x9F000000u;
constexpr std::uint32_t kADR_VAL     = 0x10000000u;

constexpr std::uint32_t kB_MASK      = 0x7C000000u;
constexpr std::uint32_t kB_VAL       = 0x14000000u;

constexpr std::uint32_t kLDR_LIT_MASK= 0x3B000000u;
constexpr std::uint32_t kLDR_LIT_VAL = 0x18000000u;

constexpr std::uint32_t kCB_MASK     = 0x7E000000u;
constexpr std::uint32_t kCB_VAL      = 0x34000000u;

constexpr std::uint32_t kTB_MASK     = 0x7E000000u;
constexpr std::uint32_t kTB_VAL      = 0x36000000u;

constexpr std::uint32_t kBcond_MASK  = 0xFF000010u;
constexpr std::uint32_t kBcond_VAL   = 0x54000000u;

void flush_cache(void* addr, std::size_t size) {
    __builtin___clear_cache(reinterpret_cast<char*>(addr),
                            reinterpret_cast<char*>(addr) + size);
}

bool make_page_writable(void* addr, std::size_t span) {
    const long ps = sysconf(_SC_PAGESIZE);
    std::uintptr_t a = reinterpret_cast<std::uintptr_t>(addr);
    std::uintptr_t page = a & ~static_cast<std::uintptr_t>(ps - 1);
    std::uintptr_t end  = a + span;
    std::size_t len = static_cast<std::size_t>(end - page);
    if (len < static_cast<std::size_t>(ps)) len = static_cast<std::size_t>(ps);
    return mprotect(reinterpret_cast<void*>(page), len,
                    PROT_READ | PROT_WRITE | PROT_EXEC) == 0;
}

bool is_adrp(std::uint32_t insn) {
    return (insn & kADRP_MASK) == kADRP_VAL;
}

bool is_adr(std::uint32_t insn) {
    return (insn & kADR_MASK) == kADR_VAL;
}

bool is_branch_imm(std::uint32_t insn) {
    return (insn & kB_MASK) == kB_VAL;
}

bool is_ldr_literal(std::uint32_t insn) {
    return (insn & kLDR_LIT_MASK) == kLDR_LIT_VAL;
}

bool is_cb(std::uint32_t insn) {
    return (insn & kCB_MASK) == kCB_VAL;
}

bool is_tb(std::uint32_t insn) {
    return (insn & kTB_MASK) == kTB_VAL;
}

bool is_bcond(std::uint32_t insn) {
    return (insn & kBcond_MASK) == kBcond_VAL;
}

std::uint64_t decode_adrp_target(std::uint32_t insn, std::uint64_t pc) {
    const std::uint64_t immlo = (insn >> 29) & 0x3;
    const std::uint64_t immhi = (insn >> 5)  & 0x7FFFF;
    std::int64_t imm = static_cast<std::int64_t>((immhi << 2) | immlo);
    if (imm & (1LL << 20)) imm |= ~((1LL << 21) - 1);
    const std::uint64_t page = pc & ~static_cast<std::uint64_t>(0xFFF);
    return page + (imm << 12);
}

std::uint64_t decode_adr_target(std::uint32_t insn, std::uint64_t pc) {
    const std::uint64_t immlo = (insn >> 29) & 0x3;
    const std::uint64_t immhi = (insn >> 5)  & 0x7FFFF;
    std::int64_t imm = static_cast<std::int64_t>((immhi << 2) | immlo);
    if (imm & (1LL << 20)) imm |= ~((1LL << 21) - 1);
    return pc + static_cast<std::uint64_t>(imm);
}

std::uint32_t movz_x(std::uint32_t rd, std::uint16_t imm16, std::uint32_t hw) {
    return 0xD2800000u | (hw << 21) | (static_cast<std::uint32_t>(imm16) << 5) | (rd & 0x1F);
}

std::uint32_t movk_x(std::uint32_t rd, std::uint16_t imm16, std::uint32_t hw) {
    return 0xF2800000u | (hw << 21) | (static_cast<std::uint32_t>(imm16) << 5) | (rd & 0x1F);
}

int relocate_instruction(std::uint32_t insn, std::uint64_t src_pc,
                         std::uint32_t* out, int out_capacity) {
    if (is_adrp(insn)) {
        const std::uint32_t rd = insn & 0x1F;
        const std::uint64_t abs_addr = decode_adrp_target(insn, src_pc);
        if (out_capacity < 4) return -1;
        out[0] = movz_x(rd, static_cast<std::uint16_t>((abs_addr >>  0) & 0xFFFF), 0);
        out[1] = movk_x(rd, static_cast<std::uint16_t>((abs_addr >> 16) & 0xFFFF), 1);
        out[2] = movk_x(rd, static_cast<std::uint16_t>((abs_addr >> 32) & 0xFFFF), 2);
        out[3] = movk_x(rd, static_cast<std::uint16_t>((abs_addr >> 48) & 0xFFFF), 3);
        return 4;
    }
    if (is_adr(insn)) {
        const std::uint32_t rd = insn & 0x1F;
        const std::uint64_t abs_addr = decode_adr_target(insn, src_pc);
        if (out_capacity < 4) return -1;
        out[0] = movz_x(rd, static_cast<std::uint16_t>((abs_addr >>  0) & 0xFFFF), 0);
        out[1] = movk_x(rd, static_cast<std::uint16_t>((abs_addr >> 16) & 0xFFFF), 1);
        out[2] = movk_x(rd, static_cast<std::uint16_t>((abs_addr >> 32) & 0xFFFF), 2);
        out[3] = movk_x(rd, static_cast<std::uint16_t>((abs_addr >> 48) & 0xFFFF), 3);
        return 4;
    }
    if (is_branch_imm(insn)) {
        std::int32_t imm26 = static_cast<std::int32_t>(insn & 0x03FFFFFFu);
        if (imm26 & (1 << 25)) imm26 |= ~((1 << 26) - 1);
        const std::int64_t offset = static_cast<std::int64_t>(imm26) * 4;
        const std::uint64_t target = src_pc + static_cast<std::uint64_t>(offset);
        const bool is_link = ((insn >> 31) & 1u) != 0u;
        if (out_capacity < 5) return -1;
        out[0] = movz_x(16, static_cast<std::uint16_t>((target >>  0) & 0xFFFF), 0);
        out[1] = movk_x(16, static_cast<std::uint16_t>((target >> 16) & 0xFFFF), 1);
        out[2] = movk_x(16, static_cast<std::uint16_t>((target >> 32) & 0xFFFF), 2);
        out[3] = movk_x(16, static_cast<std::uint16_t>((target >> 48) & 0xFFFF), 3);
        out[4] = is_link ? 0xD63F0200u : 0xD61F0200u;
        return 5;
    }
    if (is_ldr_literal(insn) || is_cb(insn) || is_tb(insn) || is_bcond(insn)) {
        return -1;
    }
    if (out_capacity < 1) return -1;
    out[0] = insn;
    return 1;
}

}

void* install(void* target, void* replacement) {
    if (target == nullptr || replacement == nullptr) return nullptr;

    constexpr int kOriginalInsns = 4;
    constexpr int kMaxExpandedInsns = kOriginalInsns * 4 + 4;

    std::uint32_t original[kOriginalInsns];
    std::memcpy(original, target, sizeof(original));

    std::uint32_t relocated[kMaxExpandedInsns];
    int relocated_count = 0;
    const std::uint64_t target_pc_base = reinterpret_cast<std::uint64_t>(target);
    for (int i = 0; i < kOriginalInsns; ++i) {
        const std::uint64_t src_pc = target_pc_base + static_cast<std::uint64_t>(i) * 4;
        const int written = relocate_instruction(
            original[i], src_pc,
            relocated + relocated_count,
            kMaxExpandedInsns - relocated_count);
        if (written < 0) {
            return nullptr;
        }
        relocated_count += written;
    }

    const long ps = sysconf(_SC_PAGESIZE);
    void* tramp = mmap(nullptr, static_cast<std::size_t>(ps),
                       PROT_READ | PROT_WRITE | PROT_EXEC,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (tramp == MAP_FAILED) {
        return nullptr;
    }

    std::uint32_t* tramp_w = reinterpret_cast<std::uint32_t*>(tramp);
    for (int i = 0; i < relocated_count; ++i) tramp_w[i] = relocated[i];

    const std::uint64_t return_addr =
        reinterpret_cast<std::uint64_t>(target) + kOriginalInsns * 4;
    tramp_w[relocated_count + 0] = kLDR_X16_PC8;
    tramp_w[relocated_count + 1] = kBR_X16;
    std::memcpy(tramp_w + relocated_count + 2, &return_addr, sizeof(return_addr));

    if (!make_page_writable(target, kOriginalInsns * 4)) {
        munmap(tramp, static_cast<std::size_t>(ps));
        return nullptr;
    }

    std::uint32_t* target_w = reinterpret_cast<std::uint32_t*>(target);
    target_w[0] = kLDR_X16_PC8;
    target_w[1] = kBR_X16;
    const std::uint64_t repl = reinterpret_cast<std::uint64_t>(replacement);
    std::memcpy(target_w + 2, &repl, sizeof(repl));

    flush_cache(target, kOriginalInsns * 4);
    flush_cache(tramp, static_cast<std::size_t>(ps));

    return tramp;
}

#else

void* install(void* /*target*/, void* /*replacement*/) {
    return nullptr;
}

#endif

}
