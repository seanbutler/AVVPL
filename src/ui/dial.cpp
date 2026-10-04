#include "dial.h"
#include <cmath>

void DialWidget::draw(tvg::Scene& scene) {
    Vec2 c = center();
    float t = norm();

    // Background arc (full track, dimmed)
    pushArc(scene, c.x, c.y, kR,
            kStartDeg, kStartDeg + kSweep,
            pal::DialTrack, kArcW + 2.f);

    // Value arc
    if (t > 0.f)
        pushArc(scene, c.x, c.y, kR,
                kStartDeg, kStartDeg + t * kSweep,
                pal::DialArc, kArcW);

    // Knob body
    pushCircle(scene, c.x, c.y, kKnobR, pal::DialKnob, pal::NodeBorder, 1.f);

    // Indicator tick: points from center toward current value position
    float tickAngle = (kStartDeg + t * kSweep) * kPi / 180.f;
    float ix = c.x + kKnobR * 0.6f * std::cos(tickAngle);
    float iy = c.y + kKnobR * 0.6f * std::sin(tickAngle);
    pushLine(scene, c.x, c.y, ix, iy, {220, 220, 255, 220}, 2.f);
}

void DialWidget::onMouseMove(Vec2 /*pt*/, Vec2 delta) {
    // Drag up = increase value; sensitivity: 1px = 0.5% of range
    float dn = -delta.y * 0.005f;
    setNorm(norm() + dn);
}
