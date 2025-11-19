// Copyright 2013-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_MERLIN_EXTENDEDREQUEST_H
#define COMPONENTS_MERLIN_EXTENDEDREQUEST_H

#include "sst/core/interfaces/simpleNetwork.h"
#include <map>
#include <string>
#include <memory>
#include <vector>

namespace SST {
namespace Merlin {

// Base class for plugin metadata
class PluginMetadata {
public:
    virtual ~PluginMetadata() {}
    virtual PluginMetadata* clone() const = 0;
};

// Templated metadata for type-safe storage
template<typename T>
class TypedMetadata : public PluginMetadata {
public:
    T data;

    TypedMetadata(const T& d) : data(d) {}

    PluginMetadata* clone() const override {
        return new TypedMetadata<T>(data);
    }
};

// Extended Request that plugins can attach metadata to
class ExtendedRequest : public SST::Interfaces::SimpleNetwork::Request {
private:
    // Map from plugin name/key to metadata
    std::map<std::string, std::shared_ptr<PluginMetadata>> metadata;

    ImplementSerializable(SST::Merlin::ExtendedRequest)

public:
    ExtendedRequest() : Request() {}

    ExtendedRequest(SST::Interfaces::SimpleNetwork::nid_t dest,
        SST::Interfaces::SimpleNetwork::nid_t src, size_t size_in_bits,
                    bool head, bool tail, Event* payload = nullptr) :
        Request(dest, src, size_in_bits, head, tail, payload) {}

    // Create ExtendedRequest from a base Request
    ExtendedRequest(Request* req) :
        Request(req->dest, req->src, req->size_in_bits, req->head, req->tail)
    {
        givePayload(req->takePayload());
        trace = req->getTraceType();
        traceID = req->getTraceID();

        // If req is already an ExtendedRequest, copy its metadata
        ExtendedRequest* ext_req = dynamic_cast<ExtendedRequest*>(req);
        if (ext_req) {
            // Deep copy metadata
            for (const auto& pair : ext_req->metadata) {
                metadata[pair.first] = std::shared_ptr<PluginMetadata>(pair.second->clone());
            }
        }
    }

    ~ExtendedRequest() {}

    // Set plugin-specific metadata
    template<typename T>
    void setMetadata(const std::string& key, const T& data) {
        metadata[key] = std::make_shared<TypedMetadata<T>>(data);
    }

    // Get plugin-specific metadata
    // Returns true if metadata exists and was retrieved, false otherwise
    template<typename T>
    bool getMetadata(const std::string& key, T& data) const {
        auto it = metadata.find(key);
        if (it == metadata.end()) return false;

        TypedMetadata<T>* typed = dynamic_cast<TypedMetadata<T>*>(it->second.get());
        if (!typed) return false;

        data = typed->data;
        return true;
    }

    // Get a const pointer to plugin-specific metadata without copying
    // Returns nullptr if metadata doesn't exist or type mismatch
    template<typename T>
    T* getMetadataPtr(const std::string& key) const {
        auto it = metadata.find(key);
        if (it == metadata.end()) return nullptr;

        TypedMetadata<T>* typed = dynamic_cast<TypedMetadata<T>*>(it->second.get());
        if (!typed) return nullptr;

        return &(typed->data);
    }

    // Check if metadata exists for a given key
    bool hasMetadata(const std::string& key) const {
        return metadata.find(key) != metadata.end();
    }

    // Remove metadata for a given key
    void removeMetadata(const std::string& key) {
        metadata.erase(key);
    }

    // Clear all metadata
    void clearMetadata() {
        metadata.clear();
    }

    void serialize_order(SST::Core::Serialization::serializer &ser) override {
        Request::serialize_order(ser);
        // Note: Metadata serialization would need to be implemented if
        // ExtendedRequest needs to be sent across MPI ranks
        // For now, metadata is assumed to be used only within a single rank
    }
};

// Common metadata structures used by plugins

// Source Routing metadata
struct SourceRoutingMetadata {
    std::deque<int> path = {};

    SourceRoutingMetadata() {}
    SourceRoutingMetadata(const std::deque<int>& p) : path(p) {}
};

// Reorder metadata
struct ReorderMetadata {
    uint32_t seq_number;

    ReorderMetadata() : seq_number(0) {}
    ReorderMetadata(uint32_t seq) : seq_number(seq) {}
};

} // namespace Merlin
} // namespace SST

#endif
