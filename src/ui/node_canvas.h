#pragma once
#include "wire_layer.h"
#include "node.h"
#include <thorvg.h>
#include <vector>
#include <memory>
#include <cstdint>

// Root canvas: owns the TVG SwCanvas, all nodes, the wire layer, and
// dispatches SDL-style mouse events into the widget tree.
class NodeCanvas {
public:
    explicit NodeCanvas(int w, int h);

    // Add a node; returns raw pointer for adding sockets/widgets.
    Node* addNode(std::unique_ptr<Node> n);

    WireLayer& wires() { return wireLayer; }

    // Render everything into pixels[].
    void render();
    const uint32_t* pixels() const { return buf.data(); }
    int canvasW() const { return cw; }
    int canvasH() const { return ch; }

    // SDL-style mouse events.
    void onMouseDown(float x, float y, int btn);
    void onMouseUp  (float x, float y, int btn);
    void onMouseMove(float x, float y, float dx, float dy);

private:
    int cw, ch;
    std::vector<uint32_t>  buf;
    std::unique_ptr<tvg::SwCanvas> canvas;

    std::vector<std::unique_ptr<Node>> nodes;
    WireLayer wireLayer;

    // Mouse capture state
    Widget*  captured        = nullptr;
    bool     isWireDrag      = false;
    bool     capturedIsDrag  = false;  // true when captured node was hit on its title bar
    Vec2     lastMouse       = {};

    Widget*       hitTest (Vec2 pt);
    SocketWidget* socketAt(Vec2 pt) const;
};
