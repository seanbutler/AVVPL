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
    to->connected = true;
    if (to->onConnect) to->onConnect();
}

// An input that loses its wire stops being refreshed by propagate(), so clear its
// connected flag and any trigger it was last handed — otherwise the trigger sticks.
static void detachInput(SocketWidget* in) {
    in->connected  = false;
    in->value.trig = false;
}

void WireLayer::disconnectSocket(SocketWidget* sock) {
    for (auto& w : wires)
        if (w.from == sock || w.to == sock) detachInput(w.to);
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

int WireLayer::wireAt(Vec2 pt, float threshold) const {
    int   best  = -1;
    float bestD = threshold;
    for (int i = 0; i < (int)wires.size(); ++i) {
        Vec2  a  = wires[i].from->worldCenter();
        Vec2  b  = wires[i].to->worldCenter();
        float dx = std::abs(b.x - a.x) * 0.5f + 30.f;
        // Sample cubic bezier at 32 points
        for (int s = 0; s <= 32; ++s) {
            float t  = s / 32.f;
            float mt = 1.f - t;
            // Control points: a, a+(dx,0), b-(dx,0), b
            float cx = a.x + dx, cy = a.y;
            float dx2 = b.x - dx, dy2 = b.y;
            float bx = mt*mt*mt*a.x + 3*mt*mt*t*cx + 3*mt*t*t*dx2 + t*t*t*b.x;
            float by = mt*mt*mt*a.y + 3*mt*mt*t*cy + 3*mt*t*t*dy2 + t*t*t*b.y;
            float d  = std::sqrt((pt.x-bx)*(pt.x-bx) + (pt.y-by)*(pt.y-by));
            if (d < bestD) { bestD = d; best = i; }
        }
    }
    return best;
}

void WireLayer::removeWire(int idx) {
    if (idx < 0 || idx >= (int)wires.size()) return;
    detachInput(wires[idx].to);
    wires.erase(wires.begin() + idx);
}

void WireLayer::propagate() {
    for (auto& w : wires)
        w.to->value = w.from->value;
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
