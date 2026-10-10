#pragma once
#include "node.h"
#include <functional>
#include <map>
#include <string>
#include <vector>

// Functional grouping of node types — drives palette ordering and colour.
enum class NodeCategory { Sound, Timing, Sequence, Utility, Display };

inline const char* categoryName(NodeCategory c) {
    switch (c) {
        case NodeCategory::Sound:    return "Sound";
        case NodeCategory::Timing:   return "Timing";
        case NodeCategory::Sequence: return "Sequence";
        case NodeCategory::Utility:  return "Utility";
        case NodeCategory::Display:  return "Display";
    }
    return "";
}

inline Color4 categoryColor(NodeCategory c) {
    switch (c) {
        case NodeCategory::Sound:    return pal::CatSound;
        case NodeCategory::Timing:   return pal::CatTiming;
        case NodeCategory::Sequence: return pal::CatSequence;
        case NodeCategory::Utility:  return pal::CatUtility;
        case NodeCategory::Display:  return pal::CatDisplay;
    }
    return pal::CatUtility;
}

// A registered node type: display label, category + factory.
struct NodeTypeInfo {
    std::string  label;
    NodeCategory category;
    std::function<std::unique_ptr<Node>(Vec2)> create;
};

// Global registry: maps type-name string → NodeTypeInfo.
// Populated at startup; used by the palette and by graph_io for load.
class NodeRegistry {
public:
    static NodeRegistry& get() {
        static NodeRegistry inst;
        return inst;
    }

    void reg(std::string typeName, std::string label, NodeCategory category,
             std::function<std::unique_ptr<Node>(Vec2)> fn) {
        order.push_back(typeName);
        // Wrap the factory so every node created from the registry (palette or load)
        // carries its category colour.
        auto create = [fn = std::move(fn), category](Vec2 pos) {
            auto n = fn(pos);
            if (n) n->accent = categoryColor(category);
            return n;
        };
        types[std::move(typeName)] = {std::move(label), category, std::move(create)};
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
