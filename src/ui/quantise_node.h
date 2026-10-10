#pragma once
#include "node.h"
#include "socket.h"
#include "selector.h"
#include "node_canvas.h"
#include <array>
#include <cmath>
#include <cstdio>

// ── NoteReadoutWidget ─────────────────────────────────────────────────────────
// Shows the note the quantiser is currently outputting, e.g. "A4  440.0 Hz".

class NoteReadoutWidget final : public Widget {
public:
    static constexpr float kH = 46.f;

    int   note = 69;     // MIDI note number
    float hz   = 440.f;

    NoteReadoutWidget() { height = kH; }

    bool contains(Vec2) const override { return false; }
    void onDrag(Vec2) override {}

    void draw(tvg::Scene& scene) override {
        Vec2 wp = worldPos();
        pushRect(scene, wp.x, wp.y, width, kH, 6.f, {30, 30, 40, 255});
        if (!NodeCanvas::fontLoaded()) return;

        static constexpr const char* kNames[12] =
            {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
        char name[8];
        std::snprintf(name, sizeof(name), "%s%d", kNames[((note % 12) + 12) % 12],
                      note / 12 - 1);
        char freq[24];
        std::snprintf(freq, sizeof(freq), "%.1f Hz", hz);

        auto t = tvg::Text::gen();
        if (t->font(NodeCanvas::fontName().c_str(), 16.f) == tvg::Result::Success) {
            t->text(name);
            t->fill(140, 200, 255);
            t->translate(wp.x + 8.f, wp.y + 4.f);
            scene.push(std::move(t));
        }
        auto f = tvg::Text::gen();
        if (f->font(NodeCanvas::fontName().c_str(), 9.f) == tvg::Result::Success) {
            f->text(freq);
            f->fill(160, 160, 180);
            f->translate(wp.x + 8.f, wp.y + kH - 15.f);
            scene.push(std::move(f));
        }
    }
};

// ── QuantiseNode ──────────────────────────────────────────────────────────────
// Snaps an incoming frequency (Hz) to the nearest note of the chosen scale and
// root, keeping a melody in tune. Wire e.g. Sequencer CV → In, Hz → Tone Freq.
// Trig fires whenever the output note changes (useful for envelopes).

class QuantiseNode final : public Node {
public:
    SocketWidget* inHz    = nullptr;
    SocketWidget* outHz   = nullptr;
    SocketWidget* outTrig = nullptr;

    static constexpr float kLabelRoom = 36.f;   // right-side space for output labels
    static constexpr float kInRoom    = 22.f;   // left-side space for the input label
    static constexpr float kHyst      = 0.1f;   // semitones; stops chatter at note boundaries
    static constexpr float kMinHz     = 16.35f; // C0
    static constexpr float kMaxHz     = 8000.f;

    explicit QuantiseNode(Vec2 pos = {}) : Node("Quantise", pos) {
        width = 280.f;

        inHz = addInput("In");
        inHz->value.f = 440.f;

        readout = addWidget(std::make_unique<NoteReadoutWidget>());
        readout->localPos.x += kInRoom;
        readout->width      -= kInRoom + kLabelRoom;

        auto* scaleSel = addWidget(std::make_unique<SelectorWidget>(
            std::vector<std::string>{"Chrom", "Major", "Minor", "MajPn", "MinPn", "Blues"}, 1));
        scaleSel->onChange = [this](int i) { scale = i; };

        auto* rootSel = addWidget(std::make_unique<SelectorWidget>(
            std::vector<std::string>{"C", "C#", "D", "D#", "E", "F",
                                     "F#", "G", "G#", "A", "A#", "B"}, 0));
        rootSel->onChange = [this](int i) { root = i; };

        outHz   = addOutput("Hz");
        outTrig = addOutput("Trig");
        outHz->value.f = 440.f;
    }

    void tick(float /*dt*/) override {
        outTrig->value.trig = false;

        float f    = clampf(inHz->value.f, kMinHz, kMaxHz);
        float midi = 69.f + 12.f * std::log2(f / 440.f);

        int best = nearestInScale(midi);
        // Only move off the current note when the new one is clearly closer.
        if (best != note &&
            (!inScale(note) || std::abs(midi - best) + kHyst < std::abs(midi - note))) {
            note = best;
            outTrig->value.trig = true;
        }

        float hz = 440.f * std::pow(2.f, (note - 69) / 12.f);
        outHz->value.f = hz;
        readout->note  = note;
        readout->hz    = hz;
    }

private:
    // Semitone masks per scale, bit i = (root + i) is in the scale.
    static constexpr std::array<uint16_t, 6> kScales = {
        0b111111111111,  // Chromatic
        0b101010110101,  // Major          0 2 4 5 7 9 11
        0b010110101101,  // Natural minor  0 2 3 5 7 8 10
        0b001010010101,  // Major pent     0 2 4 7 9
        0b010010101001,  // Minor pent     0 3 5 7 10
        0b010011101001,  // Blues          0 3 5 6 7 10
    };

    NoteReadoutWidget* readout = nullptr;
    int scale = 1;   // Major
    int root  = 0;   // C
    int note  = 69;  // current output note (MIDI)

    bool inScale(int m) const {
        int deg = ((m - root) % 12 + 12) % 12;
        return (kScales[scale] >> deg) & 1;
    }

    int nearestInScale(float midi) const {
        int   centre = (int)std::lround(midi);
        int   best   = centre;
        float bestD  = 1e9f;
        for (int off = -6; off <= 6; ++off) {
            int m = centre + off;
            float d = std::abs(midi - m);
            if (inScale(m) && d < bestD) { bestD = d; best = m; }
        }
        return best;
    }
};
