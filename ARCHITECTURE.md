# AVVPL — Architecture Notes

A living document capturing the current architecture, known structural issues, and a
prioritised refactoring roadmap. Updated as the codebase evolves.

---

## 1. Current Architecture

AVVPL is a modular node-graph audio/CV synthesiser built with SDL3, ThorVG (vector
graphics), and C++17. Five major subsystems:

| Subsystem | Files | Responsibility |
|---|---|---|
| **Widget Tree** | `widget.h/.cpp`, `node.h/.cpp`, `socket.h` | Hierarchical UI components, world coordinates, hit testing |
| **Node Canvas** | `node_canvas.h/.cpp`, `wire_layer.h/.cpp` | Graph container, data-flow tick, camera/zoom, mouse dispatch |
| **Audio Engine** | `tone_node.h/.cpp` | SDL3 audio stream, waveform synthesis, atomic inter-thread comms |
| **Transport** | `transport_bar.h`, `NodeCanvas::Transport` | Global play/pause/stop state, fixed 1ms tick accumulator |
| **Persistence** | `graph_io.h/.cpp` | JSON save/load for nodes, sockets, widgets, and wires |

### Core data flow

```
Mouse events
    → NodeCanvas::onMouseDown/Move/Up
    → Widget hit test → widget.onMouseDown / onDrag

Fixed 1ms tick (main.cpp accumulator):
    → NodeCanvas::tick(dt)
        1. Clear output triggers (stale from previous tick)
        2. node->tick(dt) for every node in registration order
        3. WireLayer::propagate() — copy output values to connected inputs

Rendering (every frame):
    → nc.render()
        → camera transform applied
        → node->drawAll() → ThorVG shapes pushed to scene
        → scene rasterised to CPU pixel buffer
    → SDL_UpdateTexture → GPU texture upload
    → SDL_RenderTexture → blit to screen

Audio (background thread, ~5.8ms period at 256 frames / 44.1kHz):
    → SDL audio callback → ToneNode::fillBuffer()
        → reads atomFreq / atomAmp / atomWave (set by main-thread tick)
        → writes to SDL audio device stream
```

---

## 2. Known Issues

### HIGH — Audio mixed into ToneNode

`ToneNode` is simultaneously a UI widget, a CV processor, and a real-time audio
synthesiser. It owns an SDL audio stream, manages atomic variables, and performs
waveform synthesis — all in one class.

- `tone_node.cpp:6` — `static std::vector<ToneNode*> gToneNodes` global registry,
  read by `ScopeNode` without thread synchronisation
- Audio domain concerns (atomics, ring buffers, sample rates) leak into the node
  definition, making it impossible to reuse synthesis without GUI
- `ScopeNode` couples to `ToneNode` internals rather than an audio tap interface

**Fix:** Extract an `AudioEngine` singleton that owns device management and buffer
filling. `ToneNode` registers a generator function; `ScopeNode` reads from an
`AudioEngine` tap buffer. Eliminates `gToneNodes`.

---

### HIGH — Widget tree mixes rendering, input, state, and serialisation

`Widget` (widget.h) couples four unrelated concerns:
- Geometry and hierarchy (localPos, width, height, children)
- Event handling (onMouseDown, onMouseMove, onDrag)
- Rendering (draw, drawAll)
- Serialisation (toJson, fromJson)

Display-only widgets (`ProgressRingWidget`, `ScopeDisplayWidget`,
`StepIndicatorWidget`) implement stub `onDrag`/`contains` to satisfy the interface.
Node state is exposed to widgets via lambda captures; if a node's state changes
mid-tick, the widget may render stale data.

**Fix:** Pass a `RenderContext` to `draw()`; state lives on the node and is injected
at draw time rather than captured. Optionally split `Widget` into separate
`DrawableWidget` and `InteractiveWidget` traits.

---

### HIGH — Global ToneNode registry

`tone_node.cpp:6`: `static std::vector<ToneNode*> gToneNodes` is a file-level static
shared between ToneNode (written on main thread) and ScopeNode (read on main thread,
but dangerously adjacent to audio thread usage). Multiple independent canvas instances
would corrupt each other.

**Fix:** Fold into `AudioEngine` (see above), or pass a canvas-scoped registry.

---

### MEDIUM — Node execution order ignores wire dependencies

Nodes tick in the order they were registered (palette order). A node wired downstream
of another reads the upstream node's output from the **previous tick** — one-frame
latency accumulates for every hop in a chain.

Example: Tempo → Clock Div → Sequencer → Quantise → Tone is a 4-hop chain with 4ms
of latency at the 1ms tick rate.

**Fix:** Topological sort (Kahn's algorithm) on the wire graph, cached and invalidated
when wires are added or removed. `NodeCanvas::tick()` iterates the sorted order.

---

### MEDIUM — Widget onChange callbacks bypass the tick system

Slider and dial `onChange` lambdas mutate node state and write to output sockets
immediately on mouse move, outside of `tick()`. This violates the single execution
model and means UI interactions can change values mid-frame.

Example (`number_node.h:19`):
```cpp
sMin->onChange = [this](float v) {
    val->min = v;
    outValue->value.f = val->value;  // direct output write outside tick()
};
```

**Fix:** onChange should write to a "pending" field; tick() flushes pending values
into outputs. For interactive responsiveness, the pending write can still update
display immediately but not propagate until tick().

---

### MEDIUM — Serialisation is per-node boilerplate

Every node that needs custom save/load manually implements `toJson`/`fromJson` with no
versioning, no schema evolution, and no compile-time safety. Adding a field to a node
requires updating serialisation manually or saves will silently lose data.

Node title doubles as the registry type key (`graph_io.cpp:48`):
```cpp
jn["type"] = n->title;
```
Renaming a node's display title silently breaks all saved graphs.

**Fix:** Add a separate `typeId` field to `Node` set at construction by the registry.
Long-term: a schema registry that declares node parameters declaratively, enabling
automatic save/load and versioning.

---

### MEDIUM — NodeCanvas does too much

`NodeCanvas` is responsible for: owning all nodes, managing the wire layer, handling
SDL mouse events, applying the camera transform, rendering, transport state, hit
testing, and mouse capture. It is the god object of the system.

**Fix:**
- Extract `Transport` class (partially done via `TransportBar`)
- Extract a `GraphController` for mouse-event dispatch
- Keep `NodeCanvas` as a pure graph data structure; separate the view/controller

---

### MEDIUM — Node socket discovery via dynamic_cast

`graph_io.cpp:21–26` discovers sockets by walking the child widget tree and casting:
```cpp
for (auto& c : n->children)
    if (auto* s = dynamic_cast<SocketWidget*>(c.get())) out.push_back(s);
```
10+ `dynamic_cast` sites across `node_canvas.cpp`, `node.cpp`, `graph_io.cpp`,
`wire_layer.cpp`. Sockets must be direct `SocketWidget` children — if this structural
assumption is ever violated, serialisation silently breaks.

**Fix:** Add `Node::inputs()` and `Node::outputs()` query methods backed by explicit
`std::vector<SocketWidget*>` members populated in `addInput`/`addOutput`.

---

### LOW — Text rendering scattered and width-estimated with magic constant

Font loading and text push code is duplicated in 10+ files. Text width is approximated
as `label.size() * 5.5f` (e.g. `node.cpp:22`, `slider.cpp:38`) — inconsistent across
font sizes, wrong for non-ASCII, and will misalign if the font changes.

**Fix:** `TextRender` utility class with `text(scene, str, pos, size, color)` and
`textWidth(str, size)` using actual ThorVG text metrics once available.

---

### LOW — Circular dependency Node ↔ NodeCanvas

`Node` is owned by `NodeCanvas`; `ScopeNode` (and potentially others) query
`NodeCanvas` internals (e.g. `gToneNodes`). Nodes cannot be instantiated or tested
without a full canvas.

**Fix:** Dependency injection — pass a `GraphContext&` to nodes that need canvas
services rather than having them reach into globals.

---

## 3. Refactoring Roadmap

### Phase 1 — Quick wins (low risk, high payoff)

- [ ] Add `Node::inputs()` / `Node::outputs()` — removes dynamic_cast from graph_io
  and wire_layer, makes socket discovery O(1)
- [ ] Add separate `typeId` to `Node` — decouple display title from serialisation key
- [ ] `TextRender` utility — consolidate 10+ sites of repeated font/text push code
- [ ] Topological tick ordering — eliminate one-frame latency in wire chains

### Phase 2 — Structural

- [ ] Extract `AudioEngine` — move SDL audio device, stream management, and waveform
  synthesis out of `ToneNode`; `ScopeNode` reads from engine tap
- [ ] Eliminate `gToneNodes` global — fold into `AudioEngine` or canvas-scoped registry
- [ ] Extract `Transport` class from `NodeCanvas`

### Phase 3 — Architecture

- [ ] Decouple widget rendering from state — `RenderContext` injected at draw time
- [ ] Separate node logic from UI — `*Logic` classes that can run headless
- [ ] Schema-driven serialisation — declarative parameter registry, versioning,
  automatic save/load

### Long-term

- [ ] Typed socket data flow — `Socket<float>`, `Socket<Trigger>`, `Socket<AudioBuffer>`
  instead of the universal `Value` struct; type mismatches become compile errors
- [ ] `StatelessNode` / `IntegratorNode` traits — allow the scheduler to optimise
  execution order based on declared node behaviour
- [ ] Node macro DSL — declare inputs/outputs/params with minimal boilerplate,
  automatic registration, serialisation, and UI binding

---

## 4. Threading Model

| Thread | Responsibilities | Shared State |
|---|---|---|
| **Main** | Tick, render, UI events, file I/O | Node state, socket values, widget state |
| **SDL Audio** | `ToneNode::fillBuffer()` at callback rate | `atomFreq`, `atomAmp`, `atomWave` (std::atomic) |
| **File Dialog** | SDL native dialog callback | `PendingFile::path` (std::mutex protected) |

The audio thread reads only atomic values written by the main thread. No locks are
held across the boundary. The ring buffer used by `ScopeNode` to display the waveform
is written by the audio thread and read by the main thread — currently without explicit
synchronisation (benign in practice for a display buffer, but formally a data race).

---

## 5. Serialisation Format

Graphs are saved as JSON with the structure:

```json
{
  "nodes": [
    {
      "type": "Tone",
      "guid": "a1b2c3d4e5f60718",
      "x": 100.0, "y": 200.0,
      "sockets": [{"dir": "in", "idx": 0, "val": 440.0}, ...],
      "widgets": [0.5]
    }
  ],
  "wires": [
    {"fg": "a1b2c3d4e5f60718", "fi": 0, "tg": "b2c3d4e5f6071829", "ti": 0}
  ]
}
```

**Known weaknesses:**
- `type` is the node's display title — renaming breaks existing saves
- ~~Wires reference nodes by array index~~ — **resolved**: wires now use 64-bit GUIDs;
  old saves without GUIDs fall back to index resolution automatically
- `connected` flag on input sockets is not serialised — wired sockets start
  disconnected until the wire is re-created on load
- No format version field — schema changes silently corrupt old saves
