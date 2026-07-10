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

#include "sst_config.h"
#include "sst/elements/carcosa/components/eccGuard.h"
#include "sst/elements/carcosa/components/eccModelMath.h"
#include <algorithm>
#include <cinttypes>
#include <cstring>
#include <numeric>
#include <sstream>
#include <vector>

using namespace SST;
using namespace SST::MemHierarchy;
using namespace SST::Carcosa;

namespace {

constexpr int kModeCount = static_cast<int>(EccGuard::FaultMode::Count);

// Sridharan ASPLOS'15 Table 4 + Schroeder SIGMETRICS'09 dominant-mode mix.
// Order: cell, word, row, column, bank, device.
constexpr double kDefaultModeWeights[kModeCount] = {
    0.55, 0.15, 0.10, 0.08, 0.07, 0.05
};

// Approx bit errors per fault mode across the line. Correlated modes deposit
// the whole count into one ECC word (spillover if needed); SingleCell scatters.
// Defaults are conservative under SECDED_64 / CHIPKILL_x4.
constexpr unsigned kFaultModeBitsLow[kModeCount]  = { 1, 2, 4, 4, 8, 32 };
constexpr unsigned kFaultModeBitsHigh[kModeCount] = { 1, 2, 8, 8, 16, 64 };

const char* faultModeName(EccGuard::FaultMode m) {
    switch (m) {
        case EccGuard::FaultMode::SingleCell:    return "single_cell";
        case EccGuard::FaultMode::SingleWord:    return "single_word";
        case EccGuard::FaultMode::SingleRow:     return "single_row";
        case EccGuard::FaultMode::SingleColumn:  return "single_column";
        case EccGuard::FaultMode::SingleBank:    return "single_bank";
        case EccGuard::FaultMode::SingleDevice:  return "single_device";
        default:                                  return "unknown";
    }
}

bool parseModeWeightsCsv(const std::string& csv, double out[kModeCount]) {
    std::stringstream ss(csv);
    std::string tok;
    int idx = 0;
    while (std::getline(ss, tok, ':')) {
        if (idx >= kModeCount) return false;
        try {
            out[idx++] = std::stod(tok);
        } catch (...) {
            return false;
        }
    }
    return idx == kModeCount;
}

EccGuard::FaultModel parseFaultModel(const std::string& s) {
    if (s == "jedec_mix" || s == "jedec" || s == "JEDEC_MIX") return EccGuard::FaultModel::JedecMix;
    if (s == "campaign"  || s == "CAMPAIGN")                  return EccGuard::FaultModel::Campaign;
    if (s == "resident"  || s == "RESIDENT")                  return EccGuard::FaultModel::Resident;
    return EccGuard::FaultModel::Poisson;
}

// Parse campaign mode name into FaultMode; unknown -> SingleRow (same
// defaulting as jedec_mix). Crash only on empty/garbage upstream.
EccGuard::FaultMode parseCampaignMode(const std::string& s) {
    if (s == "cell"   || s == "single_cell"   || s == "SingleCell")   return EccGuard::FaultMode::SingleCell;
    if (s == "word"   || s == "single_word"   || s == "SingleWord")   return EccGuard::FaultMode::SingleWord;
    if (s == "row"    || s == "single_row"    || s == "SingleRow")    return EccGuard::FaultMode::SingleRow;
    if (s == "column" || s == "single_column" || s == "SingleColumn") return EccGuard::FaultMode::SingleColumn;
    if (s == "bank"   || s == "single_bank"   || s == "SingleBank")   return EccGuard::FaultMode::SingleBank;
    if (s == "device" || s == "single_device" || s == "SingleDevice") return EccGuard::FaultMode::SingleDevice;
    if (s == "multi_chip" || s == "MULTI_CHIP") return EccGuard::FaultMode::SingleWord;
    return EccGuard::FaultMode::SingleRow;
}

bool isMultiChipCampaignAlias(const std::string& s) {
    return s == "multi_chip" || s == "MULTI_CHIP";
}

// Resolve campaign_target_kernel to the canonical currentKernelName key.
// Empty means every kernel (matches the empty name published between kernels).
std::string resolveCampaignKernel(const std::string& raw) {
    if (raw.empty() || raw == "any" || raw == "ANY" || raw == "*"
        || raw == "-1") {
        return std::string();
    }
    return raw;
}

EccGuard::PayloadDtype parseDtype(const std::string& s) {
    if (s == "bf16" || s == "BF16") return EccGuard::PayloadDtype::Bf16;
    if (s == "fp8"  || s == "FP8")  return EccGuard::PayloadDtype::Fp8;
    if (s == "int8" || s == "INT8") return EccGuard::PayloadDtype::Int8;
    return EccGuard::PayloadDtype::Bytes;
}

EccGuard::DueAction parseDueAction(const std::string& s) {
    if (s == "drop_frame" || s == "drop" || s == "DROP_FRAME") return EccGuard::DueAction::DropFrame;
    return EccGuard::DueAction::LatencyOnly;
}

const char* dtypeName(EccGuard::PayloadDtype d) {
    switch (d) {
    case EccGuard::PayloadDtype::Bytes: return "bytes";
    case EccGuard::PayloadDtype::Bf16:  return "bf16";
    case EccGuard::PayloadDtype::Fp8:   return "fp8";
    case EccGuard::PayloadDtype::Int8:  return "int8";
    }
    return "unknown";
}

// Returns true if the given bit (0-indexed inside its element) is "high blast"
// for the dtype (sign bit or top exponent bit). Bit 0 is LSB of the element.
bool isHighBlastBit(EccGuard::PayloadDtype dtype, unsigned bit_in_element) {
    switch (dtype) {
    case EccGuard::PayloadDtype::Bf16: {
        // bf16: [15] sign, [14:7] exponent, [6:0] mantissa.
        if (bit_in_element == 15)        return true;       // sign
        if (bit_in_element >= 13 && bit_in_element <= 14) return true; // top 2 exp bits
        return false;
    }
    case EccGuard::PayloadDtype::Fp8: {
        // E4M3-style fp8: [7] sign, [6:3] exponent, [2:0] mantissa.
        if (bit_in_element == 7)         return true;
        if (bit_in_element == 6)         return true;
        return false;
    }
    case EccGuard::PayloadDtype::Int8: {
        // Two's-complement int8: bit 7 sign.
        return bit_in_element == 7;
    }
    case EccGuard::PayloadDtype::Bytes:
    default:
        return false;
    }
}

unsigned dtypeBytes(EccGuard::PayloadDtype d) {
    switch (d) {
    case EccGuard::PayloadDtype::Bf16:  return 2;
    case EccGuard::PayloadDtype::Fp8:
    case EccGuard::PayloadDtype::Int8:  return 1;
    case EccGuard::PayloadDtype::Bytes:
    default:                             return 1;
    }
}

} // namespace

EccGuard::EccGuard(ComponentId_t id, Params& params) : Component(id) {
    requireLibrary("memHierarchy");

    out_ = new Output("", 1, 0, Output::STDOUT);
    verbose_ = params.find<bool>("verbose", false);
    test_total_min_ = params.find<int64_t>("test_total_min", -1);
    test_total_max_ = params.find<int64_t>("test_total_max", -1);
    test_clean_min_ = params.find<int64_t>("test_clean_min", -1);
    test_clean_max_ = params.find<int64_t>("test_clean_max", -1);
    test_correctable_min_ = params.find<int64_t>("test_correctable_min", -1);
    test_correctable_max_ = params.find<int64_t>("test_correctable_max", -1);
    test_due_min_ = params.find<int64_t>("test_due_min", -1);
    test_due_max_ = params.find<int64_t>("test_due_max", -1);
    test_escape_min_ = params.find<int64_t>("test_escape_min", -1);
    test_escape_max_ = params.find<int64_t>("test_escape_max", -1);
    test_resident_born_min_ = params.find<int64_t>("test_resident_born_min", -1);
    test_resident_born_max_ = params.find<int64_t>("test_resident_born_max", -1);

    state_key_            = params.find<std::string>("state_key", "");
    applyOnResponsesOnly_ = params.find<bool>("apply_on_responses_only", true);

    EccPolicyEntry uniform;
    uniform.inherits_uniform = false;
    std::string scheme_str = params.find<std::string>("ecc_scheme", "none");
    if (!eccSchemeFromString(scheme_str, uniform.scheme)) {
        out_->fatal(CALL_INFO, -1,
            "EccGuard: unknown ecc_scheme '%s'. Use 'none', 'secded', or 'chipkill'.\n",
            scheme_str.c_str());
    }
    uniform.ber                    = params.find<double>("ber", 0.0);
    uniform.correctable_latency_ps = params.find<uint64_t>("correctable_latency_ps", 0);
    uniform.due_latency_ps         = params.find<uint64_t>("due_latency_ps",         0);
    uniform.escape_latency_ps      = params.find<uint64_t>("escape_latency_ps",      0);
    if (uniform.ber < 0.0 || uniform.ber > 1.0) {
        out_->fatal(CALL_INFO, -1,
            "EccGuard: ber=%g out of range [0.0, 1.0].\n", uniform.ber);
    }
    policy_.setUniform(uniform);

    std::string ks_csv = params.find<std::string>("kernel_policy", "");
    if (!ks_csv.empty()) {
        std::vector<std::string> errors;
        int parsed = policy_.parseCsv(ks_csv, errors);
        for (auto& e : errors) out_->output("EccGuard: %s\n", e.c_str());
        if (verbose_) {
            out_->output("EccGuard: parsed %d kernel/region policy override(s).\n", parsed);
        }
    }

    // Phase 2: fault model + dtype-aware flips + DUE action.
    fault_model_   = parseFaultModel(params.find<std::string>("fault_model",   "poisson"));
    payload_dtype_ = parseDtype     (params.find<std::string>("payload_dtype", "bytes"));
    due_action_    = parseDueAction (params.find<std::string>("due_action",    "latency_only"));

    std::string mw_csv = params.find<std::string>("fault_mode_weights", "");
    if (!mw_csv.empty()) {
        if (!parseModeWeightsCsv(mw_csv, mode_weights_)) {
            out_->fatal(CALL_INFO, -1,
                "EccGuard: malformed fault_mode_weights '%s'; need 6 colon-separated doubles "
                "(cell:word:row:column:bank:device).\n", mw_csv.c_str());
        }
    } else {
        for (int i = 0; i < kModeCount; ++i) mode_weights_[i] = kDefaultModeWeights[i];
    }
    // Normalize. (parseCsv lets users pass arbitrary positive numbers.)
    {
        double sum = 0.0;
        for (int i = 0; i < kModeCount; ++i) {
            if (mode_weights_[i] < 0.0) mode_weights_[i] = 0.0;
            sum += mode_weights_[i];
        }
        if (sum <= 0.0) {
            out_->fatal(CALL_INFO, -1,
                "EccGuard: fault_mode_weights sum to zero; provide at least one positive weight.\n");
        }
        for (int i = 0; i < kModeCount; ++i) mode_weights_[i] /= sum;
    }

    fault_event_rate_ = params.find<double>("fault_event_rate", 0.0);

    // Campaign-mode parameters. These are inert unless fault_model_ == Campaign.
    {
        std::string raw_target = params.find<std::string>("campaign_target_kernel", "any");
        campaign_target_kernel_name_ = resolveCampaignKernel(raw_target);
        if (campaign_target_kernel_name_.empty()
            && raw_target != "any" && raw_target != "ANY"
            && raw_target != "-1" && !raw_target.empty()) {
            out_->output("EccGuard WARNING: campaign_target_kernel='%s' did not "
                          "resolve to a known kernel; defaulting to 'any'.\n",
                          raw_target.c_str());
        }
        campaign_event_budget_  = params.find<uint64_t>("campaign_event_budget", 0);
        campaign_event_rate_    = params.find<double>  ("campaign_event_rate",   0.0);
        campaign_max_per_kernel_entry_ =
            params.find<uint64_t>("campaign_max_events_per_kernel_entry", 0);
        campaign_errors_fixed_ = params.find<unsigned>("campaign_errors_fixed", 0);
        std::string raw_cmode = params.find<std::string>("campaign_mode", "row");
        campaign_force_multi_chip_ =
            params.find<bool>("campaign_force_multi_chip", false)
            || isMultiChipCampaignAlias(raw_cmode);
        campaign_mode_ = parseCampaignMode(raw_cmode);
        addr_filter_region_ = params.find<std::string>("addr_filter_region", "");
        addr_filter_len_    = params.find<uint64_t>("addr_filter_len", 0);
        inject_addr_start_  = params.find<uint64_t>("inject_addr_start", 0);
        inject_addr_len_    = params.find<uint64_t>("inject_addr_len", 0);
        if (fault_model_ == FaultModel::Campaign && verbose_) {
            out_->output("EccGuard: campaign mode active: target=%s mode=%d budget=%" PRIu64
                          " rate=%.3e\n",
                          campaign_target_kernel_name_.empty()
                              ? "any"
                              : campaign_target_kernel_name_.c_str(),
                          static_cast<int>(campaign_mode_),
                          campaign_event_budget_,
                          campaign_event_rate_);
        }
        if (fault_model_ == FaultModel::Campaign && campaign_event_budget_ == 0) {
            out_->output("EccGuard WARNING: fault_model='campaign' but "
                          "campaign_event_budget==0; no faults will be injected.\n");
        }
    }
    double fit_rate     = params.find<double>("fit_per_mbit_per_hour", 0.0);
    double dram_mb      = params.find<double>("dram_capacity_mb", 1024.0);
    double per_event_ns = params.find<double>("sim_time_per_event_ns", 100.0);
    if (fit_rate > 0.0 && fault_event_rate_ <= 0.0) {
        // FIT = failures per 1e9 device-hours, per Mbit. Convert to per-event prob.
        // dram_capacity_mb is megaBYTES; FIT is per megaBIT, hence the x8.
        fault_event_rate_ = EccModelMath::fitEventRate(
            fit_rate, dram_mb, per_event_ns);
        if (verbose_) {
            out_->output("EccGuard: FIT=%.3g per Mbit/h x dram_mb=%.0f, sim_ns/event=%.1f -> fault_event_rate=%.3e\n",
                         fit_rate, dram_mb, per_event_ns, fault_event_rate_);
        }
    }

    // Resident fault-map parameters (fault_model='resident'). Parsed
    // unconditionally so sst-info documents them; inert unless the model is
    // selected.
    {
        resident_addr_start_      = params.find<uint64_t>("resident_addr_start", 0);
        resident_addr_len_        = params.find<uint64_t>("resident_addr_len", 0);
        resident_faults_at_start_ = params.find<uint64_t>("resident_faults_at_start", 0);
        double rate_per_ms        = params.find<double>("resident_fault_rate_per_ms", 0.0);
        resident_time_accel_      = params.find<double>("resident_time_acceleration", 1.0);
        double scrub_us           = params.find<double>("resident_scrub_interval_us", 0.0);
        resident_scrub_interval_ns_ = static_cast<uint64_t>(scrub_us * 1e3);
        resident_permanent_fraction_ =
            params.find<double>("resident_permanent_fraction", 0.3);
        std::string rmode = params.find<std::string>("resident_mode", "mix");
        resident_mode_mix_ = (rmode.empty() || rmode == "mix" || rmode == "MIX");
        if (!resident_mode_mix_) resident_mode_fixed_ = parseCampaignMode(rmode);
        resident_row_bytes_ = params.find<uint64_t>("resident_row_bytes", 8192);
        resident_bank_rows_ = params.find<uint64_t>("resident_bank_rows", 8);
        if (resident_row_bytes_ < 64) resident_row_bytes_ = 64;
        if (resident_bank_rows_ == 0) resident_bank_rows_ = 1;

        if (rate_per_ms > 0.0) {
            resident_rate_per_ns_ = rate_per_ms / 1e6;
        } else if (fit_rate > 0.0) {
            // Faults arrive per (capacity x time), not per access: FIT/Mbit/h
            // x capacity -> failures/hour, then scale into sim time.
            // dram_capacity_mb is megaBYTES; FIT is per megaBIT, hence x8.
            double failures_per_hour = fit_rate * 1e-9 * dram_mb * 8.0;
            resident_rate_per_ns_ =
                failures_per_hour / 3.6e12 * resident_time_accel_;
        }

        if (fault_model_ == FaultModel::Resident) {
            uint64_t wbase = 0, wlen = 0;
            if (!resolveResidentWindow(wbase, wlen) || wlen == 0) {
                out_->fatal(CALL_INFO, -1,
                    "EccGuard: fault_model='resident' requires a bounded fault-map "
                    "window: set resident_addr_start/resident_addr_len (or the "
                    "inject_addr_start/inject_addr_len fallback).\n");
            }
            if (resident_rate_per_ns_ <= 0.0 && resident_faults_at_start_ == 0) {
                out_->output("EccGuard WARNING: fault_model='resident' with no "
                              "arrival rate (resident_fault_rate_per_ms / FIT) and "
                              "resident_faults_at_start==0; the fault map stays "
                              "empty and every access classifies clean.\n");
            }
        }
    }

    // Mersenne for the bit-pick (matches RandomFlipFault); std::mt19937 for Poisson.
    uint64_t seed = params.find<uint64_t>("seed", 0);
    if (seed != 0) {
        rng_.seed(seed);
        stdRng_.seed(static_cast<uint32_t>(seed));
    } else {
        stdRng_.seed(0xC0FFEEu);
    }
    // Dedicated RNG for the resident fault map so birth times/footprints stay
    // identical across paired A/B runs that differ only in scheme or traffic.
    residentRng_.seed(seed != 0 ? seed * 0x9E3779B97F4A7C15ULL : 0xD1CEB00Cu);

    if (isPortConnected("highlink")) {
        highlink_ = configureLink("highlink",
            new Event::Handler<EccGuard, &EccGuard::handleHighlink>(this));
    }
    if (isPortConnected("lowlink")) {
        lowlink_ = configureLink("lowlink",
            new Event::Handler<EccGuard, &EccGuard::handleLowlink>(this));
    }
    if (!highlink_ || !lowlink_) {
        out_->fatal(CALL_INFO, -1,
            "EccGuard '%s': both highlink and lowlink must be connected.\n",
            getName().c_str());
    }

    selfLink_ = configureSelfLink("ecc_self", "1ps",
        new Event::Handler<EccGuard, &EccGuard::handleSelf>(this));

    stat_total_              = registerStatistic<uint64_t>("events_total");
    stat_clean_              = registerStatistic<uint64_t>("events_clean");
    stat_correctable_        = registerStatistic<uint64_t>("events_correctable");
    stat_due_                = registerStatistic<uint64_t>("events_due");
    stat_escape_             = registerStatistic<uint64_t>("events_escape");
    stat_latency_            = registerStatistic<uint64_t>("latency_added_ps");
    stat_correlated_row_     = registerStatistic<uint64_t>("events_correlated_row");
    stat_correlated_bank_    = registerStatistic<uint64_t>("events_correlated_bank");
    stat_correlated_device_  = registerStatistic<uint64_t>("events_correlated_device");
    stat_escape_high_blast_  = registerStatistic<uint64_t>("escape_high_blast");
    stat_escape_low_blast_   = registerStatistic<uint64_t>("escape_low_blast");
    stat_due_poisoned_       = registerStatistic<uint64_t>("due_poisoned_bits");
    stat_frames_aborted_     = registerStatistic<uint64_t>("frames_aborted");
    stat_resident_born_      = registerStatistic<uint64_t>("resident_faults_born");
    stat_resident_scrubbed_  = registerStatistic<uint64_t>("resident_faults_scrubbed");
    stat_resident_scrub_due_ = registerStatistic<uint64_t>("resident_scrub_due");
}

EccGuard::~EccGuard() {
    delete out_;
}

void EccGuard::init(unsigned phase) {
    if (highlink_ && lowlink_) {
        SST::Event* ev;
        while ((ev = highlink_->recvUntimedData()) != nullptr) {
            lowlink_->sendUntimedData(ev);
        }
        while ((ev = lowlink_->recvUntimedData()) != nullptr) {
            highlink_->sendUntimedData(ev);
        }
    }
    (void)phase;
}

void EccGuard::setup() {
    resolveStateLazy();

    // Warn in setup() for every policy BER above kEccBerTightUpperBound so
    // reviewers see the bound before the sim produces numbers.
    policy_.forEachEntry([&](const std::string& origin, const EccPolicyEntry& e) {
        if (e.ber > 0.0) {
            warnIfBerExceedsTightBound(e.ber, origin.c_str());
        }
    });

    if (fault_model_ == FaultModel::Resident) {
        for (uint64_t i = 0; i < resident_faults_at_start_; ++i) {
            materializeResidentFault();
        }
        if (resident_rate_per_ns_ > 0.0) {
            std::exponential_distribution<double> exp_ns(resident_rate_per_ns_);
            resident_next_birth_ns_ =
                std::max<uint64_t>(1, static_cast<uint64_t>(exp_ns(residentRng_)));
        }
        if (resident_scrub_interval_ns_ > 0) {
            resident_next_scrub_ns_ = resident_scrub_interval_ns_;
        }
        resident_started_ = true;
        if (verbose_) {
            uint64_t wbase = 0, wlen = 0;
            resolveResidentWindow(wbase, wlen);
            out_->output("EccGuard '%s': resident fault map over [0x%" PRIx64
                         ", +%" PRIu64 "): initial_faults=%" PRIu64
                         " rate=%.3e/ns scrub_ns=%" PRIu64
                         " permanent_frac=%.2f lines_faulty=%zu\n",
                         getName().c_str(), wbase, wlen,
                         resident_faults_at_start_, resident_rate_per_ns_,
                         resident_scrub_interval_ns_,
                         resident_permanent_fraction_, resident_mask_.size());
        }
    }

    if (verbose_) {
        out_->output("EccGuard '%s': setup. uniform scheme=%s ber=%g state_key='%s' state_ptr=%p "
                     "fault_model=%s payload_dtype=%s due_action=%s event_rate=%.3e\n",
                     getName().c_str(),
                     eccSchemeName(policy_.uniform().scheme),
                     policy_.uniform().ber,
                     state_key_.c_str(),
                     (const void*)state_ptr_,
                     (fault_model_ == FaultModel::JedecMix
                          ? "jedec_mix"
                          : (fault_model_ == FaultModel::Campaign ? "campaign"
                                                                  : "poisson")),
                     dtypeName(payload_dtype_),
                     due_action_ == DueAction::DropFrame ? "drop_frame" : "latency_only",
                     fault_event_rate_);
    }
}

void EccGuard::complete(unsigned phase) {
    if (highlink_ && lowlink_) {
        SST::Event* ev;
        while ((ev = highlink_->recvUntimedData()) != nullptr) {
            lowlink_->sendUntimedData(ev);
        }
        while ((ev = lowlink_->recvUntimedData()) != nullptr) {
            highlink_->sendUntimedData(ev);
        }
    }
    (void)phase;
}

void EccGuard::finish() {
    OutcomeCounters totals;
    for (const auto& kv : per_kernel_) {
        totals.clean += kv.second.clean;
        totals.correctable += kv.second.correctable;
        totals.due += kv.second.due;
        totals.escape += kv.second.escape;
    }
    out_->output("\n=== EccGuard %s Per-Kernel Outcomes ===\n", getName().c_str());
    out_->output("kernel_name,clean,correctable,due,escape,latency_ps\n");
    for (const auto& kv : per_kernel_) {
        const auto& c = kv.second;
        if (c.clean + c.correctable + c.due + c.escape == 0) continue;
        const std::string& kname = kv.first.empty() ? std::string("UNKNOWN") : kv.first;
        out_->output("%s,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 "\n",
                     kname.c_str(),
                     c.clean, c.correctable, c.due, c.escape, c.latency_ps);
    }
    out_->output("=== End EccGuard %s Per-Kernel Outcomes ===\n\n", getName().c_str());

    if (!per_kernel_region_.empty()) {
        out_->output("\n=== EccGuard %s Per-Kernel-Per-Region Outcomes ===\n", getName().c_str());
        out_->output("kernel_name,region,clean,correctable,due,escape,latency_ps\n");
        for (auto& kv : per_kernel_region_) {
            const std::string& kname  = kv.first.first.empty()  ? std::string("UNKNOWN")   : kv.first.first;
            const std::string& region = kv.first.second.empty() ? std::string("unlabeled") : kv.first.second;
            const auto& c = kv.second;
            out_->output("%s,%s,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 "\n",
                         kname.c_str(), region.c_str(),
                         c.clean, c.correctable, c.due, c.escape, c.latency_ps);
        }
        out_->output("=== End EccGuard %s Per-Kernel-Per-Region Outcomes ===\n\n", getName().c_str());
    }

    bool any_mode = false;
    for (int i = 0; i < kModeCount; ++i) if (per_mode_draws_[i] > 0) { any_mode = true; break; }
    if (any_mode) {
        out_->output("\n=== EccGuard %s Fault-Mode Draws ===\n", getName().c_str());
        out_->output("mode,count\n");
        for (int i = 0; i < kModeCount; ++i) {
            out_->output("%s,%" PRIu64 "\n",
                         faultModeName(static_cast<FaultMode>(i)), per_mode_draws_[i]);
        }
        out_->output("=== End EccGuard %s Fault-Mode Draws ===\n\n", getName().c_str());
    }

    if (escape_high_blast_total_ + escape_low_blast_total_ > 0
        || frames_aborted_total_ > 0 || due_poison_flips_total_ > 0) {
        out_->output("\n=== EccGuard %s Escape/Abort Summary ===\n", getName().c_str());
        out_->output("escape_high_blast,escape_low_blast,frames_aborted,payload_dtype,due_poisoned_bits\n");
        out_->output("%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%s,%" PRIu64 "\n",
                     escape_high_blast_total_, escape_low_blast_total_,
                     frames_aborted_total_, dtypeName(payload_dtype_),
                     due_poison_flips_total_);
        out_->output("=== End EccGuard %s Escape/Abort Summary ===\n\n", getName().c_str());
    }

    if (fault_model_ == FaultModel::Resident) {
        // Advance the fault-map clock to end-of-sim; it otherwise only moves
        // on traffic, so sparse-then-idle patterns would under-report births
        // and scrubs due after the last access.
        advanceResidentClock(getCurrentSimTimeNano());
        uint64_t alive_permanent = 0;
        for (const auto& f : resident_faults_) if (f.permanent) ++alive_permanent;
        out_->output("\n=== EccGuard %s Resident Fault Map Summary ===\n", getName().c_str());
        out_->output("faults_born,faults_alive,faults_permanent,faults_scrubbed,scrub_due_words,lines_faulty\n");
        out_->output("%" PRIu64 ",%zu,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%zu\n",
                     resident_faults_born_total_, resident_faults_.size(),
                     alive_permanent, resident_faults_scrubbed_total_,
                     resident_scrub_due_total_, resident_mask_.size());
        out_->output("=== End EccGuard %s Resident Fault Map Summary ===\n\n", getName().c_str());
    }

    const uint64_t total = totals.clean + totals.correctable + totals.due + totals.escape;
    bool ok = true;
    auto bound = [&](const char* name, uint64_t got, int64_t lo, int64_t hi) {
        if ((lo >= 0 && got < static_cast<uint64_t>(lo)) ||
            (hi >= 0 && got > static_cast<uint64_t>(hi))) {
            out_->output("EccGuard '%s': FAIL %s=%" PRIu64
                         " outside [%" PRId64 ",%" PRId64 "].\n",
                         getName().c_str(), name, got, lo, hi);
            ok = false;
        }
    };
    bound("total", total, test_total_min_, test_total_max_);
    bound("clean", totals.clean, test_clean_min_, test_clean_max_);
    bound("correctable", totals.correctable,
          test_correctable_min_, test_correctable_max_);
    bound("due", totals.due, test_due_min_, test_due_max_);
    bound("escape", totals.escape, test_escape_min_, test_escape_max_);
    bound("resident_born", resident_faults_born_total_,
          test_resident_born_min_, test_resident_born_max_);
    if (!ok) out_->fatal(CALL_INFO, -1, "EccGuard test expectations failed.\n");
}

void EccGuard::resolveStateLazy() {
    if (state_ptr_ || state_key_.empty()) return;
    state_ptr_ = PipelineStateRegistry<PipelineStateBase>::get(state_key_);
}

int EccGuard::resolveRegionId(uint64_t addr) const {
    if (!state_ptr_) return -1;
    return state_ptr_->regionIdForAddress(addr);
}

// Region attribution: EccGuard sees physical addrs below the dTLB, but agents
// publish virtual regions. Prefer MemEvent::vAddr_ (stamped by dTLB, preserved
// by clone/makeResponse); fall back to physical when vAddr=0 (e.g. writebacks).
int EccGuard::resolveRegionIdForEvent(MemEvent* mev) const {
    if (!mev || !state_ptr_) return -1;
    uint64_t vaddr = mev->getVirtualAddress();
    if (vaddr != 0) {
        int rid = state_ptr_->regionIdForAddress(vaddr);
        if (rid >= 0) return rid;
    }
    return state_ptr_->regionIdForAddress(mev->getAddr());
}

const std::string& EccGuard::regionNameForId(int region_id) const {
    static const std::string empty;
    if (!state_ptr_ || region_id < 0) return empty;
    if (region_id >= static_cast<int>(state_ptr_->regions.size())) return empty;
    return state_ptr_->regions[region_id].name;
}

bool EccGuard::resolveAddrFilterBounds(uint64_t& base_out, uint64_t& len_out) const {
    base_out = 0;
    len_out  = 0;
    if (addr_filter_region_.empty()) return false;
    if (!state_ptr_) return false;
    for (const auto& r : state_ptr_->regions) {
        if (!r.valid || r.name != addr_filter_region_) continue;
        base_out = r.base;
        len_out  = r.size;
        if (addr_filter_len_ > 0 && addr_filter_len_ < len_out)
            len_out = addr_filter_len_;
        return len_out > 0;
    }
    return false;
}

bool EccGuard::shouldApplyPolicy(MemEvent* mev) {
    if (!mev) return false;
    resolveStateLazy();
    if (!applyOnResponsesOnly_) return true;
    if (mev->isResponse()) return true;
    if (fault_model_ != FaultModel::Campaign || addr_filter_region_.empty())
        return false;
    return eventOverlapsAddrFilter(mev) && !mev->getPayload().empty();
}

bool EccGuard::eventOverlapsAddrFilter(MemEvent* mev) const {
    if (addr_filter_region_.empty() || !mev) return true;
    if (state_ptr_) {
        int rid = resolveRegionIdForEvent(mev);
        if (rid >= 0 && regionNameForId(rid) == addr_filter_region_) return true;
    }
    uint64_t fbase = 0, flen = 0;
    if (!resolveAddrFilterBounds(fbase, flen)) return false;
    uint64_t vaddr = mev->getVirtualAddress();
    uint64_t addr  = (vaddr != 0) ? vaddr : mev->getAddr();
    uint64_t size  = mev->getPayload().empty() ? 64u : mev->getPayload().size();
    uint64_t end   = addr + size;
    uint64_t fend  = fbase + flen;
    return addr < fend && end > fbase;
}

void EccGuard::noteCampaignKernelEntry(const std::string& kernel_name) {
    if (campaign_max_per_kernel_entry_ == 0) return;
    // Reset entry budget on any kernel change so re-entry starts fresh.
    EccModelMath::resetCampaignEntry(kernel_name,
                                     campaign_entry_kernel_name_,
                                     campaign_events_this_entry_);
}

void EccGuard::requestFrameAbort() {
    if (state_key_.empty()) return;
    PipelineStateBase* s =
        PipelineStateRegistry<PipelineStateBase>::getMutable(state_key_);
    if (!s) return;
    s->frameAbortRequested = true;
    ++frames_aborted_total_;
    if (stat_frames_aborted_) stat_frames_aborted_->addData(1);
}

namespace {
// Helper: bump the registry's cumulative ECC counters so the ActionScorer
// (and any other consumer) can compute per-frame deltas. Cheap pointer chase.
void publishCumulative(const std::string& state_key, uint64_t escapes_inc,
                       uint64_t flips_inc) {
    if (state_key.empty()) return;
    PipelineStateBase* s =
        PipelineStateRegistry<PipelineStateBase>::getMutable(state_key);
    if (!s) return;
    s->eccCumulativeEscapes += escapes_inc;
    s->eccCumulativeFlips   += flips_inc;
}

// Bump per-frame per-kernel escape counts (argmaxed at frame close).
void publishPerFrameEscape(const std::string& state_key,
                           const std::string& kernel_name) {
    if (state_key.empty()) return;
    PipelineStateBase* s =
        PipelineStateRegistry<PipelineStateBase>::getMutable(state_key);
    if (!s) return;
    s->eccPerFrameEscapesByKernel[kernel_name] += 1;
}
} // namespace

void EccGuard::handleHighlink(SST::Event* ev) {
    auto* mev = dynamic_cast<MemEvent*>(ev);
    if (!mev || !shouldApplyPolicy(mev)) {
        if (lowlink_) lowlink_->send(ev); else delete ev;
        return;
    }

    uint64_t latency_ps = applyPolicy(mev);
    if (latency_ps == 0) {
        lowlink_->send(ev);
    } else {
        selfLink_->send(static_cast<SimTime_t>(latency_ps),
                        new EccGuardDelayEvent(ev, /*down=*/true));
    }
}

void EccGuard::handleLowlink(SST::Event* ev) {
    auto* mev = dynamic_cast<MemEvent*>(ev);
    if (!mev) {
        if (highlink_) highlink_->send(ev); else delete ev;
        return;
    }
    if (!shouldApplyPolicy(mev)) {
        highlink_->send(ev);
        return;
    }

    uint64_t latency_ps = applyPolicy(mev);
    if (latency_ps == 0) {
        highlink_->send(ev);
    } else {
        selfLink_->send(static_cast<SimTime_t>(latency_ps),
                        new EccGuardDelayEvent(ev, /*down=*/false));
    }
}

void EccGuard::handleSelf(SST::Event* ev) {
    auto* pe = dynamic_cast<EccGuardDelayEvent*>(ev);
    if (!pe) { delete ev; return; }
    SST::Event* original = pe->original();
    bool        down     = pe->isDown();
    pe->clearOriginal();
    delete pe;

    if (down) {
        if (lowlink_)  lowlink_->send(original);
        else           delete original;
    } else {
        if (highlink_) highlink_->send(original);
        else           delete original;
    }
}

namespace {

// Number of ECC protection words a `payload_bytes` line contains under
// `scheme`. Falls back to 1 (treat the whole payload as one "word") when the
// scheme has no word concept (e.g. NONE).
inline uint32_t numWords(uint32_t payload_bytes, EccScheme scheme) {
    return EccModelMath::wordCount(payload_bytes, scheme);
}

// Bits per ECC word for the draw. For schemes with no word concept we use
// payload bits.
inline uint32_t bitsPerWord(uint32_t payload_bytes, EccScheme scheme) {
    uint32_t wb = eccWordBytes(scheme);
    if (wb == 0) return payload_bytes * 8;
    return wb * 8;
}

inline bool isCorrelatedMode(EccGuard::FaultMode m) {
    // SingleWord and spatial modes deposit all errors into one ECC word —
    // the clustering chipkill is designed against. SingleCell is 1-bit.
    switch (m) {
    case EccGuard::FaultMode::SingleCell:    return false;
    case EccGuard::FaultMode::SingleWord:    return true;
    case EccGuard::FaultMode::SingleRow:     return true;
    case EccGuard::FaultMode::SingleColumn:  return true;
    case EccGuard::FaultMode::SingleBank:    return true;
    case EccGuard::FaultMode::SingleDevice:  return true;
    default:                                  return false;
    }
}

} // namespace

// Distribute `errs` bit-errors uniformly across chips in one word.
// For SingleDevice mode all errors land in a single randomly-chosen chip.
void EccGuard::distributeErrorsToChips(
        std::vector<uint8_t>& chip_counts,
        unsigned errs, EccScheme scheme, FaultMode mode) {
    unsigned nchips = chipsPerEccWord(scheme);
    if (nchips == 0 || errs == 0) return;
    chip_counts.assign(nchips, 0);
    if (campaign_force_multi_chip_ && nchips >= 3) {
        unsigned need = std::min<unsigned>(3u, nchips);
        std::vector<unsigned> picks;
        picks.reserve(need);
        std::uniform_int_distribution<unsigned> cpick(0, nchips - 1);
        while (picks.size() < need) {
            unsigned c = cpick(stdRng_);
            if (std::find(picks.begin(), picks.end(), c) == picks.end())
                picks.push_back(c);
        }
        unsigned per = std::max(1u, errs / need);
        unsigned rem = errs;
        for (size_t i = 0; i < picks.size(); ++i) {
            unsigned put = (i + 1 == picks.size()) ? rem : std::min(rem, per);
            chip_counts[picks[i]] = static_cast<uint8_t>(std::min<unsigned>(put, 255));
            rem -= put;
        }
        return;
    }
    if (mode == FaultMode::SingleDevice) {
        std::uniform_int_distribution<unsigned> cpick(0, nchips - 1);
        unsigned c = cpick(stdRng_);
        chip_counts[c] = static_cast<uint8_t>(std::min<unsigned>(errs, 4));
    } else {
        std::uniform_int_distribution<unsigned> cpick(0, nchips - 1);
        for (unsigned i = 0; i < errs; ++i) {
            unsigned c = cpick(stdRng_);
            if (chip_counts[c] < 255) ++chip_counts[c];
        }
    }
}

EccGuard::FaultDraw EccGuard::drawFaultPoisson(uint32_t payload_bytes,
                                               double ber,
                                               EccScheme scheme) {
    FaultDraw d;
    d.mode = FaultMode::SingleCell;
    if (payload_bytes == 0) return d;

    uint32_t nwords = numWords(payload_bytes, scheme);
    d.per_word_errors.assign(nwords, 0u);
    if (ber <= 0.0) return d;

    // Per-word Bernoulli/Poisson draws; partial final words use actual bit count.
    unsigned total = 0;
    bool need_chips = (scheme == EccScheme::CHIPKILL_x4);
    if (need_chips) d.per_word_chip_errors.resize(nwords);
    for (uint32_t w = 0; w < nwords; ++w) {
        unsigned word_bits = EccModelMath::wordBits(payload_bytes, scheme, w);
        if (word_bits == 0) break;
        std::poisson_distribution<unsigned> dist(static_cast<double>(word_bits) * ber);
        unsigned errs = dist(stdRng_);
        if (errs > word_bits) errs = word_bits;
        d.per_word_errors[w] = errs;
        total += errs;
        if (need_chips && errs > 0)
            distributeErrorsToChips(d.per_word_chip_errors[w], errs, scheme, d.mode);
    }
    d.num_errors = total;
    return d;
}

EccGuard::FaultDraw EccGuard::drawFaultJedecMix(uint32_t payload_bytes,
                                                double event_rate,
                                                EccScheme scheme) {
    FaultDraw d;
    if (payload_bytes == 0) return d;

    uint32_t nwords = numWords(payload_bytes, scheme);
    d.per_word_errors.assign(nwords, 0u);
    if (event_rate <= 0.0) return d;
    std::bernoulli_distribution gate(std::min(event_rate, 1.0));
    if (!gate(stdRng_)) return d;

    // Choose a mode by cumulative weight.
    std::uniform_real_distribution<double> u01(0.0, 1.0);
    double r = u01(stdRng_);
    double acc = 0.0;
    int chosen = 0;
    for (int i = 0; i < kModeCount; ++i) {
        acc += mode_weights_[i];
        if (r <= acc) { chosen = i; break; }
    }
    d.mode = static_cast<FaultMode>(chosen);

    // Sample a bit-error count uniformly inside the mode's range; cap at payload bits.
    unsigned lo = kFaultModeBitsLow[chosen];
    unsigned hi = kFaultModeBitsHigh[chosen];
    if (hi < lo) hi = lo;
    std::uniform_int_distribution<unsigned> nbits(lo, hi);
    unsigned errs = nbits(stdRng_);
    unsigned cap  = payload_bytes * 8;
    if (cap > 0 && errs > cap) errs = cap;
    d.num_errors = errs;

    // Correlated/single-word modes deposit into one random word (physical
    // clustering); SingleCell scatters across words bit-by-bit.
    bool need_chips = (scheme == EccScheme::CHIPKILL_x4);
    if (need_chips) d.per_word_chip_errors.resize(nwords);
    if (nwords > 0) {
        if (isCorrelatedMode(d.mode)) {
            std::uniform_int_distribution<uint32_t> wpick(0, nwords - 1);
            uint32_t w = wpick(stdRng_);
            unsigned word_cap = bitsPerWord(payload_bytes, scheme);
            if (word_cap > 0 && errs > word_cap) {
                d.per_word_errors[w] = word_cap;
                if (need_chips)
                    distributeErrorsToChips(d.per_word_chip_errors[w], word_cap, scheme, d.mode);
                unsigned remaining = errs - word_cap;
                uint32_t offset = 1;
                while (remaining > 0 && offset < nwords) {
                    uint32_t wn = (w + offset) % nwords;
                    unsigned put = std::min<unsigned>(word_cap, remaining);
                    d.per_word_errors[wn] = put;
                    if (need_chips)
                        distributeErrorsToChips(d.per_word_chip_errors[wn], put, scheme, d.mode);
                    remaining -= put;
                    ++offset;
                }
            } else {
                d.per_word_errors[w] = errs;
                if (need_chips)
                    distributeErrorsToChips(d.per_word_chip_errors[w], errs, scheme, d.mode);
            }
        } else {
            // Uncorrelated (SingleCell, default): scatter bit-by-bit.
            std::uniform_int_distribution<uint32_t> wpick(0, nwords - 1);
            for (unsigned i = 0; i < errs; ++i) {
                uint32_t w = wpick(stdRng_);
                d.per_word_errors[w] += 1;
            }
            if (need_chips) {
                for (uint32_t w = 0; w < nwords; ++w) {
                    if (d.per_word_errors[w] > 0)
                        distributeErrorsToChips(d.per_word_chip_errors[w],
                                                d.per_word_errors[w], scheme, d.mode);
                }
            }
        }
    }

    ++per_mode_draws_[chosen];
    if (chosen == static_cast<int>(FaultMode::SingleRow)    && stat_correlated_row_)    stat_correlated_row_->addData(1);
    if (chosen == static_cast<int>(FaultMode::SingleBank)   && stat_correlated_bank_)   stat_correlated_bank_->addData(1);
    if (chosen == static_cast<int>(FaultMode::SingleDevice) && stat_correlated_device_) stat_correlated_device_->addData(1);

    return d;
}

// Campaign injector: at most campaign_event_budget_ events of campaign_mode_
// into one random ECC word. Gated on campaign_target_kernel_name_ when set.
EccGuard::FaultDraw EccGuard::drawFaultCampaign(uint32_t payload_bytes,
                                                 EccScheme scheme,
                                                 const std::string& kernel_name) {
    FaultDraw d;
    if (payload_bytes == 0) return d;

    uint32_t nwords = numWords(payload_bytes, scheme);
    d.per_word_errors.assign(nwords, 0u);

    if (campaign_event_budget_ == 0) return d;
    if (campaign_events_fired_ >= campaign_event_budget_) return d;
    // Addr-filtered campaign: action_queue traffic is the temporal proxy.
    // ReadResp often returns after publishKernel(IDLE), so do not gate on FSM.
    const bool addr_filtered = !addr_filter_region_.empty();
    if (addr_filtered && campaign_max_per_kernel_entry_ > 0 && state_ptr_) {
        const int pc = state_ptr_->pipelineCycle;
        if (pc != campaign_entry_pipeline_cycle_) {
            campaign_entry_pipeline_cycle_ = pc;
            campaign_events_this_entry_    = 0;
        }
    } else {
        // Must run before the target gate below: off-target events update the
        // last-seen kernel so re-entering the target resets the entry budget.
        noteCampaignKernelEntry(kernel_name);
    }
    if (!addr_filtered && !campaign_target_kernel_name_.empty()
        && kernel_name != campaign_target_kernel_name_) {
        return d;
    }
    if (campaign_max_per_kernel_entry_ > 0
        && campaign_events_this_entry_ >= campaign_max_per_kernel_entry_) {
        return d;
    }
    if (campaign_event_rate_ <= 0.0) return d;
    std::bernoulli_distribution gate(std::min(campaign_event_rate_, 1.0));
    if (!gate(stdRng_)) return d;

    d.mode = campaign_mode_;
    int chosen = static_cast<int>(campaign_mode_);

    unsigned lo = kFaultModeBitsLow [chosen];
    unsigned hi = kFaultModeBitsHigh[chosen];
    if (hi < lo) hi = lo;
    unsigned errs = 0;
    if (campaign_errors_fixed_ > 0) {
        errs = campaign_errors_fixed_;
    } else {
        std::uniform_int_distribution<unsigned> nbits(lo, hi);
        errs = nbits(stdRng_);
    }
    unsigned cap  = payload_bytes * 8;
    if (cap > 0 && errs > cap) errs = cap;
    d.num_errors = errs;

    bool need_chips = (scheme == EccScheme::CHIPKILL_x4);
    if (need_chips) d.per_word_chip_errors.resize(nwords);
    if (nwords > 0) {
        if (isCorrelatedMode(d.mode)) {
            std::uniform_int_distribution<uint32_t> wpick(0, nwords - 1);
            uint32_t w = wpick(stdRng_);
            unsigned word_cap = bitsPerWord(payload_bytes, scheme);
            if (word_cap > 0 && errs > word_cap) {
                d.per_word_errors[w] = word_cap;
                if (need_chips)
                    distributeErrorsToChips(d.per_word_chip_errors[w], word_cap, scheme, d.mode);
                unsigned remaining = errs - word_cap;
                uint32_t offset = 1;
                while (remaining > 0 && offset < nwords) {
                    uint32_t wn = (w + offset) % nwords;
                    unsigned put = std::min<unsigned>(word_cap, remaining);
                    d.per_word_errors[wn] = put;
                    if (need_chips)
                        distributeErrorsToChips(d.per_word_chip_errors[wn], put, scheme, d.mode);
                    remaining -= put;
                    ++offset;
                }
            } else {
                d.per_word_errors[w] = errs;
                if (need_chips)
                    distributeErrorsToChips(d.per_word_chip_errors[w], errs, scheme, d.mode);
            }
        } else {
            std::uniform_int_distribution<uint32_t> wpick(0, nwords - 1);
            for (unsigned i = 0; i < errs; ++i) {
                d.per_word_errors[wpick(stdRng_)] += 1;
            }
            if (need_chips) {
                for (uint32_t w = 0; w < nwords; ++w) {
                    if (d.per_word_errors[w] > 0)
                        distributeErrorsToChips(d.per_word_chip_errors[w],
                                                d.per_word_errors[w], scheme, d.mode);
                }
            }
        }
    }

    ++campaign_events_fired_;
    ++campaign_events_this_entry_;
    ++per_mode_draws_[chosen];
    if (chosen == static_cast<int>(FaultMode::SingleRow)    && stat_correlated_row_)    stat_correlated_row_->addData(1);
    if (chosen == static_cast<int>(FaultMode::SingleBank)   && stat_correlated_bank_)   stat_correlated_bank_->addData(1);
    if (chosen == static_cast<int>(FaultMode::SingleDevice) && stat_correlated_device_) stat_correlated_device_->addData(1);
    return d;
}

// Resident fault map: Poisson births in sim time (capacity x residency),
// deterministic on every access, cleared only by patrol scrub. All randomness
// from residentRng_ so equal seeds give identical faults across schemes.

bool EccGuard::resolveResidentWindow(uint64_t& base_out, uint64_t& len_out) const {
    if (resident_addr_len_ > 0) {
        base_out = resident_addr_start_;
        len_out  = resident_addr_len_;
        return true;
    }
    if (inject_addr_len_ > 0) {
        base_out = inject_addr_start_;
        len_out  = inject_addr_len_;
        return true;
    }
    base_out = 0;
    len_out  = 0;
    return false;
}

void EccGuard::advanceResidentClock(uint64_t now_ns) {
    if (!resident_started_) return;
    while (true) {
        const bool     have_birth = resident_rate_per_ns_ > 0.0;
        const bool     have_scrub = resident_scrub_interval_ns_ > 0;
        const uint64_t tb = have_birth ? resident_next_birth_ns_ : UINT64_MAX;
        const uint64_t ts = have_scrub ? resident_next_scrub_ns_ : UINT64_MAX;
        if (std::min(tb, ts) > now_ns) break;
        if (tb <= ts) {
            materializeResidentFault();
            std::exponential_distribution<double> exp_ns(resident_rate_per_ns_);
            resident_next_birth_ns_ =
                tb + std::max<uint64_t>(1, static_cast<uint64_t>(exp_ns(residentRng_)));
        } else {
            applyResidentScrub();
            resident_next_scrub_ns_ = ts + resident_scrub_interval_ns_;
        }
    }
}

// Interleaved x4 chip layout (write + read attribution): nibble N -> chip
// N % kResidentChipsPerLine. Keep residentChipForWordBit in sync or chipkill
// vs SECDED attribution silently rots.
static constexpr unsigned kResidentNibbleBits   = 4;
static constexpr unsigned kResidentChipsPerLine = 32;

static inline unsigned residentLineBitForChip(unsigned chip, unsigned nibble_idx,
                                              unsigned bit_in_nibble) {
    return kResidentNibbleBits * (chip + kResidentChipsPerLine * nibble_idx)
           + bit_in_nibble;
}

static inline unsigned residentChipForWordBit(uint32_t bit_in_word, size_t nchips) {
    return static_cast<unsigned>((bit_in_word / kResidentNibbleBits) % nchips);
}

void EccGuard::addUniformBitInLine(ResidentFault& f, uint64_t line_base,
                                   unsigned bit_in_line) {
    auto& mask = f.line_bits[line_base]; // value-initialized (zeroed) on first touch
    mask[bit_in_line / 8] |= static_cast<uint8_t>(1u << (bit_in_line % 8));
}

void EccGuard::addChipBitsInLine(ResidentFault& f, uint64_t line_base,
                                 unsigned chip, unsigned nbits) {
    unsigned positions[16];
    unsigned n = 0;
    for (unsigned j = 0; j < 4; ++j)
        for (unsigned i = 0; i < kResidentNibbleBits; ++i)
            positions[n++] = residentLineBitForChip(chip, j, i);
    if (nbits > 16) nbits = 16;
    // Partial Fisher-Yates: nbits distinct positions.
    for (unsigned k = 0; k < nbits; ++k) {
        std::uniform_int_distribution<unsigned> pick(k, 15);
        std::swap(positions[k], positions[pick(residentRng_)]);
        addUniformBitInLine(f, line_base, positions[k]);
    }
}

void EccGuard::materializeResidentFault() {
    uint64_t wbase = 0, wlen = 0;
    if (!resolveResidentWindow(wbase, wlen) || wlen == 0) return;
    const uint64_t first_line = wbase & ~63ULL;
    const uint64_t nlines     = (wbase + wlen - first_line + 63) / 64;

    ResidentFault f;
    if (resident_mode_mix_) {
        std::uniform_real_distribution<double> u01(0.0, 1.0);
        double r = u01(residentRng_), acc = 0.0;
        int chosen = 0;
        for (int i = 0; i < kModeCount; ++i) {
            acc += mode_weights_[i];
            if (r <= acc) { chosen = i; break; }
        }
        f.mode = static_cast<FaultMode>(chosen);
    } else {
        f.mode = resident_mode_fixed_;
    }
    f.permanent =
        std::bernoulli_distribution(resident_permanent_fraction_)(residentRng_);

    auto lineAt = [&](uint64_t idx) { return first_line + idx * 64; };
    std::uniform_int_distribution<uint64_t> lpick(0, nlines - 1);
    std::uniform_int_distribution<unsigned> chip_pick(0, 31);
    std::uniform_int_distribution<unsigned> k14(1, 4);
    std::uniform_int_distribution<unsigned> k12(1, 2);
    std::uniform_int_distribution<unsigned> bpick(0, 511);

    const uint64_t row_lines = std::max<uint64_t>(1, resident_row_bytes_ / 64);
    const uint64_t nrows     = (nlines + row_lines - 1) / row_lines;
    // Subsample gigantic windows so a device fault cannot materialize an
    // unbounded footprint.
    constexpr uint64_t kMaxFootprintLines = 65536;
    const uint64_t stride = 1 + (nlines - 1) / kMaxFootprintLines;

    switch (f.mode) {
    case FaultMode::SingleCell:
        addUniformBitInLine(f, lineAt(lpick(residentRng_)), bpick(residentRng_));
        break;
    case FaultMode::SingleWord: {
        // Multi-bit fault at one address: 2 distinct bits inside one aligned
        // 64-bit region of a single line.
        uint64_t lb = lineAt(lpick(residentRng_));
        std::uniform_int_distribution<unsigned> rpick(0, 7);
        std::uniform_int_distribution<unsigned> bit64(0, 63);
        unsigned region = rpick(residentRng_);
        unsigned b1 = bit64(residentRng_), b2 = b1;
        while (b2 == b1) b2 = bit64(residentRng_);
        addUniformBitInLine(f, lb, region * 64 + b1);
        addUniformBitInLine(f, lb, region * 64 + b2);
        break;
    }
    case FaultMode::SingleRow: {
        // One DRAM row on one x4 chip: every line of the row carries 1-4 bad
        // bits confined to that chip's nibbles.
        unsigned chip = chip_pick(residentRng_);
        std::uniform_int_distribution<uint64_t> rowp(0, nrows - 1);
        uint64_t r0 = rowp(residentRng_) * row_lines;
        for (uint64_t i = r0; i < std::min(nlines, r0 + row_lines); i += stride)
            addChipBitsInLine(f, lineAt(i), chip, k14(residentRng_));
        break;
    }
    case FaultMode::SingleColumn: {
        // Same in-row line offset across every row, one chip.
        unsigned chip = chip_pick(residentRng_);
        std::uniform_int_distribution<uint64_t> colp(0, row_lines - 1);
        uint64_t col = colp(residentRng_);
        for (uint64_t r = 0; r < nrows; r += stride) {
            uint64_t idx = r * row_lines + col;
            if (idx < nlines)
                addChipBitsInLine(f, lineAt(idx), chip, k12(residentRng_));
        }
        break;
    }
    case FaultMode::SingleBank: {
        // A contiguous group of rows in one bank, one chip; the fault
        // manifests at one scattered line per row.
        unsigned chip   = chip_pick(residentRng_);
        uint64_t nbanks = std::max<uint64_t>(1, nrows / resident_bank_rows_);
        std::uniform_int_distribution<uint64_t> bankp(0, nbanks - 1);
        std::uniform_int_distribution<uint64_t> colp(0, row_lines - 1);
        uint64_t r0 = bankp(residentRng_) * resident_bank_rows_;
        for (uint64_t r = r0; r < std::min(nrows, r0 + resident_bank_rows_); ++r) {
            uint64_t idx = r * row_lines + colp(residentRng_);
            if (idx < nlines)
                addChipBitsInLine(f, lineAt(idx), chip, k14(residentRng_));
        }
        break;
    }
    case FaultMode::SingleDevice:
        // Whole x4 chip: every line in the window sees 1-4 bad bits in that
        // chip's nibbles. The pattern chipkill is built to absorb.
        for (uint64_t i = 0, chip = chip_pick(residentRng_); i < nlines; i += stride)
            addChipBitsInLine(f, lineAt(i), static_cast<unsigned>(chip),
                              k14(residentRng_));
        break;
    default:
        addUniformBitInLine(f, lineAt(lpick(residentRng_)), bpick(residentRng_));
        break;
    }

    for (const auto& kv : f.line_bits) {
        auto& m = resident_mask_[kv.first];
        for (int i = 0; i < 64; ++i) m[i] |= kv.second[i];
    }
    int chosen = static_cast<int>(f.mode);
    ++per_mode_draws_[chosen];
    if (chosen == static_cast<int>(FaultMode::SingleRow)    && stat_correlated_row_)    stat_correlated_row_->addData(1);
    if (chosen == static_cast<int>(FaultMode::SingleBank)   && stat_correlated_bank_)   stat_correlated_bank_->addData(1);
    if (chosen == static_cast<int>(FaultMode::SingleDevice) && stat_correlated_device_) stat_correlated_device_->addData(1);
    ++resident_faults_born_total_;
    if (stat_resident_born_) stat_resident_born_->addData(1);
    if (verbose_) {
        out_->output("EccGuard '%s': resident fault born: mode=%s permanent=%d "
                     "lines=%zu\n",
                     getName().c_str(), faultModeName(f.mode),
                     f.permanent ? 1 : 0, f.line_bits.size());
    }
    resident_faults_.push_back(std::move(f));
}

void EccGuard::rebuildResidentMask() {
    resident_mask_.clear();
    for (const auto& f : resident_faults_) {
        for (const auto& kv : f.line_bits) {
            bool any = false;
            for (uint8_t b : kv.second) if (b) { any = true; break; }
            if (!any) continue;
            auto& m = resident_mask_[kv.first];
            for (int i = 0; i < 64; ++i) m[i] |= kv.second[i];
        }
    }
}

void EccGuard::applyResidentScrub() {
    if (resident_mask_.empty()) return;
    // Patrol scrub: correct what the code can, rewrite clears transient cells;
    // permanent cells fail again. Multi-fault words beyond correction stay —
    // the accumulation-before-scrub effect. Uses the uniform scheme.
    const EccScheme scheme = policy_.uniform().scheme;
    const uint32_t  wb     = eccWordBytes(scheme);
    for (auto& kv : resident_mask_) {
        const uint64_t lb    = kv.first;
        const auto&    mask  = kv.second;
        const uint32_t words = (wb == 0) ? 1 : (64 + wb - 1) / wb;
        for (uint32_t w = 0; w < words; ++w) {
            const uint32_t b0 = (wb == 0) ? 0  : w * wb;
            const uint32_t b1 = (wb == 0) ? 64 : std::min<uint32_t>(64, b0 + wb);
            unsigned errs = 0;
            std::vector<uint8_t> chip_errs;
            if (scheme == EccScheme::CHIPKILL_x4)
                chip_errs.assign(chipsPerEccWord(scheme), 0);
            for (uint32_t byte = b0; byte < b1; ++byte) {
                unsigned m = mask[byte];
                while (m) {
                    unsigned bit = static_cast<unsigned>(__builtin_ctz(m));
                    m &= m - 1;
                    ++errs;
                    if (!chip_errs.empty()) {
                        unsigned bit_in_word = (byte - b0) * 8 + bit;
                        unsigned chip = residentChipForWordBit(bit_in_word,
                                                               chip_errs.size());
                        if (chip_errs[chip] < 255) ++chip_errs[chip];
                    }
                }
            }
            if (errs == 0) continue;
            EccOutcome o = chip_errs.empty()
                ? classifyEccWord(errs, scheme)
                : classifyEccWordChipAware(chip_errs, scheme);
            if (o == EccOutcome::Correctable) {
                for (auto& f : resident_faults_) {
                    if (f.permanent) continue;
                    auto it = f.line_bits.find(lb);
                    if (it == f.line_bits.end()) continue;
                    for (uint32_t byte = b0; byte < b1; ++byte)
                        it->second[byte] = 0;
                }
            } else if (o != EccOutcome::Clean) {
                ++resident_scrub_due_total_;
                if (stat_resident_scrub_due_) stat_resident_scrub_due_->addData(1);
            }
        }
    }
    const size_t before = resident_faults_.size();
    resident_faults_.erase(
        std::remove_if(resident_faults_.begin(), resident_faults_.end(),
            [](const ResidentFault& f) {
                if (f.permanent) return false;
                for (const auto& kv : f.line_bits)
                    for (uint8_t b : kv.second)
                        if (b) return false;
                return true;
            }),
        resident_faults_.end());
    const size_t cleared = before - resident_faults_.size();
    if (cleared > 0) {
        resident_faults_scrubbed_total_ += cleared;
        if (stat_resident_scrubbed_) stat_resident_scrubbed_->addData(cleared);
        if (verbose_) {
            out_->output("EccGuard '%s': patrol scrub cleared %zu transient "
                         "fault(s); %zu alive\n",
                         getName().c_str(), cleared, resident_faults_.size());
        }
    }
    rebuildResidentMask();
}

EccGuard::FaultDraw EccGuard::drawFaultResident(MemEvent* mev,
                                                uint32_t payload_bytes,
                                                EccScheme scheme) {
    FaultDraw d;
    if (payload_bytes == 0) return d;
    uint32_t nwords = numWords(payload_bytes, scheme);
    d.per_word_errors.assign(nwords, 0u);
    if (resident_mask_.empty()) return d;

    // Same address-space preference as the window filters: the preserved
    // virtual address when present, else the physical/SST address.
    uint64_t a = mev->getVirtualAddress();
    if (a == 0) a = mev->getAddr();

    const uint32_t wb = eccWordBytes(scheme);
    const bool need_chips = (scheme == EccScheme::CHIPKILL_x4);
    if (need_chips) d.per_word_chip_errors.resize(nwords);

    for (uint64_t lb = a & ~63ULL; lb < a + payload_bytes; lb += 64) {
        auto it = resident_mask_.find(lb);
        if (it == resident_mask_.end()) continue;
        const auto& mask = it->second;
        for (unsigned byte = 0; byte < 64; ++byte) {
            unsigned m = mask[byte];
            if (!m) continue;
            const uint64_t abs_byte = lb + byte;
            if (abs_byte < a || abs_byte >= a + payload_bytes) continue;
            const uint32_t rel_byte = static_cast<uint32_t>(abs_byte - a);
            const uint32_t w = (wb == 0) ? 0
                : std::min<uint32_t>(rel_byte / wb, nwords - 1);
            while (m) {
                const unsigned bit = static_cast<unsigned>(__builtin_ctz(m));
                m &= m - 1;
                d.exact_bits.push_back(rel_byte * 8 + bit);
                d.per_word_errors[w] += 1;
                d.num_errors += 1;
                if (need_chips) {
                    auto& cc = d.per_word_chip_errors[w];
                    if (cc.empty()) cc.assign(chipsPerEccWord(scheme), 0);
                    const uint32_t bit_in_word = (rel_byte - w * wb) * 8 + bit;
                    const unsigned chip = residentChipForWordBit(bit_in_word,
                                                                 cc.size());
                    if (cc[chip] < 255) ++cc[chip];
                }
            }
        }
    }
    return d;
}

unsigned EccGuard::flipExactBitsInWord(MemEvent* mev, uint32_t word_index,
                                       EccScheme scheme,
                                       const std::vector<uint32_t>& exact_bits,
                                       unsigned& high_flips, unsigned& low_flips) {
    auto& payload = mev->getPayload();
    if (payload.empty() || exact_bits.empty()) return 0;
    const uint32_t total_bits = static_cast<uint32_t>(payload.size()) * 8;
    const uint32_t wb        = eccWordBytes(scheme);
    const uint32_t start_bit = (wb == 0) ? 0 : word_index * wb * 8;
    const uint32_t end_bit   = (wb == 0) ? total_bits
        : std::min(total_bits, start_bit + wb * 8);
    unsigned elem_bytes = dtypeBytes(payload_dtype_);
    if (elem_bytes == 0) elem_bytes = 1;

    unsigned flipped = 0;
    for (uint32_t b : exact_bits) {
        if (b < start_bit || b >= end_bit) continue;
        const uint32_t byte = b / 8u, bit = b % 8u;
        payload[byte] ^= static_cast<uint8_t>(1u << bit);
        bool hi = false;
        if (payload_dtype_ != PayloadDtype::Bytes) {
            hi = isHighBlastBit(payload_dtype_, (byte % elem_bytes) * 8u + bit);
        }
        if (hi) ++high_flips; else ++low_flips;
        ++flipped;
    }
    return flipped;
}

void EccGuard::warnIfBerExceedsTightBound(double ber, const char* origin) {
    if (ber <= kEccBerTightUpperBound) return;
    // Memoize so repeated BER values don't spam the log.
    uint64_t key = 0;
    std::memcpy(&key, &ber, sizeof(key));
    if (!ber_warned_.insert(key).second) return;
    if (out_) {
        out_->output(
            "EccGuard '%s': WARNING %s BER=%.3e exceeds the per-word "
            "single-bit approximation's tight bound (%.1e). The per-word "
            "draws still classify each event correctly, but the "
            "Correctable/DUE/Escape proportions are no longer provably "
            "tight to within ~1%% of an exact Binomial decode. See "
            "eccScheme.h::kEccBerTightUpperBound for the derivation.\n",
            getName().c_str(), origin, ber, kEccBerTightUpperBound);
    }
}

uint64_t EccGuard::applyPolicy(MemEvent* mev) {
    if (!state_ptr_) resolveStateLazy();

    std::string kernel_name;
    if (state_ptr_) kernel_name = state_ptr_->currentKernelName;

    if (!addr_filter_region_.empty() && !eventOverlapsAddrFilter(mev)) {
        if (stat_total_) stat_total_->addData(1);
        if (stat_clean_) stat_clean_->addData(1);
        return 0;
    }

    // Raw inject window (no region registry): confine to [start, start+len).
    // Prefer preserved vAddr; fall back to physical (e.g. balar H2D path).
    if (inject_addr_len_ > 0) {
        uint64_t a = mev->getVirtualAddress();
        if (a == 0) a = mev->getAddr();
        uint64_t sz = mev->getPayload().empty() ? 64u : mev->getPayload().size();
        if (a + sz <= inject_addr_start_ ||
            a >= inject_addr_start_ + inject_addr_len_) {
            if (stat_total_) stat_total_->addData(1);
            if (stat_clean_) stat_clean_->addData(1);
            return 0;
        }
    }

    int region_id = resolveRegionIdForEvent(mev);
    const std::string& region_name = regionNameForId(region_id);

    const EccPolicyEntry& entry = policy_.effectiveFor(kernel_name, region_name);

    auto& kernel_bucket = per_kernel_[kernel_name];
    auto& region_bucket = per_kernel_region_[std::make_pair(kernel_name, region_name)];

    auto countClean = [&]() {
        if (stat_total_) stat_total_->addData(1);
        if (stat_clean_) stat_clean_->addData(1);
        kernel_bucket.clean += 1;
        region_bucket.clean += 1;
    };

    if (entry.ber <= 0.0 && fault_event_rate_ <= 0.0
        && entry.scheme == EccScheme::NONE
        && fault_model_ != FaultModel::Campaign
        && fault_model_ != FaultModel::Resident) {
        countClean();
        return 0;
    }

    auto& payload = mev->getPayload();
    if (payload.empty()) {
        countClean();
        return 0;
    }

    uint32_t payload_bytes = static_cast<uint32_t>(payload.size());

    FaultDraw draw;
    if (fault_model_ == FaultModel::JedecMix) {
        // Documented priority (see 'fault_event_rate' param docs): an explicit
        // (or FIT-derived) event rate overrides BER; otherwise approximate the
        // per-access event probability as BER * payload_bits (Poisson approx).
        double rate = EccModelMath::jedecEventRate(
            fault_event_rate_, entry.ber, payload_bytes);
        draw = drawFaultJedecMix(payload_bytes, rate, entry.scheme);
    } else if (fault_model_ == FaultModel::Campaign) {
        draw = drawFaultCampaign(payload_bytes, entry.scheme, kernel_name);
    } else if (fault_model_ == FaultModel::Resident) {
        advanceResidentClock(getCurrentSimTimeNano());
        draw = drawFaultResident(mev, payload_bytes, entry.scheme);
    } else {
        draw = drawFaultPoisson(payload_bytes, entry.ber, entry.scheme);
    }

    EccLineOutcome line = draw.per_word_chip_errors.empty()
        ? aggregateLineOutcome(draw.per_word_errors, entry.scheme)
        : aggregateLineOutcomeChipAware(draw.per_word_errors,
                                        draw.per_word_chip_errors, entry.scheme);
    EccOutcome     outcome = line.outcome;

    // DUE response shared by DUE and Escape line outcomes (hardware fires per
    // word). drop_frame aborts; latency_only forwards poison by flipping the
    // DUE words' drawn error bits into the payload.
    auto handleDueWords = [&]() {
        if (line.due_words.empty()) return;
        if (due_action_ == DueAction::DropFrame) {
            requestFrameAbort();
            return;
        }
        unsigned hi = 0, lo = 0, flips = 0;
        for (uint32_t w : line.due_words) {
            flips += draw.exact_bits.empty()
                ? flipBitsInWord(mev, w, entry.scheme,
                                 draw.per_word_errors[w], hi, lo)
                : flipExactBitsInWord(mev, w, entry.scheme,
                                      draw.exact_bits, hi, lo);
        }
        due_poison_flips_total_ += flips;
        if (stat_due_poisoned_) stat_due_poisoned_->addData(flips);
        publishCumulative(state_key_, /*escapes*/0, /*flips*/flips);
    };

    uint64_t latency_ps = 0;
    bool high_blast_flip = false;
    switch (outcome) {
    case EccOutcome::Clean:
        latency_ps = 0;
        break;
    case EccOutcome::Correctable:
        latency_ps = entry.correctable_latency_ps;
        break;
    case EccOutcome::DetectableUncorrectable:
        latency_ps = entry.due_latency_ps;
        handleDueWords();
        break;
    case EccOutcome::SilentEscape: {
        latency_ps = entry.escape_latency_ps;
        // Corrupt only words whose ECC decode escaped (per_word_errors[w] bits).
        // Correctable words leak nothing; DUE words get the DUE response below.
        unsigned hi = 0, lo = 0, flips = 0;
        for (uint32_t w : line.escape_words) {
            flips += draw.exact_bits.empty()
                ? flipBitsInWord(mev, w, entry.scheme,
                                 draw.per_word_errors[w], hi, lo)
                : flipExactBitsInWord(mev, w, entry.scheme,
                                      draw.exact_bits, hi, lo);
        }
        escape_high_blast_total_ += hi;
        escape_low_blast_total_  += lo;
        if (hi && stat_escape_high_blast_) stat_escape_high_blast_->addData(hi);
        if (lo && stat_escape_low_blast_)  stat_escape_low_blast_->addData(lo);
        high_blast_flip = (hi > 0);
        publishCumulative(state_key_, /*escapes*/1, /*flips*/flips);
        publishPerFrameEscape(state_key_, kernel_name);
        handleDueWords();
        if (!line.due_words.empty() && entry.due_latency_ps > latency_ps)
            latency_ps = entry.due_latency_ps;
        break;
    }
    }

    if (stat_total_) stat_total_->addData(1);
    switch (outcome) {
    case EccOutcome::Clean:
        if (stat_clean_)       stat_clean_->addData(1);
        kernel_bucket.clean += 1;
        region_bucket.clean += 1;
        break;
    case EccOutcome::Correctable:
        if (stat_correctable_) stat_correctable_->addData(1);
        kernel_bucket.correctable += 1;
        region_bucket.correctable += 1;
        break;
    case EccOutcome::DetectableUncorrectable:
        if (stat_due_)         stat_due_->addData(1);
        kernel_bucket.due += 1;
        region_bucket.due += 1;
        break;
    case EccOutcome::SilentEscape:
        if (stat_escape_)      stat_escape_->addData(1);
        kernel_bucket.escape += 1;
        region_bucket.escape += 1;
        break;
    }
    if (latency_ps > 0) {
        if (stat_latency_) stat_latency_->addData(latency_ps);
        kernel_bucket.latency_ps += latency_ps;
        region_bucket.latency_ps += latency_ps;
    }
    if (verbose_ && (outcome != EccOutcome::Clean || high_blast_flip)) {
        out_->output("EccGuard '%s': addr=0x%llx vaddr=0x%llx kernel=%s region=%s mode=%s "
                     "errors=%u (escape_bits=%u over %zu words) outcome=%s "
                     "+%" PRIu64 " ps\n",
                     getName().c_str(),
                     (unsigned long long)mev->getAddr(),
                     (unsigned long long)mev->getVirtualAddress(),
                     kernel_name.empty() ? "UNKNOWN" : kernel_name.c_str(),
                     region_name.empty() ? "unlabeled" : region_name.c_str(),
                     faultModeName(draw.mode),
                     draw.num_errors, line.escape_bits,
                     draw.per_word_errors.size(),
                     eccOutcomeName(outcome), latency_ps);
    }

    return latency_ps;
}

unsigned EccGuard::flipBitsInWord(MemEvent* mev, uint32_t word_index,
                                  EccScheme scheme, unsigned nbits,
                                  unsigned& high_flips, unsigned& low_flips) {
    auto& payload = mev->getPayload();
    if (payload.empty() || nbits == 0) return 0;
    uint32_t total_bytes = static_cast<uint32_t>(payload.size());
    uint32_t wb    = eccWordBytes(scheme);
    uint32_t start = (wb == 0) ? 0 : word_index * wb;
    uint32_t end   = (wb == 0) ? total_bytes
                               : std::min(total_bytes, start + wb);
    if (start >= end) return 0;
    uint32_t span_bits = (end - start) * 8;
    if (nbits > span_bits) nbits = span_bits;

    unsigned elem_bytes = dtypeBytes(payload_dtype_);
    if (elem_bytes == 0) elem_bytes = 1;

    // Sample distinct bit positions inside the word span so repeated flips
    // cannot XOR-cancel. nbits << span_bits in practice, so rejection
    // sampling terminates quickly; the cap above guarantees termination.
    std::set<uint32_t> used;
    unsigned flipped = 0;
    while (flipped < nbits) {
        uint32_t bit = rng_.generateNextUInt32() % span_bits;
        if (!used.insert(bit).second) continue;
        uint32_t global_byte = start + bit / 8u;
        uint32_t bit_in_byte = bit % 8u;
        payload[global_byte] ^= static_cast<uint8_t>(1u << bit_in_byte);
        bool hi = false;
        if (payload_dtype_ != PayloadDtype::Bytes) {
            // Elements are little-endian and aligned to the payload start;
            // bit 0 of an element is its LSB (byte 0 of bf16 = mantissa low,
            // byte 1 = sign + exponent high).
            uint32_t bit_in_elem = (global_byte % elem_bytes) * 8u + bit_in_byte;
            hi = isHighBlastBit(payload_dtype_, bit_in_elem);
        }
        if (hi) ++high_flips; else ++low_flips;
        ++flipped;
    }
    return flipped;
}
