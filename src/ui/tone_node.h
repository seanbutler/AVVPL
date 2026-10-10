#pragma once
#include "node.h"
#include "socket.h"
#include "timer_node.h"
#include <SDL3/SDL.h>
#include <atomic>
#include <cstdint>
#include <vector>

// ToneNode — generates audio from Freq, Wave, and Amp inputs.
// Freq and Wave are typically driven by a WaveNode; Amp by a dial or CV.
class ToneNode final : public Node {
public:
    SocketWidget* inFreq  = nullptr;
    SocketWidget* inWave  = nullptr;
    SocketWidget* inAmp   = nullptr;
    SocketWidget* inAmpCV = nullptr;  // 0-1 CV scales the Amp dial; defaults to 1 when unwired
    SocketWidget* outAudio = nullptr;

    static constexpr int kSampleRate = 44100;

    explicit ToneNode(Vec2 pos = {});
    ~ToneNode() override;

    void tick(float dt) override;

    // All live ToneNodes (main thread only) — used by ScopeNode to mix the graph's output.
    static const std::vector<ToneNode*>& instances();

    // Adds the most recent n output samples (oldest first) into dst. Keep n <= kTapSize / 2.
    static constexpr int kTapSize = 16384;  // power of two
    void mixRecent(float* dst, int n) const;

private:
    std::atomic<float> atomFreq{440.f};
    std::atomic<float> atomAmp {0.5f};
    std::atomic<int>   atomWave{0};

    // Smoothed amplitude for the visual level ring (main thread only).
    float dispAmp = 0.f;

    double   phase     = 0.0;
    uint32_t noiseSeed = 22222;

    SDL_AudioStream* stream = nullptr;

    // Ring buffer of recent output samples, written by the audio thread.
    std::atomic<float>    tap[kTapSize];
    std::atomic<uint32_t> tapWrite{0};

    static void SDLCALL audioGetCb(void* userdata, SDL_AudioStream* stream,
                                    int additionalBytes, int totalBytes);
    void fillBuffer(float* out, int frames);
};
