#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <thorvg.h>

#include "ui/node_canvas.h"
#include "ui/slider.h"
#include "ui/dial.h"
#include "ui/histogram.h"

#include <cstdlib>
#include <memory>

static constexpr int W = 1280;
static constexpr int H = 800;

static void buildScene(NodeCanvas& nc)
{
    // ── Node A: Oscillator ────────────────────────────────────────────────────
    auto nA = std::make_unique<Node>("Oscillator", Vec2{80, 80});
    nA->addInput("Freq Mod");
    nA->addInput("Amp Mod");
    auto* freqSlider = nA->addWidget(std::make_unique<SliderWidget>("Freq", 20.f, 2000.f, 440.f));
    auto* ampDial    = nA->addWidget(std::make_unique<DialWidget>  ("Amp",   0.f,    1.f,   0.7f));
    auto* outA       = nA->addOutput("Audio");
    auto* outPhase   = nA->addOutput("Phase");
    (void)freqSlider; (void)ampDial; (void)outA; (void)outPhase;
    auto* pA = nc.addNode(std::move(nA));

    // ── Node B: Filter ────────────────────────────────────────────────────────
    auto nB = std::make_unique<Node>("Filter", Vec2{360, 60});
    auto* inFilter  = nB->addInput("Audio In");
    auto* inCutoff  = nB->addInput("Cutoff CV");
    auto* cutSlider = nB->addWidget(std::make_unique<SliderWidget>("Cutoff", 20.f, 20000.f, 1200.f));
    auto* resDial   = nB->addWidget(std::make_unique<DialWidget>  ("Resonance", 0.f, 1.f, 0.3f));
    auto* outFilter = nB->addOutput("Audio");
    (void)inCutoff; (void)cutSlider; (void)resDial; (void)outFilter;
    auto* pB = nc.addNode(std::move(nB));
    (void)pB;

    // ── Node C: Envelope ─────────────────────────────────────────────────────
    auto nC = std::make_unique<Node>("Envelope", Vec2{360, 290});
    nC->addInput("Gate");
    auto* envCurve = nC->addWidget(std::make_unique<HistogramWidget>(8, 0.5f));
    // Ramp up shape
    if (envCurve) {
        for (int i = 0; i < 8; ++i)
            envCurve->values[i] = (float)i / 7.f;
    }
    auto* outEnv = nC->addOutput("CV Out");
    (void)outEnv;
    auto* pC = nc.addNode(std::move(nC));
    (void)pC;

    // ── Node D: Spectral shaper ───────────────────────────────────────────────
    auto nD = std::make_unique<Node>("Spectral", Vec2{640, 80});
    nD->addInput("Audio");
    nD->addInput("Shape CV");
    auto* spectrum = nD->addWidget(std::make_unique<HistogramWidget>(16, 0.5f));
    // A gentle bell curve
    if (spectrum) {
        for (int i = 0; i < 16; ++i) {
            float t = (float)i / 15.f;
            float v = std::exp(-12.f * (t - 0.5f) * (t - 0.5f));
            spectrum->values[i] = v;
        }
    }
    auto* gainDial = nD->addWidget(std::make_unique<DialWidget>("Gain", 0.f, 2.f, 1.f));
    nD->addOutput("Audio");
    nD->addOutput("Peaks");
    (void)gainDial;
    nc.addNode(std::move(nD));

    // ── Node E: Output ────────────────────────────────────────────────────────
    auto nE = std::make_unique<Node>("Output", Vec2{900, 150});
    auto* inOut  = nE->addInput("Audio L");
    auto* inOut2 = nE->addInput("Audio R");
    auto* volSlider = nE->addWidget(std::make_unique<SliderWidget>("Volume", 0.f, 1.f, 0.8f));
    (void)inOut; (void)inOut2; (void)volSlider;
    nc.addNode(std::move(nE));

    // ── Pre-wire some connections ─────────────────────────────────────────────
    nc.wires().connect(outA,     inFilter);   // Osc → Filter audio
    nc.wires().connect(outEnv,   inCutoff);   // Envelope → Filter cutoff
    (void)pA;
}

int main(int /*argc*/, char* /*argv*/[])
{
    if (tvg::Initializer::init(tvg::CanvasEngine::Sw, 0) != tvg::Result::Success) {
        SDL_Log("ThorVG init failed");
        return EXIT_FAILURE;
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init: %s", SDL_GetError());
        tvg::Initializer::term(tvg::CanvasEngine::Sw);
        return EXIT_FAILURE;
    }

    SDL_Window*   window   = SDL_CreateWindow("AVVPL — Node Graph", W, H, 0);
    SDL_Renderer* renderer = window ? SDL_CreateRenderer(window, nullptr) : nullptr;
    if (!renderer) {
        SDL_Log("SDL_CreateRenderer: %s", SDL_GetError());
        SDL_Quit();
        tvg::Initializer::term(tvg::CanvasEngine::Sw);
        return EXIT_FAILURE;
    }

    SDL_Texture* tex = SDL_CreateTexture(renderer,
                                         SDL_PIXELFORMAT_ARGB8888,
                                         SDL_TEXTUREACCESS_STREAMING, W, H);
    if (!tex) {
        SDL_Log("SDL_CreateTexture: %s", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        tvg::Initializer::term(tvg::CanvasEngine::Sw);
        return EXIT_FAILURE;
    }

    NodeCanvas nc(W, H);
    buildScene(nc);

    bool running = true;
    SDL_Event ev;

    while (running) {
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
                case SDL_EVENT_KEY_DOWN:
                    if (ev.key.key == SDLK_ESCAPE) running = false;
                    break;
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    nc.onMouseDown(ev.button.x, ev.button.y, ev.button.button);
                    break;
                case SDL_EVENT_MOUSE_BUTTON_UP:
                    nc.onMouseUp(ev.button.x, ev.button.y, ev.button.button);
                    break;
                case SDL_EVENT_MOUSE_MOTION:
                    nc.onMouseMove(ev.motion.x, ev.motion.y,
                                   ev.motion.xrel, ev.motion.yrel);
                    break;
            }
        }

        nc.render();
        SDL_UpdateTexture(tex, nullptr, nc.pixels(), nc.canvasW() * sizeof(uint32_t));
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, tex, nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    tvg::Initializer::term(tvg::CanvasEngine::Sw);
    return EXIT_SUCCESS;
}
