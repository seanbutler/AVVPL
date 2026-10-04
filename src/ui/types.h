#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

struct Vec2 {
    float x = 0.f, y = 0.f;
    Vec2 operator+(Vec2 o)  const { return {x+o.x, y+o.y}; }
    Vec2 operator-(Vec2 o)  const { return {x-o.x, y-o.y}; }
    Vec2 operator*(float s) const { return {x*s,   y*s};   }
    Vec2& operator+=(Vec2 o) { x+=o.x; y+=o.y; return *this; }
    float len() const { return std::sqrt(x*x + y*y); }
};

struct Color4 { uint8_t r=0, g=0, b=0, a=255; };

// Value carried along wires between sockets.
// trig is a one-frame pulse; f is a continuous float.
struct Value {
    float f    = 0.f;
    bool  trig = false;
};

namespace pal {
    inline constexpr Color4 NodeBg      {28,  28,  36,  235};
    inline constexpr Color4 NodeBorder  {55,  55,  72,  255};
    inline constexpr Color4 TitleBg     {42,  42,  58,  255};
    inline constexpr Color4 TitleBgSel  {55,  80, 130,  255};
    inline constexpr Color4 SliderTrack {42,  42,  55,  255};
    inline constexpr Color4 SliderFill  {60, 120, 210,  200};
    inline constexpr Color4 SliderThumb {130,175, 255,  255};
    inline constexpr Color4 DialTrack   {42,  42,  55,  255};
    inline constexpr Color4 DialArc     {60, 190, 130,  255};
    inline constexpr Color4 DialKnob    {70,  70,  88,  255};
    inline constexpr Color4 HistBar     {70, 170, 110,  210};
    inline constexpr Color4 HistBarHot  {110,220, 155,  240};
    inline constexpr Color4 SockIn      {70, 200, 150,  255};
    inline constexpr Color4 SockOut     {210,110,  70,  255};
    inline constexpr Color4 WireColor   {190,165,  55,  210};
    inline constexpr Color4 WireDraft   {200,200, 200,  120};
    inline constexpr Color4 CanvasBg    { 18,  18,  22,  255};
}

inline float clamp01(float v) { return std::max(0.f, std::min(1.f, v)); }
inline float clampf(float v, float lo, float hi) { return std::max(lo, std::min(hi, v)); }
inline float lerpf(float a, float b, float t) { return a + (b-a)*t; }
inline constexpr float kPi = 3.14159265f;
