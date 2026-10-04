#include "tone_node.h"
#include "dial.h"
#include <cmath>

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

    SDL_AudioSpec spec;
    SDL_zero(spec);
    spec.format   = SDL_AUDIO_F32;
    spec.channels = 1;
    spec.freq     = kSampleRate;

    stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                       &spec, audioGetCb, this);
    if (stream) SDL_ResumeAudioStreamDevice(stream);
}

ToneNode::~ToneNode() {
    if (stream) {
        SDL_PauseAudioStreamDevice(stream);
        SDL_DestroyAudioStream(stream);
    }
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

    for (int i = 0; i < frames; ++i) {
        float s;
        switch (wave) {
            case 1:  s = phase < 0.5 ? 1.f : -1.f;                           break;
            case 2:  s = (float)(2.0 * phase - 1.0);                          break;
            case 3:  s = (float)(4.0 * std::abs(phase - 0.5) - 1.0);         break;
            default: s = (float)std::sin(phase * 2.0 * M_PI);                 break;
        }
        out[i] = s * a;
        phase += inc;
        if (phase >= 1.0) phase -= 1.0;
    }
}
