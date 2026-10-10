#pragma once
#include "types.h"
#include <thorvg.h>
#include <functional>
#include <string>
#include <vector>

class NodeCanvas;

// A full-height panel docked to the left edge of the window.
// Node buttons grouped by category at the top; Save / Load pinned to the bottom.
class Palette {
public:
    static constexpr float kW        = 130.f;  // panel width
    static constexpr float kBtnH     =  24.f;  // button height
    static constexpr float kBtnGap   =   4.f;  // gap between buttons
    static constexpr float kPadX     =   8.f;
    static constexpr float kPadY     =  10.f;
    static constexpr float kCorner   =   5.f;
    static constexpr float kHeaderH  =  16.f;  // category section header height
    static constexpr float kStripeW  =   4.f;  // category colour stripe on node buttons

    Palette(int canvasH, NodeCanvas& nc);

    void draw(tvg::Scene& scene) const;
    void resize(int canvasH);                 // re-pins the footer to the window bottom

    // Returns true if the palette consumed the event (canvas should ignore it).
    bool onMouseDown(float x, float y);
    bool contains   (float x, float y) const;
    void onMouseMove(float x, float y);       // updates hover highlight
    void onMouseLeave() { hoveredIdx = -1; }

    // Spawn position for newly added nodes (shifts each time to avoid overlap)
    Vec2 nextSpawnPos();

private:
    enum class Kind { Button, Header, Ghost };  // Ghost = outlined footer button

    struct Button {
        std::string label;
        Color4      color;
        std::function<void()> action;
        Kind  kind = Kind::Button;
        float x = 0.f, y = 0.f, w = 0.f;  // assigned by layout()
    };

    std::vector<Button> buttons;
    int   hoveredIdx = -1;
    float panelH     = 0.f;   // full canvas height
    float footerY    = 0.f;   // top of the Save/Load row
    NodeCanvas& nc;
    Vec2  spawnCursor = {kW + 20.f, 60.f};

    void  layout();
    int   buttonAt(float x, float y) const;

    void drawButton(tvg::Scene& scene, int idx, bool hovered) const;
    static void pushText(tvg::Scene& scene, const std::string& text,
                         float x, float y, float size, Color4 color);
};
