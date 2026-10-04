#include "widget.h"
#include <cmath>

void Widget::pushRect(tvg::Scene& s, float x, float y, float w, float h,
                      float rx, Color4 fill, Color4 stroke, float sw)
{
    auto sh = tvg::Shape::gen();
    sh->appendRect(x, y, w, h, rx, rx);
    sh->fill(fill.r, fill.g, fill.b, fill.a);
    if (sw > 0.f && stroke.a > 0) {
        sh->stroke(sw);
        sh->stroke(stroke.r, stroke.g, stroke.b, stroke.a);
    }
    s.push(std::move(sh));
}

void Widget::pushCircle(tvg::Scene& s, float cx, float cy, float r,
                        Color4 fill, Color4 stroke, float sw)
{
    auto sh = tvg::Shape::gen();
    sh->appendCircle(cx, cy, r, r);
    sh->fill(fill.r, fill.g, fill.b, fill.a);
    if (sw > 0.f && stroke.a > 0) {
        sh->stroke(sw);
        sh->stroke(stroke.r, stroke.g, stroke.b, stroke.a);
    }
    s.push(std::move(sh));
}

void Widget::pushLine(tvg::Scene& s, float x1, float y1, float x2, float y2,
                      Color4 c, float sw)
{
    auto sh = tvg::Shape::gen();
    sh->moveTo(x1, y1);
    sh->lineTo(x2, y2);
    sh->stroke(sw);
    sh->stroke(c.r, c.g, c.b, c.a);
    sh->fill(0, 0, 0, 0);
    s.push(std::move(sh));
}

// startDeg / endDeg: degrees in screen-CW-from-right (Y-down) convention.
// cos/sin produce CW screen angles because sin(90°)=1 = Y pointing down = 6 o'clock.
void Widget::pushArc(tvg::Scene& s, float cx, float cy, float r,
                     float startDeg, float endDeg, Color4 c, float sw, int segs)
{
    auto sh = tvg::Shape::gen();
    float a0 = startDeg * kPi / 180.f;
    float a1 = endDeg   * kPi / 180.f;
    sh->moveTo(cx + r * std::cos(a0), cy + r * std::sin(a0));
    for (int i = 1; i <= segs; ++i) {
        float t = (float)i / (float)segs;
        float a = a0 + t * (a1 - a0);
        sh->lineTo(cx + r * std::cos(a), cy + r * std::sin(a));
    }
    sh->stroke(sw);
    sh->stroke(c.r, c.g, c.b, c.a);
    sh->fill(0, 0, 0, 0);
    s.push(std::move(sh));
}

void Widget::pushBezier(tvg::Scene& s, Vec2 a, Vec2 b, Color4 c, float sw)
{
    float dx = std::abs(b.x - a.x) * 0.5f + 30.f;
    auto sh = tvg::Shape::gen();
    sh->moveTo(a.x, a.y);
    sh->cubicTo(a.x + dx, a.y, b.x - dx, b.y, b.x, b.y);
    sh->stroke(sw);
    sh->stroke(c.r, c.g, c.b, c.a);
    sh->fill(0, 0, 0, 0);
    s.push(std::move(sh));
}
