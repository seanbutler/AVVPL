#pragma once
#include "node.h"
#include <functional>
#include <map>
#include <string>
#include <vector>

// A registered node type: display label + factory.
struct NodeTypeInfo {
    std::string label;
    Color4      color;   // palette button tint
    std::function<std::unique_ptr<Node>(Vec2)> create;
    bool        stub = false;  // true = placeholder, no real processing
};

// Global registry: maps type-name string → NodeTypeInfo.
// Populated at startup; used by the palette and by graph_io for load.
class NodeRegistry {
public:
    static NodeRegistry& get() {
        static NodeRegistry inst;
        return inst;
    }

    void reg(std::string typeName, std::string label, Color4 color,
             std::function<std::unique_ptr<Node>(Vec2)> fn,
             bool stub = false) {
        order.push_back(typeName);
        types[std::move(typeName)] = {std::move(label), color, std::move(fn), stub};
    }

    const NodeTypeInfo* find(const std::string& name) const {
        auto it = types.find(name);
        return it != types.end() ? &it->second : nullptr;
    }

    // Ordered list of type names for the palette.
    const std::vector<std::string>& typeOrder() const { return order; }

private:
    std::map<std::string, NodeTypeInfo> types;
    std::vector<std::string>            order;
};
