#include "node_canvas.h"
#include <algorithm>
#include <cmath>

// ── Font state ────────────────────────────────────────────────────────────────

static bool        s_fontLoaded = false;
static std::string s_fontName;

bool NodeCanvas::loadFont(const std::string& path, const std::string& familyName) {
    if (tvg::Text::load(path) != tvg::Result::Success) return false;
    s_fontName   = familyName;
    s_fontLoaded = true;
    return true;
}
bool        NodeCanvas::fontLoaded() { return s_fontLoaded; }
const std::string& NodeCanvas::fontName()  { return s_fontName; }

// ── NodeCanvas ────────────────────────────────────────────────────────────────

NodeCanvas::NodeCanvas(int w, int h) : cw(w), ch(h) {
    buf.assign(w * h, 0);
    canvas = tvg::SwCanvas::gen();
    canvas->target(buf.data(), w, w, h, tvg::SwCanvas::Colorspace::ARGB8888S);
}

void NodeCanvas::resize(int w, int h) {
    cw = w; ch = h;
    buf.assign(w * h, 0);
    canvas->target(buf.data(), w, w, h, tvg::SwCanvas::Colorspace::ARGB8888S);
}

// Clear trigger flags on output sockets only. Input triggers are overwritten
// each frame by propagate(), so they naturally clear without explicit reset.
static void clearTriggers(Widget* w) {
    if (auto* s = dynamic_cast<SocketWidget*>(w))
        if (s->dir == SocketDir::Output) s->value.trig = false;
    for (auto& c : w->children) clearTriggers(c.get());
}

void NodeCanvas::tick(float dt) {
    // 1. Clear stale one-frame triggers from last tick's propagation.
    for (auto& n : nodes) clearTriggers(n.get());

    // 2. Execute all nodes (read inputs → update state → write outputs).
    for (auto& n : nodes) n->tick(dt);

    // 3. Propagate output values along wires to downstream input sockets.
    wireLayer.propagate();
}

Node* NodeCanvas::addNode(std::unique_ptr<Node> n) {
    auto* raw = n.get();
    nodes.push_back(std::move(n));
    return raw;
}

void NodeCanvas::clear() {
    wireLayer.wires.clear();
    wireLayer.cancelDraft();
    nodes.clear();
    captured       = nullptr;
    isWireDrag     = false;
    capturedIsDrag = false;
}

// Walk a widget tree collecting the first SocketWidget that contains pt.
static SocketWidget* findSocket(Widget* w, Vec2 pt) {
    if (!w || !w->visible) return nullptr;
    if (auto* s = dynamic_cast<SocketWidget*>(w))
        if (s->contains(pt)) return s;
    for (auto& c : w->children)
        if (auto* found = findSocket(c.get(), pt)) return found;
    return nullptr;
}

SocketWidget* NodeCanvas::socketAt(Vec2 pt) const {
    for (auto it = nodes.rbegin(); it != nodes.rend(); ++it)
        if (auto* s = findSocket(it->get(), pt)) return s;
    return nullptr;
}

void NodeCanvas::render() {
    canvas->clear(true);
    auto root = tvg::Scene::gen();

    // Background (screen space, always fills window)
    {
        auto bg = tvg::Shape::gen();
        bg->appendRect(0, 0, (float)cw, (float)ch, 0, 0);
        bg->fill(pal::CanvasBg.r, pal::CanvasBg.g, pal::CanvasBg.b, 255);
        root->push(std::move(bg));
    }

    // World scene — everything here is in world space; camera transform applied below.
    {
        auto world = tvg::Scene::gen();

        // Grid lines: single shape; step doubles until lines are >= 20px apart on screen.
        if (showGrid) {
            const float baseStep = 64.f;
            float screenStep = baseStep * camZoom;
            while (screenStep < 48.f) screenStep *= 2.f;
            float step = screenStep / camZoom;

            float wx0 = std::floor((-camPan.x) / camZoom / step) * step;
            float wy0 = std::floor((-camPan.y) / camZoom / step) * step;
            float wx1 = ((float)cw - camPan.x) / camZoom;
            float wy1 = ((float)ch - camPan.y) / camZoom;

            auto grid = tvg::Shape::gen();
            for (float gx = wx0; gx <= wx1; gx += step) {
                grid->moveTo(gx, wy0);
                grid->lineTo(gx, wy1);
            }
            for (float gy = wy0; gy <= wy1; gy += step) {
                grid->moveTo(wx0, gy);
                grid->lineTo(wx1, gy);
            }
            grid->stroke(1.f);
            grid->stroke(45, 45, 55, 60);
            grid->fill(0, 0, 0, 0);
            world->push(std::move(grid));
        }

        wireLayer.draw(*world);
        for (auto& n : nodes) n->drawAll(*world);

        tvg::Matrix mat = {
            camZoom, 0.f,     camPan.x,
            0.f,     camZoom, camPan.y,
            0.f,     0.f,     1.f
        };
        world->transform(mat);
        root->push(std::move(world));
    }

    if (overlayFn) overlayFn(*root);

    canvas->push(std::move(root));
    canvas->draw();
    canvas->sync();
}

Widget* NodeCanvas::hitTest(Vec2 pt) {
    for (auto it = nodes.rbegin(); it != nodes.rend(); ++it)
        if (auto* h = (*it)->hitTest(pt)) return h;
    return nullptr;
}

void NodeCanvas::onMouseDown(float x, float y, int btn) {
    lastMouse = {x, y};

    // Middle mouse: start canvas pan.
    if (btn == 2) { isPanning = true; panButton = btn; return; }

    Vec2 pt = screenToWorld({x, y});

    // Right-click: disconnect socket, delete node, or remove wire.
    if (btn == 3) {
        if (auto* sock = socketAt(pt)) {
            wireLayer.disconnectSocket(sock);
        } else if (Widget* hit = hitTest(pt)) {
            Widget* w = hit;
            Node*   n = nullptr;
            while (w) { if ((n = dynamic_cast<Node*>(w))) break; w = w->parent; }
            if (n) {
                for (auto& child : n->children)
                    if (auto* s = dynamic_cast<SocketWidget*>(child.get()))
                        wireLayer.disconnectSocket(s);
                nodes.erase(std::remove_if(nodes.begin(), nodes.end(),
                    [n](const auto& owned){ return owned.get() == n; }), nodes.end());
                if (captured && (captured == n || captured->parent == n))
                    captured = nullptr;
            }
        } else {
            int idx = wireLayer.wireAt(pt);
            if (idx >= 0) wireLayer.removeWire(idx);
        }
        return;
    }

    // Socket click → wire drag (check before general hit test).
    if (auto* sock = socketAt(pt)) {
        wireLayer.beginDraft(sock, pt);
        isWireDrag = true;
        return;
    }

    Widget* hit = hitTest(pt);
    // Left-drag on empty canvas also pans — middle-click is unreliable on some
    // setups (WSLg forwards input over RDP; many touchpads have no middle button).
    if (!hit) { isPanning = true; panButton = btn; return; }

    if (auto* s = dynamic_cast<SocketWidget*>(hit)) {
        wireLayer.beginDraft(s, pt);
        isWireDrag = true;
        return;
    }

    captured = hit;
    capturedIsDrag = false;
    if (auto* node = dynamic_cast<Node*>(hit))
        capturedIsDrag = node->titleBarHit(pt);

    hit->onMouseDown(pt, btn);
}

void NodeCanvas::onMouseUp(float x, float y, int btn) {
    if (isPanning && btn == panButton) { isPanning = false; return; }

    Vec2 pt = screenToWorld({x, y});

    if (isWireDrag) {
        wireLayer.completeDraft(socketAt(pt));
        isWireDrag = false;
        return;
    }

    if (captured) {
        captured->onMouseUp(pt, btn);
        captured = nullptr;
    }
}

void NodeCanvas::onMouseMove(float x, float y, float dx, float dy) {
    lastMouse = {x, y};

    if (isPanning) {
        camPan.x += dx;
        camPan.y += dy;
        return;
    }

    Vec2 pt         = screenToWorld({x, y});
    Vec2 worldDelta = {dx / camZoom, dy / camZoom};

    if (isWireDrag) {
        wireLayer.updateDraft(pt);
        return;
    }

    if (!captured) return;

    if (auto* node = dynamic_cast<Node*>(captured)) {
        if (capturedIsDrag) node->onDrag(worldDelta);
        return;
    }

    captured->onMouseMove(pt, worldDelta);
}

void NodeCanvas::onMouseWheel(float x, float y, float delta) {
    Vec2  cursor = {x, y};
    Vec2  world  = screenToWorld(cursor);
    float factor = delta > 0.f ? 1.1f : 1.f / 1.1f;
    camZoom = std::clamp(camZoom * factor, 0.35f, 3.f);
    camPan.x = cursor.x - world.x * camZoom;
    camPan.y = cursor.y - world.y * camZoom;
}
