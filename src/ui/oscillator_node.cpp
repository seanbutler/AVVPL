#include "oscillator_node.h"
#include "slider.h"
#include "dial.h"
#include "selector.h"
#include <cmath>

OscillatorNode::OscillatorNode(Vec2 pos) : Node("Oscillator", pos) {
    inFreqMod = addInput("Freq Mod");
    inAmpMod  = addInput("Amp Mod");

    auto* freqSlider = addWidget(std::make_unique<SliderWidget>("Freq", 20.f, 2000.f, 440.f));
    freqSlider->onChange = [this](float v) { baseFreq = v; };

    auto* ampDial = addWidget(std::make_unique<DialWidget>("Amp", 0.f, 1.f, 0.7f));
    ampDial->onChange = [this](float v) { baseAmp = v; };

    auto* waveSel = addWidget(std::make_unique<SelectorWidget>(
        std::vector<std::string>{"Sine", "Sqr", "Saw", "Tri"}, 0));
    waveSel->onChange = [this](int i) { atomWave.store(i); };

    auto* ring = addWidget(std::make_unique<ProgressRingWidget>([this]{ return dispAmp; }));
    ring->arcColor = pal::DialArc;

    outAudio = addOutput("Audio");
    outPhase = addOutput("Phase");

    SDL_AudioSpec spec;
    SDL_zero(spec);
    spec.format   = SDL_AUDIO_F32;
    spec.channels = 1;
    spec.freq     = kSampleRate;

    stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                       &spec, audioGetCb, this);
    if (stream) SDL_ResumeAudioStreamDevice(stream);
}

OscillatorNode::~OscillatorNode() {
    if (stream) {
        SDL_PauseAudioStreamDevice(stream);
        SDL_DestroyAudioStream(stream);
    }
}

void OscillatorNode::tick(float /*dt*/) {
    float freqMod = inFreqMod->connected ? inFreqMod->value.f : 0.f;
    float ampMod  = inAmpMod->connected  ? inAmpMod->value.f  : 1.f;

    float freq = std::max(1.f, baseFreq + freqMod);
    float amp  = baseAmp * ampMod;

    atomFreq.store(freq, std::memory_order_relaxed);
    atomAmp .store(amp,  std::memory_order_relaxed);

    outAudio->value.f = freq;
    outPhase->value.f = (float)atomPhase.load(std::memory_order_relaxed);

    dispAmp += (amp - dispAmp) * 0.3f;
}

void SDLCALL OscillatorNode::audioGetCb(void* userdata, SDL_AudioStream* /*stream*/,
                                         int additionalBytes, int /*totalBytes*/)
{
    auto* self   = static_cast<OscillatorNode*>(userdata);
    int   frames = additionalBytes / (int)sizeof(float);
    if (frames <= 0) return;

    float buf[4096];
    if (frames > 4096) frames = 4096;

    self->fillBuffer(buf, frames);
    SDL_PutAudioStreamData(self->stream, buf, frames * (int)sizeof(float));
}

void OscillatorNode::fillBuffer(float* out, int frames) {
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

    atomPhase.store(phase, std::memory_order_relaxed);
}
