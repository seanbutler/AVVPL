#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <thorvg.h>

#include "ui/node_canvas.h"
#include "ui/node_registry.h"
#include "ui/graph_io.h"
#include "ui/palette.h"
#include "ui/slider.h"
#include "ui/timer_node.h"
#include "ui/tone_node.h"
#include "ui/envelope_node.h"
#include "ui/number_node.h"
#include "ui/waveform_node.h"
#include "ui/step_seq_node.h"

#include <cmath>
#include <cstdlib>
#include <memory>

static int W = 1280;
static int H = 800;

// ── Node factory functions ────────────────────────────────────────────────────

static std::unique_ptr<Node> makeTimer(Vec2 pos) {
    auto n = std::make_unique<TimerNode>(pos);
    auto* s = n->addWidget(std::make_unique<SliderWidget>("Duration", 0.5f, 8.f, 3.f));
    s->onChange = [raw = n.get()](float v) { raw->inDuration->value.f = v; };
    return n;
}

static std::unique_ptr<Node> makeLoop(Vec2 pos) {
    auto n = std::make_unique<LoopTimerNode>(pos);
    auto* s = n->addWidget(std::make_unique<SliderWidget>("Interval", 0.1f, 4.f, 1.f));
    s->onChange = [raw = n.get()](float v) { raw->inInterval->value.f = v; };
    return n;
}

// ── Register all node types ───────────────────────────────────────────────────

static void registerNodes() {
    auto& reg = NodeRegistry::get();
    reg.reg("Tone",       "Tone",       {160, 130, 40, 255},
            [](Vec2 pos){ return std::make_unique<ToneNode>(pos); });
    reg.reg("Envelope",   "Envelope",   {130, 80,  60, 255},
            [](Vec2 pos){ return std::make_unique<EnvelopeNode>(pos); });
    reg.reg("Number",     "Number",     {100, 100, 140, 255},
            [](Vec2 pos){ return std::make_unique<NumberNode>(pos); });
    reg.reg("Waveform",   "Waveform",   {100, 140, 100, 255},
            [](Vec2 pos){ return std::make_unique<WaveformNode>(pos); });
    reg.reg("Sequencer",  "Sequencer",  {120, 80, 160, 255},
            [](Vec2 pos){ return std::make_unique<StepSeqNode>(pos); });
    reg.reg("Timer",      "Timer",      {160, 60,  60, 255}, makeTimer);
    reg.reg("Loop",       "Loop",       {60, 160,  90, 255}, makeLoop);
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main(int /*argc*/, char* /*argv*/[])
{
    if (tvg::Initializer::init(tvg::CanvasEngine::Sw, 0) != tvg::Result::Success) {
        SDL_Log("ThorVG init failed");
        return EXIT_FAILURE;
    }

    NodeCanvas::loadFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", "DejaVuSans");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        SDL_Log("SDL_Init: %s", SDL_GetError());
        tvg::Initializer::term(tvg::CanvasEngine::Sw);
        return EXIT_FAILURE;
    }

    SDL_Window*   window   = SDL_CreateWindow("AVVPL — Node Graph", W, H, SDL_WINDOW_RESIZABLE);
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

    registerNodes();

    NodeCanvas nc(W, H);

    Palette palette(H, nc);
    nc.setOverlay([&palette](tvg::Scene& s){ palette.draw(s); });

    bool   running  = true;
    SDL_Event ev;
    Uint64 lastTick = SDL_GetTicksNS();

    while (running) {
        Uint64 now = SDL_GetTicksNS();
        float  dt  = (float)((now - lastTick) * 1e-9);
        lastTick   = now;
        dt = std::min(dt, 0.1f);

        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;

                case SDL_EVENT_WINDOW_RESIZED:
                    W = ev.window.data1;
                    H = ev.window.data2;
                    SDL_DestroyTexture(tex);
                    tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                            SDL_TEXTUREACCESS_STREAMING, W, H);
                    nc.resize(W, H);
                    break;

                case SDL_EVENT_KEY_DOWN:
                    if (ev.key.key == SDLK_ESCAPE) running = false;
                    if (ev.key.key == SDLK_S && (ev.key.mod & SDL_KMOD_CTRL))
                        graphSave(nc, "graph.json");
                    if (ev.key.key == SDLK_O && (ev.key.mod & SDL_KMOD_CTRL))
                        graphLoad(nc, "graph.json");
                    break;

                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    if (!palette.onMouseDown(ev.button.x, ev.button.y))
                        nc.onMouseDown(ev.button.x, ev.button.y, ev.button.button);
                    break;

                case SDL_EVENT_MOUSE_BUTTON_UP:
                    nc.onMouseUp(ev.button.x, ev.button.y, ev.button.button);
                    break;

                case SDL_EVENT_MOUSE_MOTION:
                    nc.onMouseMove(ev.motion.x, ev.motion.y,
                                   ev.motion.xrel, ev.motion.yrel);
                    break;

                case SDL_EVENT_MOUSE_WHEEL:
                    if (!palette.contains(ev.wheel.mouse_x, ev.wheel.mouse_y))
                        nc.onMouseWheel(ev.wheel.mouse_x, ev.wheel.mouse_y, ev.wheel.y);
                    break;
            }
        }

        nc.tick(dt);
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
