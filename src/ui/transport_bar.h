#pragma once
#include "types.h"
#include "node_canvas.h"
#include "widget.h"
#include <thorvg.h>
#include <cstring>
#include <array>

namespace tbar_detail {
    inline void fillRect(tvg::Scene& scene, float x, float y, float w, float h,
                         float rx, Color4 fill, Color4 stroke = {0,0,0,0}, float sw = 0.f) {
        auto sh = tvg::Shape::gen();
        sh->appendRect(x, y, w, h, rx, rx);
        sh->fill(fill.r, fill.g, fill.b, fill.a);
        if (sw > 0.f) {
            sh->stroke(sw);
            sh->stroke(stroke.r, stroke.g, stroke.b, stroke.a);
        }
        scene.push(std::move(sh));
    }
}

class TransportBar {
public:
    static constexpr float kH    = 36.f;
    static constexpr float kBtnW = 64.f;
    static constexpr float kBtnH = 24.f;
    static constexpr float kPadX =  8.f;
    static constexpr float kPadY = (kH - kBtnH) * 0.5f;

    enum class Btn { Play, Pause, Stop, Count };

    TransportBar(NodeCanvas& nc, int canvasW) : nc(nc), w((float)canvasW) {}

    void resize(int canvasW) { w = (float)canvasW; }

    bool contains(float x, float y) const {
        return x >= 0.f && x <= w && y >= 0.f && y <= kH;
    }

    bool onMouseDown(float x, float y) {
        if (!contains(x, y)) return false;
        for (int i = 0; i < (int)Btn::Count; ++i) {
            auto r = btnRect((Btn)i);
            if (x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h) {
                switch ((Btn)i) {
                    case Btn::Play:  nc.play();  break;
                    case Btn::Pause:
                        nc.transport() == NodeCanvas::Transport::Paused
                            ? nc.play() : nc.pause();
                        break;
                    case Btn::Stop:  nc.stop();  break;
                    default: break;
                }
                return true;
            }
        }
        return true;
    }

    void draw(tvg::Scene& scene) const {
        using T = NodeCanvas::Transport;
        T state = nc.transport();

        using namespace tbar_detail;
        fillRect(scene, 0.f, 0.f, w, kH, 0.f, {22, 22, 30, 255});
        fillRect(scene, 0.f, kH - 1.f, w, 1.f, 0.f, {50, 50, 65, 255});

        struct BtnDef { const char* label; bool active; Color4 col; };
        BtnDef defs[] = {
            { "Play",  state == T::Playing,  {50,  160,  80, 255} },
            { "Pause", state == T::Paused,   {180, 140,  40, 255} },
            { "Stop",  state == T::Stopped,  {180,  60,  60, 255} },
        };
        for (int i = 0; i < (int)Btn::Count; ++i) {
            auto r = btnRect((Btn)i);
            drawBtn(scene, r.x, r.y, defs[i].label, defs[i].active, defs[i].col);
        }
    }

private:
    NodeCanvas& nc;
    float       w;

    struct Rect { float x, y, w, h; };

    Rect btnRect(Btn b) const {
        float startX = w * 0.5f - (3.f * kBtnW + 2.f * kPadX) * 0.5f;
        float x = startX + (float)b * (kBtnW + kPadX);
        return { x, kPadY, kBtnW, kBtnH };
    }

    static void drawBtn(tvg::Scene& scene, float x, float y,
                        const char* label, bool active, Color4 activeCol) {
        Color4 bg  = active ? activeCol      : Color4{38, 38, 52, 255};
        Color4 bdr = active ? activeCol      : Color4{60, 60, 80, 255};
        tbar_detail::fillRect(scene, x, y, kBtnW, kBtnH, 5.f, bg, bdr, 1.5f);

        if (!NodeCanvas::fontLoaded()) return;
        auto t = tvg::Text::gen();
        if (t->font(NodeCanvas::fontName().c_str(), 9.f) != tvg::Result::Success) return;
        t->text(label);
        t->fill(active ? 255 : 180, active ? 255 : 180, active ? 255 : 200);
        float tw = std::strlen(label) * 5.5f;
        t->translate(x + (kBtnW - tw) * 0.5f, y + kBtnH * 0.5f - 5.f);
        scene.push(std::move(t));
    }
};
