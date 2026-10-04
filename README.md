# AVVPL — Audio/Visual Visual Programming Language

A node graph editor for building audio signal chains, written in C++ with SDL3 and ThorVG.

## Building

Requires: CMake 3.25+, Ninja, Meson, a C++17 compiler.

```bash
cmake -B build -G Ninja
cmake --build build
./build/AVVPL
```

Dependencies are fetched automatically: SDL3, ThorVG (built via Meson), nlohmann/json.

## Controls

| Input | Action |
|---|---|
| Left-click drag on title bar | Move node |
| Left-click drag from socket | Draw wire |
| Right-click on node | Delete node |
| Right-click on socket | Disconnect socket |
| Right-click near wire | Delete wire |
| Ctrl+S | Save graph to `graph.json` |
| Ctrl+O | Load graph from `graph.json` |
| Escape | Quit |

## Nodes

| Node | Status | Description |
|---|---|---|
| **Wave** | ✓ | Outputs a frequency (Hz) and waveform type (Sine/Sqr/Saw/Tri) |
| **Tone** | ✓ | Synthesises audio from Freq, Wave, and Amp inputs via SDL audio |
| **Envelope** | ✓ | ADSR envelope — fires a 0–1 CV curve on each Gate trigger |
| **Loop** | ✓ | Fires a Tick trigger at a set interval; also outputs Phase (0–1) and Count |
| **Timer** | ✓ | One-shot countdown; fires Done trigger when it reaches zero |
| **Oscillator** | stub | Placeholder — no DSP implemented |
| **Filter** | stub | Placeholder — no DSP implemented |
| **Spectral** | stub | Placeholder — no DSP implemented |
| **Output** | stub | Placeholder — no audio mixing implemented |

## Example patch — rhythmic piano tone

```
Loop (Tick) ──► Envelope (Gate)
Envelope (CV Out) ──► Tone (Amp CV)
Wave (Freq) ──► Tone (Freq)
Wave (Wave) ──► Tone (Wave)
```

Set Envelope sustain to 0 for a piano-like decay. Adjust Loop interval for tempo.

## Architecture

```
src/
  main.cpp              — SDL3 setup, node factories, event loop
  ui/
    types.h             — Vec2, Color4, Value (float + trigger bool), palette colours
    widget.h/.cpp       — Base widget: world coords, hit test, draw, child tree
    socket.h            — Socket widget: input/output, Value propagation, connected flag
    wire_layer.h/.cpp   — Wire storage, draft wiring, propagation, hit detection
    node.h/.cpp         — Draggable node container, socket layout, title/label drawing
    node_canvas.h/.cpp  — Canvas: owns nodes + wire layer, tick/render loop
    node_registry.h     — Factory registry for save/load and palette
    graph_io.h/.cpp     — JSON save and load
    palette.h/.cpp      — Left-panel node spawner
    slider.h/.cpp       — Horizontal value slider
    dial.h/.cpp         — Rotary dial
    histogram.h/.cpp    — Editable bar array
    selector.h          — Row of labelled toggle buttons
    timer_node.h        — TimerNode + LoopTimerNode (header-only)
    envelope_node.h     — EnvelopeNode ADSR (header-only)
    wave_node.h         — WaveNode (header-only)
    tone_node.h/.cpp    — ToneNode with SDL audio stream
```

**Tick order each frame:** clear output triggers → tick all nodes → propagate wires.  
Input triggers persist one frame (set by propagation, read by the receiving node next tick).
