#include "node_canvas.h"

NodeCanvas::NodeCanvas(int w, int h) : cw(w), ch(h) {
    buf.assign(w * h, 0);
    canvas = tvg::SwCanvas::gen();
    canvas->target(buf.data(), w, w, h, tvg::SwCanvas::Colorspace::ARGB8888S);
}

Node* NodeCanvas::addNode(std::unique_ptr<Node> n) {
    auto* raw = n.get();
    nodes.push_back(std::move(n));
    return raw;
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

    // Background
    {
        auto bg = tvg::Shape::gen();
        bg->appendRect(0, 0, (float)cw, (float)ch, 0, 0);
        bg->fill(pal::CanvasBg.r, pal::CanvasBg.g, pal::CanvasBg.b, 255);
        root->push(std::move(bg));
    }

    // Grid dots
    {
        const int step = 24;
        for (int y = 0; y < ch; y += step) {
            for (int x = 0; x < cw; x += step) {
                auto dot = tvg::Shape::gen();
                dot->appendCircle((float)x, (float)y, 1.f, 1.f);
                dot->fill(45, 45, 55, 110);
                root->push(std::move(dot));
            }
        }
    }

    // Wires drawn below nodes so nodes sit on top.
    wireLayer.draw(*root);

    for (auto& n : nodes)
        n->drawAll(*root);

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
    Vec2 pt = {x, y};
    lastMouse = pt;

    // Socket click → wire drag (check before general hit test).
    if (auto* sock = socketAt(pt)) {
        wireLayer.beginDraft(sock, pt);
        isWireDrag = true;
        return;
    }

    Widget* hit = hitTest(pt);
    if (!hit) return;

    // If hitTest returns a socket (missed by socketAt), start wire drag.
    if (auto* s = dynamic_cast<SocketWidget*>(hit)) {
        wireLayer.beginDraft(s, pt);
        isWireDrag = true;
        return;
    }

    captured = hit;
    // Record at press-time whether this is a node title-bar drag so we don't
    // re-evaluate it each frame against the (moving) node's position.
    capturedIsDrag = false;
    if (auto* node = dynamic_cast<Node*>(hit))
        capturedIsDrag = node->titleBarHit(pt);

    hit->onMouseDown(pt, btn);
}

void NodeCanvas::onMouseUp(float x, float y, int btn) {
    Vec2 pt = {x, y};

    if (isWireDrag) {
        SocketWidget* drop = socketAt(pt);
        wireLayer.completeDraft(drop);
        isWireDrag = false;
        return;
    }

    if (captured) {
        captured->onMouseUp(pt, btn);
        captured = nullptr;
    }
}

void NodeCanvas::onMouseMove(float x, float y, float dx, float dy) {
    Vec2 pt    = {x, y};
    Vec2 delta = {dx, dy};
    lastMouse  = pt;

    if (isWireDrag) {
        wireLayer.updateDraft(pt);
        return;
    }

    if (!captured) return;

    // Node dragging: only when the initial press was in the title bar.
    if (auto* node = dynamic_cast<Node*>(captured)) {
        if (capturedIsDrag)
            node->onDrag(delta);
        return;
    }

    // Value widget: forward move event for direct manipulation.
    captured->onMouseMove(pt, delta);
}
