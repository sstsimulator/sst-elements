// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_ELEMENTS_CARCOSA_ECC_SCHEME_H
#define SST_ELEMENTS_CARCOSA_ECC_SCHEME_H

#include <cstdint>
#include <string>
#include <vector>

namespace SST {
namespace Carcosa {

enum class EccOutcome : uint8_t {
    Clean                    = 0,
    Correctable              = 1,
    DetectableUncorrectable  = 2,
    SilentEscape             = 3,
};

inline const char* eccOutcomeName(EccOutcome o) {
    switch (o) {
    case EccOutcome::Clean:                   return "clean";
    case EccOutcome::Correctable:             return "correctable";
    case EccOutcome::DetectableUncorrectable: return "due";
    case EccOutcome::SilentEscape:            return "escape";
    }
    return "unknown";
}

enum class EccScheme : uint8_t {
    NONE        = 0,
    SECDED_64   = 1,
    CHIPKILL_x4 = 2,
};

inline const char* eccSchemeName(EccScheme s) {
    switch (s) {
    case EccScheme::NONE:        return "none";
    case EccScheme::SECDED_64:   return "secded";
    case EccScheme::CHIPKILL_x4: return "chipkill";
    }
    return "unknown";
}

inline bool eccSchemeFromString(const std::string& s, EccScheme& out) {
    if (s == "none" || s == "NONE")           { out = EccScheme::NONE;        return true; }
    if (s == "secded" || s == "SECDED" ||
        s == "secded_64" || s == "SECDED_64") { out = EccScheme::SECDED_64;   return true; }
    if (s == "chipkill" || s == "CHIPKILL" ||
        s == "chipkill_x4")                   { out = EccScheme::CHIPKILL_x4; return true; }
    return false;
}

// Per-word ECC: line outcome = worst word (Clean<Correctable<DUE<Escape).
// Model assumes independent-bit Bernoulli; tight for BER <= kEccBerTightUpperBound
// (1e-3). Chipkill "<=4/<=8 bits" rule is optimistic for scattered errors.

constexpr double kEccBerTightUpperBound = 1.0e-3;

// Data bytes per ECC protection word, by scheme. 0 means "no word concept";
// the line is treated as a single bag of bits (used for EccScheme::NONE and
// as a fallback if the scheme is unknown).
inline uint32_t eccWordBytes(EccScheme scheme) {
    switch (scheme) {
    case EccScheme::NONE:        return 0;
    case EccScheme::SECDED_64:   return 8;   // 64-bit data per SECDED word
    case EccScheme::CHIPKILL_x4: return 16;  // 128-bit data per chipkill word
    }
    return 0;
}

// Classify a single ECC word given its bit-error count.
// For CHIPKILL_x4 this uses optimistic per-bit thresholds; prefer
// classifyEccWordChipAware when per-chip error counts are available.
inline EccOutcome classifyEccWord(unsigned word_errors, EccScheme scheme) {
    if (word_errors == 0) return EccOutcome::Clean;
    switch (scheme) {
    case EccScheme::NONE:
        return EccOutcome::SilentEscape;
    case EccScheme::SECDED_64:
        if (word_errors == 1) return EccOutcome::Correctable;
        if (word_errors == 2) return EccOutcome::DetectableUncorrectable;
        return EccOutcome::SilentEscape;
    case EccScheme::CHIPKILL_x4:
        if (word_errors <= 4) return EccOutcome::Correctable;
        if (word_errors <= 8) return EccOutcome::DetectableUncorrectable;
        return EccOutcome::SilentEscape;
    }
    return EccOutcome::SilentEscape;
}

// Number of x4 chips per ECC word. Only meaningful for CHIPKILL_x4.
inline unsigned chipsPerEccWord(EccScheme scheme) {
    if (scheme == EccScheme::CHIPKILL_x4)
        return eccWordBytes(scheme) * 8 / 4;  // 128 data bits / 4 bits per chip = 32
    return 0;
}

// Chip-aware CHIPKILL_x4: corrects 1-chip, detects 2-chip, escapes on 3+.
// Falls back to classifyEccWord for non-chipkill schemes.
inline EccOutcome classifyEccWordChipAware(
        const std::vector<uint8_t>& chip_error_counts, EccScheme scheme) {
    if (scheme != EccScheme::CHIPKILL_x4)
        return classifyEccWord(
            [&]() -> unsigned {
                unsigned s = 0;
                for (auto c : chip_error_counts) s += c;
                return s;
            }(), scheme);
    unsigned affected = 0;
    for (auto c : chip_error_counts)
        if (c > 0) ++affected;
    if (affected == 0) return EccOutcome::Clean;
    if (affected == 1) return EccOutcome::Correctable;
    if (affected == 2) return EccOutcome::DetectableUncorrectable;
    return EccOutcome::SilentEscape;
}

// Result of aggregating per-word outcomes into a single line outcome.
struct EccLineOutcome {
    EccOutcome outcome     = EccOutcome::Clean;
    unsigned   escape_bits = 0;   // bits-on-the-wire from escaping words only
    unsigned   total_errors = 0;  // sum across all words (book-keeping)
    std::vector<uint32_t> escape_words; // indices of words that escaped
    std::vector<uint32_t> due_words;    // indices of words that went DUE
};

// Aggregate per-word outcomes: line = worst word; escape_bits sum only over
// SilentEscape words. DUE words are uncorrectable — DropFrame aborts,
// LatencyOnly forwards poisoned bits (EccGuard flips them into the payload).
inline EccLineOutcome aggregateLineOutcome(const std::vector<unsigned>& per_word_errors,
                                           EccScheme scheme) {
    EccLineOutcome r;
    for (size_t w = 0; w < per_word_errors.size(); ++w) {
        unsigned e = per_word_errors[w];
        r.total_errors += e;
        EccOutcome o = classifyEccWord(e, scheme);
        if (static_cast<uint8_t>(o) > static_cast<uint8_t>(r.outcome)) r.outcome = o;
        if (o == EccOutcome::SilentEscape) {
            r.escape_bits += e;
            r.escape_words.push_back(static_cast<uint32_t>(w));
        } else if (o == EccOutcome::DetectableUncorrectable) {
            r.due_words.push_back(static_cast<uint32_t>(w));
        }
    }
    return r;
}

// Chip-aware variant for CHIPKILL_x4. Uses per-chip error distribution
// instead of per-bit thresholds for more accurate classification.
inline EccLineOutcome aggregateLineOutcomeChipAware(
        const std::vector<unsigned>& per_word_errors,
        const std::vector<std::vector<uint8_t>>& per_word_chip_errors,
        EccScheme scheme) {
    if (scheme != EccScheme::CHIPKILL_x4
        || per_word_chip_errors.size() != per_word_errors.size()) {
        return aggregateLineOutcome(per_word_errors, scheme);
    }
    EccLineOutcome r;
    for (size_t w = 0; w < per_word_errors.size(); ++w) {
        r.total_errors += per_word_errors[w];
        EccOutcome o = classifyEccWordChipAware(per_word_chip_errors[w], scheme);
        if (static_cast<uint8_t>(o) > static_cast<uint8_t>(r.outcome)) r.outcome = o;
        if (o == EccOutcome::SilentEscape) {
            r.escape_bits += per_word_errors[w];
            r.escape_words.push_back(static_cast<uint32_t>(w));
        } else if (o == EccOutcome::DetectableUncorrectable) {
            r.due_words.push_back(static_cast<uint32_t>(w));
        }
    }
    return r;
}

// Back-compat: single "total errors on the line" count treated as one word.
// Over-counts DUE/Escape when errors span multiple words — prefer per-word
// classifyEccWord + aggregateLineOutcome.
inline EccOutcome classifyEccOutcome(unsigned num_bit_errors,
                                     uint32_t /*payload_bytes*/,
                                     EccScheme scheme) {
    return classifyEccWord(num_bit_errors, scheme);
}

} // namespace Carcosa
} // namespace SST

#endif /* SST_ELEMENTS_CARCOSA_ECC_SCHEME_H */
