// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef CARCOSA_PMDATAREGISTRY_H
#define CARCOSA_PMDATAREGISTRY_H

#include <sst/core/event.h>
#include <sst/core/serialization/serializable.h>
#include <sst/core/serialization/serializer.h>
#include <map>
#include <string>
#include <vector>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <cstdint>

namespace SST {
namespace Carcosa {

/**
 * Parsed Port Module command: "<command> [param1 [param2 ...]]".
 * Parameters are kept as strings and converted on-demand via getParam<T>().
 */
struct PMData {
    std::string command;
    std::vector<std::string> params;

    PMData() : command("") {}
    PMData(const std::string& cmd) : command(cmd) {}
    PMData(const std::string& cmd, const std::vector<std::string>& p) : command(cmd), params(p) {}

    size_t paramCount() const { return params.size(); }

    bool hasParam(size_t index) const { return index < params.size(); }

    /** Throws std::out_of_range on bad index, std::invalid_argument on conversion failure. */
    template<typename T>
    T getParam(size_t index) const {
        if (index >= params.size()) {
            throw std::out_of_range("PMData::getParam: index " + std::to_string(index) +
                                    " out of range (size=" + std::to_string(params.size()) + ")");
        }
        return convertParam<T>(params[index]);
    }

    /** Returns defaultValue if index is missing or conversion fails. */
    template<typename T>
    T getParamOr(size_t index, T defaultValue) const {
        if (index >= params.size()) {
            return defaultValue;
        }
        try {
            return convertParam<T>(params[index]);
        } catch (...) {
            return defaultValue;
        }
    }

    /** Backward compat: first parameter as double (0.0 if missing). */
    double getValue() const {
        return getParamOr<double>(0, 0.0);
    }

    static PMData parse(const std::string& str) {
        PMData data;
        std::istringstream iss(str);

        if (!(iss >> data.command)) {
            return data;
        }

        std::string param;
        while (iss >> param) {
            data.params.push_back(param);
        }

        return data;
    }

private:
    template<typename T>
    static T convertParam(const std::string& str) {
        T value;
        std::istringstream iss(str);
        if (!(iss >> value)) {
            throw std::invalid_argument("PMData: cannot convert '" + str + "'");
        }
        return value;
    }
};

template<>
inline std::string PMData::convertParam<std::string>(const std::string& str) {
    return str;
}

template<>
inline double PMData::convertParam<double>(const std::string& str) {
    try {
        return std::stod(str);
    } catch (...) {
        throw std::invalid_argument("PMData: cannot convert '" + str + "' to double");
    }
}

template<>
inline float PMData::convertParam<float>(const std::string& str) {
    try {
        return std::stof(str);
    } catch (...) {
        throw std::invalid_argument("PMData: cannot convert '" + str + "' to float");
    }
}

template<>
inline int PMData::convertParam<int>(const std::string& str) {
    try {
        // base 0 allows hex (0x) and octal (0) prefixes
        return std::stoi(str, nullptr, 0);
    } catch (...) {
        throw std::invalid_argument("PMData: cannot convert '" + str + "' to int");
    }
}

template<>
inline long PMData::convertParam<long>(const std::string& str) {
    try {
        return std::stol(str, nullptr, 0);
    } catch (...) {
        throw std::invalid_argument("PMData: cannot convert '" + str + "' to long");
    }
}

template<>
inline unsigned long PMData::convertParam<unsigned long>(const std::string& str) {
    try {
        return std::stoul(str, nullptr, 0);
    } catch (...) {
        throw std::invalid_argument("PMData: cannot convert '" + str + "' to unsigned long");
    }
}

template<>
inline long long PMData::convertParam<long long>(const std::string& str) {
    try {
        return std::stoll(str, nullptr, 0);
    } catch (...) {
        throw std::invalid_argument("PMData: cannot convert '" + str + "' to long long");
    }
}

template<>
inline unsigned long long PMData::convertParam<unsigned long long>(const std::string& str) {
    try {
        return std::stoull(str, nullptr, 0);
    } catch (...) {
        throw std::invalid_argument("PMData: cannot convert '" + str + "' to unsigned long long");
    }
}

template<>
inline bool PMData::convertParam<bool>(const std::string& str) {
    // Accept "true", "1", "yes" (any case) as true; everything else is false.
    return (str == "true" || str == "1" || str == "yes" ||
            str == "True" || str == "TRUE" || str == "Yes" || str == "YES");
}

/** Message from a PortModule to a FaultInjManager (queued inside PMDataRegistry). */
struct ManagerMessage {
    enum class Type { RegisterPM };
    Type type;
    std::string pmId;

    ManagerMessage() : type(Type::RegisterPM), pmId("") {}
    ManagerMessage(Type t, const std::string& id) : type(t), pmId(id) {}
    static ManagerMessage makeRegisterPM(const std::string& id) {
        return ManagerMessage(Type::RegisterPM, id);
    }

    void serialize_order(SST::Core::Serialization::serializer& ser) {
        SST_SER(type);
        SST_SER(pmId);
    }
};

/**
 * Per-manager registry: maps event IDs to PM command strings, plus a queue of
 * PortModule -> Manager messages (e.g. RegisterPM). Look up by id via PMRegistryResolver.
 */
class PMDataRegistry {
public:
    PMDataRegistry() = default;

    void serialize_order(SST::Core::Serialization::serializer& ser) {
        SST_SER(registry_);
        SST_SER(managerQueue_);
    }

    void registerPMData(SST::Event::id_type id, const std::string& data) {
        registry_[id] = data;
    }

    bool hasPMData(SST::Event::id_type id) const {
        return registry_.find(id) != registry_.end();
    }

    std::string lookupRaw(SST::Event::id_type id) const {
        auto it = registry_.find(id);
        return (it != registry_.end()) ? it->second : "";
    }

    PMData lookupPMData(SST::Event::id_type id) const {
        auto it = registry_.find(id);
        return (it != registry_.end()) ? PMData::parse(it->second) : PMData();
    }

    void clearPMData(SST::Event::id_type id) {
        registry_.erase(id);
    }

    void clearAll() {
        registry_.clear();
    }

    void pushMessageToManager(const ManagerMessage& msg) {
        managerQueue_.push(msg);
    }

    /** Drain and return all messages; the queue is cleared. */
    std::vector<ManagerMessage> popAllMessagesFromPMs() {
        std::vector<ManagerMessage> out;
        while (!managerQueue_.empty()) {
            out.push_back(managerQueue_.front());
            managerQueue_.pop();
        }
        return out;
    }

private:
    std::map<SST::Event::id_type, std::string> registry_;
    std::queue<ManagerMessage> managerQueue_;
};

/** Global id -> PMDataRegistry* map; FaultInjManager registers, FaultInjectorMemH looks up. */
class PMRegistryResolver {
public:
    static void registerRegistry(const std::string& id, PMDataRegistry* reg) {
        registryMap_[id] = reg;
    }

    static PMDataRegistry* getRegistry(const std::string& id) {
        auto it = registryMap_.find(id);
        return (it != registryMap_.end()) ? it->second : nullptr;
    }

    static void unregisterRegistry(const std::string& id) {
        registryMap_.erase(id);
    }

private:
    static std::map<std::string, PMDataRegistry*> registryMap_;
};

inline std::map<std::string, PMDataRegistry*> PMRegistryResolver::registryMap_;

} // namespace Carcosa
} // namespace SST

#endif // CARCOSA_PMDATAREGISTRY_H
