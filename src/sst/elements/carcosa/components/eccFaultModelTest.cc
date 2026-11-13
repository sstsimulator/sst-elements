#include "sst_config.h"
#include "sst/elements/carcosa/components/eccFaultModelTest.h"

#include <limits>
#include <utility>

using namespace SST;
using namespace SST::Carcosa;

void EccFaultModelTest::require(bool condition, const char* message) {
    if (!condition)
        out_->fatal(CALL_INFO, -1, "EccFaultModelTest: FAIL %s\n", message);
}

void EccFaultModelTest::setup() {
    EccGuard::setup();
    testResidentFootprints();
    testResidentDecode();
    out_->output("EccFaultModelTest: PASS resident footprints and decode.\n");
}

void EccFaultModelTest::testResidentFootprints() {
    const uint64_t original_base = resident_addr_start_;
    const uint64_t original_len = resident_addr_len_;
    const EccPolicyEntry original_policy = policy_.uniform();
    resident_mode_mix_ = false;
    resident_permanent_fraction_ = 1.0;
    resident_row_bytes_ = 8192;
    resident_bank_rows_ = 8;

    auto reset = [&]() {
        resident_faults_.clear();
        resident_mask_.clear();
    };
    auto check = [&]() {
        require(resident_faults_.size() == 1, "one materialized object per birth");
        const auto& fault = resident_faults_.front();
        unsigned bits = 0;
        uint32_t chips = 0;
        uint64_t word = 0;
        bool one_word = true;
        for (const auto& line : fault.line_bits) {
            for (unsigned byte = 0; byte < 64; ++byte) {
                const unsigned mask = line.second[byte];
                if (mask == 0) continue;
                const uint64_t addr = line.first + byte;
                require(addr >= resident_addr_start_
                            && addr - resident_addr_start_ < resident_addr_len_,
                        "every resident bit stays within the byte window");
                if (bits == 0) word = addr / 8;
                one_word = one_word && word == addr / 8;
                bits += __builtin_popcount(mask);
                if (mask & 0x0f) chips |= 1u << ((byte % 16) * 2);
                if (mask & 0xf0) chips |= 1u << ((byte % 16) * 2 + 1);
            }
        }
        require(bits != 0, "every counted birth has an actual in-window footprint");
        if (fault.mode == FaultMode::SingleCell)
            require(bits == 1, "cell birth contains exactly one bit");
        else if (fault.mode == FaultMode::SingleWord)
            require(bits == 2 && one_word, "word birth contains two distinct aligned-word bits");
        else
            require(__builtin_popcount(chips) == 1, "correlated footprint stays on one x4 chip");
    };

    // Include one-byte windows, a cacheline boundary, partial endpoints,
    // a sub-row window, and the final byte of the address space.
    const std::pair<uint64_t, uint64_t> windows[] = {
        {0x4001, 1}, {0x403f, 2}, {0x4001, 129}, {0x4000, 64},
        {std::numeric_limits<uint64_t>::max(), 1}
    };
    for (const auto& window : windows) {
        resident_addr_start_ = window.first;
        resident_addr_len_ = window.second;
        for (unsigned mode = 0; mode < static_cast<unsigned>(FaultMode::Count); ++mode) {
            resident_mode_fixed_ = static_cast<FaultMode>(mode);
            residentRng_.seed(12345);
            for (unsigned birth = 0; birth < 64; ++birth) {
                reset();
                materializeResidentFault();
                check();
            }
        }
    }

    // Nine rows include both one full bank and a partial final bank. Repeat
    // with a one-line final row to also check valid per-row column sampling.
    resident_addr_start_ = 0x4000;
    resident_mode_fixed_ = FaultMode::SingleBank;
    for (uint64_t tail : {uint64_t(8192), uint64_t(64)}) {
        resident_addr_len_ = 8 * 8192 + tail;
        residentRng_.seed(12345);
        bool saw_first_bank = false, saw_last_bank = false;
        for (unsigned birth = 0; birth < 128; ++birth) {
            reset();
            materializeResidentFault();
            check();
            for (const auto& line : resident_faults_.front().line_bits) {
                if (line.first < resident_addr_start_ + 8 * 8192) saw_first_bank = true;
                else saw_last_bank = true;
            }
        }
        require(saw_first_bank && saw_last_bank, "full and partial banks can both receive faults");
    }

    // A scheme change must preserve a paired run's complete fault sequence.
    resident_addr_start_ = 0x4001;
    resident_addr_len_ = 3 * 8192 + 17;
    resident_mode_mix_ = true;
    std::vector<ResidentFault> reference;
    for (EccScheme scheme : {EccScheme::SECDED_64, EccScheme::CHIPKILL_x4}) {
        EccPolicyEntry entry = original_policy;
        entry.scheme = scheme;
        policy_.setUniform(entry);
        residentRng_.seed(12345);
        reset();
        for (unsigned birth = 0; birth < 64; ++birth) materializeResidentFault();
        if (reference.empty()) reference = resident_faults_;
        else {
            require(reference.size() == resident_faults_.size(), "paired birth counts match");
            for (size_t i = 0; i < reference.size(); ++i)
                require(reference[i].mode == resident_faults_[i].mode
                            && reference[i].permanent == resident_faults_[i].permanent
                            && reference[i].line_bits == resident_faults_[i].line_bits,
                        "paired schemes see identical physical faults");
        }
    }

    reset();
    resident_addr_start_ = original_base;
    resident_addr_len_ = original_len;
    policy_.setUniform(original_policy);
}

void EccFaultModelTest::testResidentDecode() {
    using namespace SST::MemHierarchy;
    EccPolicyEntry entry = policy_.uniform();
    entry.scheme = EccScheme::SECDED_64;
    entry.correctable_latency_ps = 11;
    entry.due_latency_ps = 22;
    entry.escape_latency_ps = 33;
    policy_.setUniform(entry);

    auto response = [&](uint64_t address, unsigned bytes) {
        MemEvent event(getName(), address, address & ~63ULL, Command::GetSResp, bytes);
        event.setFlag(MemEvent::F_NONCACHEABLE);
        std::vector<uint8_t> payload(bytes, 0xA5);
        event.setPayload(payload);
        return event;
    };
    auto unchanged = [&](MemEvent& event) {
        return event.getPayload() == std::vector<uint8_t>(event.getPayloadSize(), 0xA5);
    };

    // One bit in each of two aligned words must remain two correctable words,
    // even when one unaligned request returns portions of both words.
    resident_mask_.clear();
    resident_mask_[0x4000][5] = 1;
    resident_mask_[0x4000][10] = 1;
    auto split = response(0x4004, 8);
    auto draw = drawFaultResident(&split, 8, EccScheme::SECDED_64);
    require(draw.per_word_errors == std::vector<unsigned>({1, 1}),
            "unaligned access preserves aligned protection words");
    require(applyPolicy(&split) == 11 && unchanged(split),
            "separate correctable words do not leak faults");

    // Faults outside the requested byte still participate in its ECC decode.
    resident_mask_.clear();
    resident_mask_[0x4000][5] = 1;
    resident_mask_[0x4000][6] = 1;
    auto partial = response(0x4005, 1);
    require(applyPolicy(&partial) == 22 && partial.getPayload()[0] == (0xA5 ^ 1),
            "partial read detects a double-fault word and flips only its returned fault");
    auto outside = response(0x4004, 1);
    require(applyPolicy(&outside) == 22 && unchanged(outside),
            "off-payload faults never trigger random payload corruption");

    // Exact flips must account for the partial first word and preserve a
    // correctable neighboring word, for both escape and DUE paths.
    resident_mask_.clear();
    resident_mask_[0x4000][5] = 7;
    resident_mask_[0x4000][10] = 1;
    auto escaped = response(0x4004, 8);
    std::vector<uint8_t> expected(8, 0xA5);
    expected[1] ^= 7;
    require(applyPolicy(&escaped) == 33 && escaped.getPayload() == expected,
            "escape flips stay in their aligned word");
    resident_mask_[0x4000][5] = 1;
    resident_mask_[0x4000][10] = 3;
    auto due = response(0x4004, 8);
    expected.assign(8, 0xA5);
    expected[6] ^= 3;
    require(applyPolicy(&due) == 22 && due.getPayload() == expected,
            "DUE flips in a later word use the payload offset");

    resident_mask_.clear();
    resident_mask_[0x4000][15] = 1;
    resident_mask_[0x4000][16] = 1;
    auto chips = response(0x400f, 2);
    draw = drawFaultResident(&chips, 2, EccScheme::CHIPKILL_x4);
    require(draw.per_word_errors == std::vector<unsigned>({1, 1})
                && draw.per_word_chip_errors[0][30] == 1
                && draw.per_word_chip_errors[1][0] == 1,
            "Chipkill attribution uses aligned word and chip coordinates");
    resident_mask_.clear();
}
