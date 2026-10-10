#include "tone_node.h"
#include "dial.h"
#include <algorithm>
#include <cmath>

static std::vector<ToneNode*> gToneNodes;

const std::vector<ToneNode*>& ToneNode::instances() { return gToneNodes; }

void ToneNode::mixRecent(float* dst, int n) const {
    uint32_t w = tapWrite.load(std::memory_order_acquire);
    for (int i = 0; i < n; ++i)
        dst[i] += tap[(w - n + i) & (kTapSize - 1)].load(std::memory_order_relaxed);
}

ToneNode::ToneNode(Vec2 pos) : Node("Tone", pos) {
    inFreq  = addInput("Freq");
    inWave  = addInput("Wave");
    inAmp   = addInput("Amp");
    inAmpCV = addInput("Amp CV");

    inFreq->value.f  = 440.f;
    inWave->value.f  = 0.f;
    inAmp->value.f   = 0.5f;
    inAmpCV->value.f = 1.f;  // neutral when unwired

    auto* ampDial = addWidget(std::make_unique<DialWidget>("Amp", 0.f, 1.f, 0.5f));
    ampDial->onChange = [this](float v) { inAmp->value.f = v; atomAmp.store(v); };

    auto* ring = addWidget(std::make_unique<ProgressRingWidget>([this]{ return dispAmp; }));
    ring->arcColor = pal::SockOut;

    outAudio = addOutput("Audio");

    for (auto& t : tap) t.store(0.f, std::memory_order_relaxed);
    gToneNodes.push_back(this);

    SDL_AudioSpec spec;
    SDL_zero(spec);
    spec.format   = SDL_AUDIO_F32;
    spec.channels = 1;
    spec.freq     = kSampleRate;

    stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                       &spec, audioGetCb, this);
    if (stream) {
        SDL_ResumeAudioStreamDevice(stream);
        static bool logged = false;  // report the device buffer once, for latency diagnosis
        SDL_AudioSpec dev; int frames = 0;
        if (!logged && SDL_GetAudioDeviceFormat(SDL_GetAudioStreamDevice(stream), &dev, &frames)) {
            SDL_Log("Audio: %s, %d Hz, %d-frame device buffer (~%.1f ms)",
                    SDL_GetCurrentAudioDriver(), dev.freq, frames, 1000.0 * frames / dev.freq);
            logged = true;
        }
    }
}

ToneNode::~ToneNode() {
    if (stream) {
        SDL_PauseAudioStreamDevice(stream);
        SDL_DestroyAudioStream(stream);
    }
    gToneNodes.erase(std::remove(gToneNodes.begin(), gToneNodes.end(), this), gToneNodes.end());
}

void ToneNode::tick(float /*dt*/) {
    atomFreq.store(inFreq->value.f, std::memory_order_relaxed);
    atomWave.store((int)std::round(inWave->value.f), std::memory_order_relaxed);
    float cv = inAmpCV->connected ? inAmpCV->value.f : 1.f;
    float amp = inAmp->value.f * cv;
    atomAmp.store(amp, std::memory_order_relaxed);
    // Smooth display amp toward current value for the visual ring.
    dispAmp += (amp - dispAmp) * 0.3f;
    outAudio->value.f = inFreq->value.f;
}

void SDLCALL ToneNode::audioGetCb(void* userdata, SDL_AudioStream* /*stream*/,
                                   int additionalBytes, int /*totalBytes*/)
{
    auto* self   = static_cast<ToneNode*>(userdata);
    int   frames = additionalBytes / (int)sizeof(float);
    if (frames <= 0) return;

    float buf[4096];
    if (frames > 4096) frames = 4096;

    self->fillBuffer(buf, frames);
    SDL_PutAudioStreamData(self->stream, buf, frames * (int)sizeof(float));
}

void ToneNode::fillBuffer(float* out, int frames) {
    float  f    = atomFreq.load(std::memory_order_relaxed);
    float  a    = atomAmp .load(std::memory_order_relaxed);
    int    wave = atomWave.load(std::memory_order_relaxed);

    if (f < 1.f) f = 1.f;
    double inc = (double)f / kSampleRate;
    uint32_t w = tapWrite.load(std::memory_order_relaxed);

    for (int i = 0; i < frames; ++i) {
        float s;
        switch (wave) {
            case 1:  s = phase < 0.5 ? 1.f : -1.f;                           break;
            case 2:  s = (float)(2.0 * phase - 1.0);                          break;
            case 3:  s = (float)(4.0 * std::abs(phase - 0.5) - 1.0);         break;
            case 4:  noiseSeed = noiseSeed * 1664525u + 1013904223u;
                     s = (float)(int32_t)noiseSeed / (float)0x80000000u;      break;
            default: s = (float)std::sin(phase * 2.0 * M_PI);                 break;
        }
        out[i] = s * a;
        tap[(w + i) & (kTapSize - 1)].store(out[i], std::memory_order_relaxed);
        phase += inc;
        if (phase >= 1.0) phase -= 1.0;
    }
    tapWrite.store(w + (uint32_t)frames, std::memory_order_release);
}
