#pragma once
#include "node.h"
#include "socket.h"
#include "timer_node.h"
#include <SDL3/SDL.h>
#include <atomic>

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

private:
    std::atomic<float> atomFreq{440.f};
    std::atomic<float> atomAmp {0.5f};
    std::atomic<int>   atomWave{0};

    // Smoothed amplitude for the visual level ring (main thread only).
    float dispAmp = 0.f;

    double phase = 0.0;

    SDL_AudioStream* stream = nullptr;

    static void SDLCALL audioGetCb(void* userdata, SDL_AudioStream* stream,
                                    int additionalBytes, int totalBytes);
    void fillBuffer(float* out, int frames);
};
