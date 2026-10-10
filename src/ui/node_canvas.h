#pragma once
#include "wire_layer.h"
#include "node.h"
#include <thorvg.h>
#include <functional>
#include <vector>
#include <memory>
#include <cstdint>
#include <string>

// Root canvas: owns the TVG SwCanvas, all nodes, the wire layer, and
// dispatches SDL-style mouse events into the widget tree.
class NodeCanvas {
public:
    explicit NodeCanvas(int w, int h);

    bool showGrid = false;  // background grid lines (off for now)

    // Add a node; returns raw pointer for adding sockets/widgets.
    Node* addNode(std::unique_ptr<Node> n);

    WireLayer&       wires()       { return wireLayer; }
    const WireLayer& wires() const { return wireLayer; }

    // Graph management
    void clear();
    const std::vector<std::unique_ptr<Node>>& allNodes() const { return nodes; }

    // Font loading. `familyName` must match the TTF's internal family name
    // (e.g. "DejaVu Sans" for DejaVuSans.ttf, "Ubuntu" for Ubuntu-R.ttf).
    static bool loadFont(const std::string& path, const std::string& familyName);
    static bool fontLoaded();
    static const std::string& fontName();

    // Resize the canvas (call after the window resizes).
    void resize(int w, int h);

    // Advance all node logic by dt seconds. Call once per frame before render().
    void tick(float dt);

    // Optional overlay drawn on top of the node graph each frame (e.g. palette).
    void setOverlay(std::function<void(tvg::Scene&)> fn) { overlayFn = std::move(fn); }

    // Render everything into pixels[].
    void render();
    const uint32_t* pixels() const { return buf.data(); }
    int canvasW() const { return cw; }
    int canvasH() const { return ch; }

    // SDL-style mouse events.
    void onMouseDown (float x, float y, int btn);
    void onMouseUp   (float x, float y, int btn);
    void onMouseMove (float x, float y, float dx, float dy);
    void onMouseWheel(float x, float y, float delta);

    // Camera transform helpers.
    Vec2 screenToWorld(Vec2 s) const { return { (s.x - camPan.x) / camZoom, (s.y - camPan.y) / camZoom }; }
    Vec2 worldToScreen(Vec2 w) const { return { w.x * camZoom + camPan.x,   w.y * camZoom + camPan.y   }; }

private:
    int cw, ch;
    std::vector<uint32_t>  buf;
    std::unique_ptr<tvg::SwCanvas> canvas;

    std::vector<std::unique_ptr<Node>> nodes;
    WireLayer wireLayer;

    // Camera state
    Vec2  camPan  = {};
    float camZoom = 1.f;

    // Mouse capture state
    Widget*  captured        = nullptr;
    bool     isWireDrag      = false;
    bool     isPanning       = false;
    int      panButton       = 0;      // mouse button that started the current pan
    bool     capturedIsDrag  = false;  // true when captured node was hit on its title bar
    Vec2     lastMouse       = {};

    std::function<void(tvg::Scene&)> overlayFn;

    Widget*       hitTest (Vec2 pt);
    SocketWidget* socketAt(Vec2 pt) const;
};
