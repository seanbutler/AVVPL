#pragma once
#include "types.h"
#include <thorvg.h>
#include <vector>
#include <algorithm>

class SocketWidget;
class Widget;

struct Wire {
    SocketWidget* from;  // output socket
    SocketWidget* to;    // input socket
};

class WireLayer {
public:
    std::vector<Wire> wires;

    SocketWidget* draftFrom = nullptr;
    Vec2          draftEnd  = {};
    bool          hasDraft  = false;

    void beginDraft (SocketWidget* from, Vec2 mousePos);
    void updateDraft(Vec2 mousePos);
    // Returns the input socket that was connected, or nullptr.
    SocketWidget* completeDraft(SocketWidget* dropTarget);
    void cancelDraft();

    void connect         (SocketWidget* from, SocketWidget* to);
    void disconnectSocket(SocketWidget* sock);

    // Walk widget tree collecting all SocketWidgets.
    SocketWidget* findSocketAt(Vec2 worldPt, const std::vector<std::unique_ptr<Widget>>& roots) const;

    void draw(tvg::Scene& scene) const;

private:
    static void drawWire(tvg::Scene&, Vec2 a, Vec2 b, Color4 c, float sw);
};
