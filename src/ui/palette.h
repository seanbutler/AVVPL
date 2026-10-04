#pragma once
#include "types.h"
#include <thorvg.h>
#include <functional>
#include <string>
#include <vector>

class NodeCanvas;

// A vertical panel of buttons docked to the left edge of the window.
// Buttons: one per registered node type + Save + Load.
class Palette {
public:
    static constexpr float kW        = 110.f;  // panel width
    static constexpr float kBtnH     =  32.f;  // button height
    static constexpr float kBtnGap   =   4.f;  // gap between buttons
    static constexpr float kPadX     =   8.f;
    static constexpr float kPadY     =  10.f;
    static constexpr float kCorner   =   5.f;

    Palette(int canvasH, NodeCanvas& nc);

    void draw(tvg::Scene& scene) const;

    // Returns true if the palette consumed the event (canvas should ignore it).
    bool onMouseDown(float x, float y);
    bool contains   (float x, float y) const;

    // Spawn position for newly added nodes (shifts each time to avoid overlap)
    Vec2 nextSpawnPos();

private:
    struct Button {
        std::string label;
        Color4      color;
        std::function<void()> action;
        bool isSeparator = false;
    };

    std::vector<Button> buttons;
    int   hoveredIdx = -1;
    float panelH     = 0.f;
    NodeCanvas& nc;
    Vec2  spawnCursor = {130.f, 60.f};

    float btnY(int i) const { return kPadY + i * (kBtnH + kBtnGap); }
    int   buttonAt(float x, float y) const;

    void drawButton(tvg::Scene& scene, int idx, bool hovered) const;
    static void pushText(tvg::Scene& scene, const std::string& text,
                         float x, float y, float size, Color4 color);
};
