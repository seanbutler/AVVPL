#pragma once
#include "node.h"
#include "socket.h"
#include "timer_node.h"
#include <SDL3/SDL.h>
#include <atomic>

// OscillatorNode — self-contained audio generator with CV modulation inputs.
//
// Freq Mod (additive Hz): wiring a CV here shifts the frequency up/down from
// the base slider value.  Useful for vibrato or FM-style pitch modulation.
//
// Amp Mod (0–1 multiplier): scales the base amplitude dial.  Wire an Envelope
// CV Out here for ADSR shaping directly on the oscillator.
//
// Phase output tracks the oscillator's 0–1 cycle position (audio-thread
// accurate) so it can be used to sync or modulate downstream nodes.
class OscillatorNode final : public Node {
public:
    SocketWidget* inFreqMod = nullptr;
    SocketWidget* inAmpMod  = nullptr;
    SocketWidget* outAudio  = nullptr;
    SocketWidget* outPhase  = nullptr;

    static constexpr int kSampleRate = 44100;

    explicit OscillatorNode(Vec2 pos = {});
    ~OscillatorNode() override;

    void tick(float dt) override;

private:
    std::atomic<float>  atomFreq {440.f};
    std::atomic<float>  atomAmp  {0.7f};
    std::atomic<int>    atomWave {0};
    std::atomic<double> atomPhase{0.0};  // written by audio thread, read by tick()

    float baseFreq = 440.f;
    float baseAmp  = 0.7f;
    float dispAmp  = 0.f;

    double phase = 0.0;

    SDL_AudioStream* stream = nullptr;

    static void SDLCALL audioGetCb(void* userdata, SDL_AudioStream*,
                                    int additionalBytes, int totalBytes);
    void fillBuffer(float* out, int frames);
};
