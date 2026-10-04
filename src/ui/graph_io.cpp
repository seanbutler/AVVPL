#include "graph_io.h"
#include "node_registry.h"
#include "socket.h"
#include <nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;

// ── helpers ───────────────────────────────────────────────────────────────────

static int nodeIndex(const NodeCanvas& nc, const Node* n) {
    int i = 0;
    for (auto& owned : nc.allNodes()) {
        if (owned.get() == n) return i;
        ++i;
    }
    return -1;
}

// Collect all sockets (children that are SocketWidget) in order.
static std::vector<SocketWidget*> nodeSockets(Node* n) {
    std::vector<SocketWidget*> out;
    for (auto& c : n->children)
        if (auto* s = dynamic_cast<SocketWidget*>(c.get())) out.push_back(s);
    return out;
}

// Collect all value widgets (non-socket children) in order.
static std::vector<Widget*> nodeWidgets(Node* n) {
    std::vector<Widget*> out;
    for (auto& c : n->children)
        if (!dynamic_cast<SocketWidget*>(c.get())) out.push_back(c.get());
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
        for (auto* w : nodeWidgets(n)) {
            json jw = w->toJson();
            if (!jw.is_null()) jwidgets.push_back(jw);
        }
        jn["widgets"] = jwidgets;

        jnodes.push_back(jn);
    }
    root["nodes"] = jnodes;

    // Wires: (from_node_idx, from_sock_idx, to_node_idx, to_sock_idx)
    json jwires = json::array();
    for (auto& w : nc.wires().wires) {
        // Find which node owns each socket
        int fn = -1, tn = -1, fi = -1, ti = -1;
        int ni = 0;
        for (auto& owned : nc.allNodes()) {
            for (auto* s : nodeSockets(owned.get())) {
                if (s == w.from) { fn = ni; fi = s->index; }
                if (s == w.to)   { tn = ni; ti = s->index; }
            }
            ++ni;
        }
        if (fn >= 0 && tn >= 0)
            jwires.push_back({{"fn", fn}, {"fi", fi}, {"tn", tn}, {"ti", ti}});
    }
    root["wires"] = jwires;

    std::ofstream f(path);
    if (!f) return false;
    f << root.dump(2);
    return true;
}

// ── load ──────────────────────────────────────────────────────────────────────

bool graphLoad(NodeCanvas& nc, const std::string& path) {
    std::ifstream f(path);
    if (!f) return false;
    json root;
    try { root = json::parse(f); }
    catch (...) { return false; }

    nc.clear();

    auto& reg = NodeRegistry::get();

    for (auto& jn : root["nodes"]) {
        std::string type = jn["type"].get<std::string>();
        Vec2 pos = {jn.value("x", 0.f), jn.value("y", 0.f)};

        auto* info = reg.find(type);
        if (!info) continue;   // unknown type — skip

        auto owned = info->create(pos);
        Node* n    = owned.get();

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

        // Restore widget values (in order, skipping null entries)
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

    // Re-create wires by matching node indices and socket indices
    auto& nodes = nc.allNodes();
    for (auto& jw : root["wires"]) {
        int fn = jw["fn"], fi = jw["fi"], tn = jw["tn"], ti = jw["ti"];
        if (fn >= (int)nodes.size() || tn >= (int)nodes.size()) continue;

        SocketWidget* from = nullptr;
        SocketWidget* to   = nullptr;
        for (auto* s : nodeSockets(nodes[fn].get()))
            if (s->dir == SocketDir::Output && s->index == fi) { from = s; break; }
        for (auto* s : nodeSockets(nodes[tn].get()))
            if (s->dir == SocketDir::Input  && s->index == ti) { to   = s; break; }

        if (from && to) nc.wires().connect(from, to);
    }

    return true;
}
