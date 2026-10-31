// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.

#ifndef SST_ELEMENTS_CARCOSA_COMPONENT_TEST_BOUNDS_H
#define SST_ELEMENTS_CARCOSA_COMPONENT_TEST_BOUNDS_H

#include <sst/core/output.h>

#include <cinttypes>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace SST::Carcosa {

class ComponentTestBounds {
public:
    void add(std::string name, int64_t minimum, int64_t maximum) {
        bounds_.push_back({std::move(name), minimum, maximum, 0});
    }

    void addExact(std::string name, int64_t expected) {
        add(std::move(name), expected, expected);
    }

    void set(const std::string& name, uint64_t value) {
        for (auto& bound : bounds_) {
            if (bound.name == name) {
                bound.value = value;
                return;
            }
        }
    }

    bool check(SST::Output& out, const std::string& component) const {
        bool valid = true;
        for (const auto& bound : bounds_) {
            if ((bound.minimum >= 0 && bound.value < static_cast<uint64_t>(bound.minimum)) ||
                (bound.maximum >= 0 && bound.value > static_cast<uint64_t>(bound.maximum))) {
                out.output("%s: FAIL %s=%" PRIu64 " outside [%" PRId64 ",%" PRId64 "].\n",
                           component.c_str(), bound.name.c_str(), bound.value,
                           bound.minimum, bound.maximum);
                valid = false;
            }
        }
        return valid;
    }

private:
    struct Bound {
        std::string name;
        int64_t minimum;
        int64_t maximum;
        uint64_t value;
    };

    std::vector<Bound> bounds_;
};

} // namespace SST::Carcosa

#endif
