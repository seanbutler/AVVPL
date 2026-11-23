#include "graph_io.h"
#include "node_registry.h"
#include "socket.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <cstdio>
#include <unordered_map>

using json = nlohmann::json;

// ── helpers ───────────────────────────────────────────────────────────────────

static std::string guidToHex(uint64_t g) {
    char buf[17];
    std::snprintf(buf, sizeof(buf), "%016llx", (unsigned long long)g);
    return buf;
}

static uint64_t hexToGuid(const std::string& s) {
    return (uint64_t)std::stoull(s, nullptr, 16);
}

// Collect all sockets (children that are SocketWidget) in order.
static std::vector<SocketWidget*> nodeSockets(Node* n) {
    std::vector<SocketWidget*> out;
    for (auto& c : n->children)
        if (auto* s = dynamic_cast<SocketWidget*>(c.get())) out.push_back(s);
    return out;
}

// Collect all value widgets (non-socket children that serialise something) in order.
// Display-only widgets (toJson() == null) are excluded so save and load index the
// same list — otherwise a display before a slider shifts every value by one.
static std::vector<Widget*> nodeWidgets(Node* n) {
    std::vector<Widget*> out;
    for (auto& c : n->children)
        if (!dynamic_cast<SocketWidget*>(c.get()) && !c->toJson().is_null())
            out.push_back(c.get());
    return out;
}

// ── save ──────────────────────────────────────────────────────────────────────

bool graphSave(const NodeCanvas& nc, const std::string& path) {
    json root;
    json jnodes = json::array();

    for (auto& owned : nc.allNodes()) {
        Node* n = owned.get();
        json jn;
        jn["type"] = n->title;
        jn["guid"] = guidToHex(n->guid);
        jn["x"]    = n->localPos.x;
        jn["y"]    = n->localPos.y;

        // Socket default values (input sockets carry user-set defaults)
        json jsocks = json::array();
        for (auto* s : nodeSockets(n))
            jsocks.push_back({{"dir", s->dir == SocketDir::Input ? "in" : "out"},
                              {"idx", s->index},
                              {"val", s->toJson()}});
        jn["sockets"] = jsocks;

        // Value widget states (sliders, dials, histograms, ProgressRing ignored)
        json jwidgets = json::array();
        for (auto* w : nodeWidgets(n))
            jwidgets.push_back(w->toJson());
        jn["widgets"] = jwidgets;

        jnodes.push_back(jn);
    }
    root["nodes"] = jnodes;

    // Wires: (from_node_guid, from_sock_idx, to_node_guid, to_sock_idx)
    json jwires = json::array();
    for (auto& w : nc.wires().wires) {
        std::string fg, tg;
        int fi = -1, ti = -1;
        for (auto& owned : nc.allNodes()) {
            for (auto* s : nodeSockets(owned.get())) {
                if (s == w.from) { fg = guidToHex(owned->guid); fi = s->index; }
                if (s == w.to)   { tg = guidToHex(owned->guid); ti = s->index; }
            }
        }
        if (!fg.empty() && !tg.empty())
            jwires.push_back({{"fg", fg}, {"fi", fi}, {"tg", tg}, {"ti", ti}});
    }
    root["wires"] = jwires;

    std::ofstream f(path);
    if (!f) return false;
    f << root.dump(2);
    return true;
}

// ── load ──────────────────────────────────────────────────────────────────────

// Shared implementation — merges JSON graph data into nc without clearing first.
static bool loadGraphData(NodeCanvas& nc, const json& root) {
    auto& reg = NodeRegistry::get();

    // When merging into a non-empty canvas, shift incoming nodes to the right
    // of all existing content so they don't land on top.
    float offsetX = 0.f;
    if (!nc.allNodes().empty() && root.contains("nodes") && !root["nodes"].empty()) {
        float existMaxX = -1e9f;
        for (auto& owned : nc.allNodes()) {
            float right = owned->localPos.x + owned->width;
            if (right > existMaxX) existMaxX = right;
        }
        float incomingMinX = 1e9f;
        for (auto& jn : root["nodes"])
            incomingMinX = std::min(incomingMinX, jn.value("x", 0.f));
        offsetX = existMaxX + 80.f - incomingMinX;
    }

    for (auto& jn : root["nodes"]) {
        std::string type = jn["type"].get<std::string>();
        Vec2 pos = {jn.value("x", 0.f) + offsetX, jn.value("y", 0.f)};

        auto* info = reg.find(type);
        if (!info) continue;   // unknown type — skip

        auto owned = info->create(pos);
        Node* n    = owned.get();

        // Restore GUID so wires can reference this node by its saved identity.
        // Old saves without a "guid" field keep the freshly generated one.
        if (jn.contains("guid"))
            n->guid = hexToGuid(jn["guid"].get<std::string>());

        // Restore socket defaults
        if (jn.contains("sockets")) {
            auto socks = nodeSockets(n);
            for (auto& js : jn["sockets"]) {
                SocketDir dir = js["dir"] == "in" ? SocketDir::Input : SocketDir::Output;
                int idx       = js["idx"].get<int>();
                for (auto* s : socks) {
                    if (s->dir == dir && s->index == idx) {
                        s->fromJson(js["val"]);
                        break;
                    }
                }
            }
        }

        // Restore widget values (in order)
        if (jn.contains("widgets")) {
            auto wgets = nodeWidgets(n);
            size_t wi  = 0;
            for (auto& jw : jn["widgets"]) {
                if (wi >= wgets.size()) break;
                wgets[wi++]->fromJson(jw);
            }
        }

        nc.addNode(std::move(owned));
    }

    // Build GUID → Node* map for wire resolution.
    auto& nodes = nc.allNodes();
    std::unordered_map<uint64_t, Node*> guidMap;
    for (auto& owned : nodes) guidMap[owned->guid] = owned.get();

    // Re-create wires. New format uses GUIDs ("fg"/"tg"); old format uses
    // array indices ("fn"/"tn") and is still accepted for backward compat.
    for (auto& jw : root["wires"]) {
        int fi = jw["fi"], ti = jw["ti"];
        Node* fromNode = nullptr;
        Node* toNode   = nullptr;

        if (jw.contains("fg")) {
            auto itF = guidMap.find(hexToGuid(jw["fg"].get<std::string>()));
            auto itT = guidMap.find(hexToGuid(jw["tg"].get<std::string>()));
            if (itF != guidMap.end()) fromNode = itF->second;
            if (itT != guidMap.end()) toNode   = itT->second;
        } else {
            int fn = jw["fn"], tn = jw["tn"];
            if (fn < (int)nodes.size()) fromNode = nodes[fn].get();
            if (tn < (int)nodes.size()) toNode   = nodes[tn].get();
        }

        if (!fromNode || !toNode) continue;

        SocketWidget* from = nullptr;
        SocketWidget* to   = nullptr;
        for (auto* s : nodeSockets(fromNode))
            if (s->dir == SocketDir::Output && s->index == fi) { from = s; break; }
        for (auto* s : nodeSockets(toNode))
            if (s->dir == SocketDir::Input  && s->index == ti) { to   = s; break; }

        if (from && to) nc.wires().connect(from, to);
    }

    return true;
}

bool graphLoad(NodeCanvas& nc, const std::string& path) {
    std::ifstream f(path);
    if (!f) return false;
    json root;
    try { root = json::parse(f); } catch (...) { return false; }
    nc.clear();
    return loadGraphData(nc, root);
}

bool graphLoadAdditive(NodeCanvas& nc, const std::string& path) {
    std::ifstream f(path);
    if (!f) return false;
    json root;
    try { root = json::parse(f); } catch (...) { return false; }
    return loadGraphData(nc, root);
}

// ── duplicate ─────────────────────────────────────────────────────────────────

void duplicateNode(NodeCanvas& nc, Node* n, float dx, float dy) {
    auto& reg  = NodeRegistry::get();
    auto* info = reg.find(n->title);
    if (!info) return;

    auto  owned = info->create({ n->localPos.x + dx, n->localPos.y + dy });
    Node* copy  = owned.get();

    // Copy socket defaults
    auto srcSocks = nodeSockets(n);
    auto dstSocks = nodeSockets(copy);
    for (auto* ss : srcSocks)
        for (auto* ds : dstSocks)
            if (ds->dir == ss->dir && ds->index == ss->index)
                { ds->fromJson(ss->toJson()); break; }

    // Copy widget values
    auto srcW = nodeWidgets(n);
    auto dstW = nodeWidgets(copy);
    for (size_t i = 0; i < srcW.size() && i < dstW.size(); ++i)
        dstW[i]->fromJson(srcW[i]->toJson());

    nc.addNode(std::move(owned));
}
