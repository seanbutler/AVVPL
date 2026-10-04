#include "wire_layer.h"
#include "socket.h"
#include "widget.h"
#include <cmath>

// ── helpers ───────────────────────────────────────────────────────────────────

static void gatherSockets(Widget* w, Vec2 worldPt, SocketWidget*& result) {
    if (result || !w) return;
    if (auto* s = dynamic_cast<SocketWidget*>(w)) {
        if (s->contains(worldPt)) { result = s; return; }
    }
    for (auto& c : w->children)
        gatherSockets(c.get(), worldPt, result);
}

// ── WireLayer ─────────────────────────────────────────────────────────────────

void WireLayer::beginDraft(SocketWidget* from, Vec2 mousePos) {
    draftFrom = from;
    draftEnd  = mousePos;
    hasDraft  = true;
}

void WireLayer::updateDraft(Vec2 mousePos) {
    draftEnd = mousePos;
}

SocketWidget* WireLayer::completeDraft(SocketWidget* drop) {
    if (!hasDraft || !draftFrom || !drop || drop == draftFrom) {
        cancelDraft();
        return nullptr;
    }
    SocketWidget* out = nullptr;
    SocketWidget* in  = nullptr;
    if (draftFrom->dir == SocketDir::Output && drop->dir == SocketDir::Input) {
        out = draftFrom; in = drop;
    } else if (draftFrom->dir == SocketDir::Input && drop->dir == SocketDir::Output) {
        out = drop; in = draftFrom;
    } else {
        cancelDraft();
        return nullptr;
    }
    cancelDraft();
    connect(out, in);
    return in;
}

void WireLayer::cancelDraft() {
    draftFrom = nullptr;
    hasDraft  = false;
}

void WireLayer::connect(SocketWidget* from, SocketWidget* to) {
    // Input sockets accept only one wire; remove any existing.
    wires.erase(std::remove_if(wires.begin(), wires.end(),
        [to](const Wire& w){ return w.to == to; }), wires.end());
    wires.push_back({from, to});
    if (to->onConnect) to->onConnect();
}

void WireLayer::disconnectSocket(SocketWidget* sock) {
    wires.erase(std::remove_if(wires.begin(), wires.end(),
        [sock](const Wire& w){ return w.from == sock || w.to == sock; }), wires.end());
}

SocketWidget* WireLayer::findSocketAt(Vec2 worldPt,
    const std::vector<std::unique_ptr<Widget>>& roots) const
{
    SocketWidget* result = nullptr;
    for (auto& root : roots)
        gatherSockets(root.get(), worldPt, result);
    return result;
}

void WireLayer::drawWire(tvg::Scene& scene, Vec2 a, Vec2 b, Color4 c, float sw) {
    float dx = std::abs(b.x - a.x) * 0.5f + 30.f;
    auto sh = tvg::Shape::gen();
    sh->moveTo(a.x, a.y);
    sh->cubicTo(a.x + dx, a.y, b.x - dx, b.y, b.x, b.y);
    sh->stroke(sw);
    sh->stroke(c.r, c.g, c.b, c.a);
    sh->fill(0, 0, 0, 0);
    scene.push(std::move(sh));
}

void WireLayer::draw(tvg::Scene& scene) const {
    for (auto& w : wires)
        drawWire(scene, w.from->worldCenter(), w.to->worldCenter(), pal::WireColor, 2.f);
    if (hasDraft && draftFrom)
        drawWire(scene, draftFrom->worldCenter(), draftEnd, pal::WireDraft, 1.5f);
}
