# TODO

## Audio DSP

- [x] Implement Oscillator node — sine/square/saw/tri/noise, Freq Mod + Amp Mod CV inputs
- [ ] Implement Filter node — low-pass/high-pass with cutoff and resonance
- [ ] Implement Output node — stereo mixer that routes to SDL audio device
- [ ] Implement Spectral node — per-band frequency shaping
- [ ] Audio buffer type — pass a frame of samples between nodes rather than a single float
- [ ] Multiple Tone nodes should share one SDL audio device and mix

## Graph editing

- [ ] Socket type system — Trigger / Float / Audio with colour coding and connection validation
- [x] Canvas pan and zoom — middle mouse button
- [ ] Select multiple nodes (rubber-band drag) and move/delete as a group
- [ ] Duplicate node (e.g. Ctrl+D)
- [ ] Undo/redo

## Sequencing

- [x] Number node — scalar float output via slider
- [x] Waveform node — discrete waveform type selector (Sine/Sqr/Saw/Tri/Noise)
- [x] Sequencer CV — 16-step lane, CV output scaled by Max slider, Trig output per step
- [x] Sequencer Trig — 16-step gate sequencer, each step toggles on/off, fires Trig output
- [x] Sequencer Router — 16-step sequencer with one dedicated Trig output per step
- [x] Tempo node — BPM slider, Swing dial, Tick (eighth note) and Beat (quarter note) outputs
- [x] Clock Divider node — /1 /2 /4 /8 trigger outputs from a single master clock
- [x] Pulse node — converts a trigger to a timed gate with Gate and Rel outputs
- [x] Envelope node — ADSR with exponential decay/release, Gate and Rel inputs, up to 8s decay
- [ ] Random node — outputs a random float on each trigger
- [ ] Quantise node — snaps a float to the nearest musical note (CV → Hz)
- [ ] Euclidean sequencer — distributes K beats across N steps (Bjorklund algorithm), with rotation offset
- [ ] Probability sequencer — each of 16 steps has an independent chance to fire
- [ ] Arpeggiator — cycles through a set of held CV values on each trigger
- [ ] Shift register sequencer — Turing machine style; looping random pattern with lock control
- [ ] Variable loop length — all step sequencers should support a settable loop length (1–16)

## Oscillators / Sound sources

- [x] Tone node — VCO with Freq, Waveform, Amp, and Detune inputs
- [ ] LFO node — low-frequency oscillator for slow modulation (rate, depth, waveform)
- [ ] Wavetable oscillator — cycles through stored waveforms

## Filters

- [ ] VCF node — low-pass/high-pass/band-pass with Cutoff and Resonance CV inputs
- [ ] Ladder filter — Moog-style resonant low-pass

## Modulators / Utilities

- [ ] Sample & hold — captures input CV on each trigger
- [ ] Slew limiter — smooths abrupt CV changes (portamento / glide)
- [ ] Attenuverter — scales and/or inverts a CV signal
- [ ] Mixer node — sums multiple audio or CV signals with per-input level
- [ ] Logic node — AND / OR / XOR on trigger signals
- [ ] Comparator — fires trigger when input exceeds threshold

## Effects

- [ ] Delay node — echo with time and feedback controls
- [ ] Reverb node
- [ ] Ring modulator — multiplies two audio signals

## UI polish

- [ ] Right-align output socket labels precisely (need ThorVG text metrics or pre-measured widths)
- [ ] Node resize handle
- [ ] Highlight compatible sockets while dragging a wire
- [ ] Show wire value on hover
- [ ] Dark/light theme toggle
- [ ] Separate visual style into a JSON style doc — move `pal::` colours, sizes and font settings out of code into a loadable file (prerequisite for theming)

## Infrastructure

- [ ] Remove stale `buildScene` function from main.cpp
- [ ] Socket `connected` flag should survive save/load (currently not serialised)
- [ ] Graceful handling of unknown node types on load (currently silently skipped)

## Potential refactorings (code smells)

### Global static font state (`node_canvas.cpp:7-17`)
`s_fontLoaded` and `s_fontName` are file-level statics accessed via static class methods on
`NodeCanvas`. All drawing code couples to this global, and a second `NodeCanvas` instance would
silently share font state. Move them to instance members; pass or query via the canvas reference
that widgets already have access to through the draw call chain.

### `dynamic_cast` as primary dispatch (10+ sites)
`dynamic_cast<SocketWidget*>` and `dynamic_cast<Node*>` appear throughout `node_canvas.cpp`,
`node.cpp`, `graph_io.cpp`, and `wire_layer.cpp`. The heaviest uses are the "find parent node"
walk (`node_canvas.cpp:156`) and the socket/widget partition in `graph_io.cpp:21-34`. A small
virtual `isSocket()` method, or storing sockets in a dedicated `Node::sockets` vector separate
from body `children`, would eliminate most casts and make the partition O(1) rather than O(children).

### Node `title` doubles as registry type key (`graph_io.cpp:45`)
`jn["type"] = n->title` serialises the display name as the type key used for loading. These
coincidentally match today ("Tone", "Envelope", etc.) but are distinct concepts. If a node is
ever renamed for display, saved graphs will silently fail to load. Add a separate `typeId` field
to `Node` (set by the registry at construction) and serialise that instead.

### Label width estimated with magic `5.5f` multiplier (`node.cpp:22`, `slider.cpp:38`)
Output socket label right-alignment and slider value right-alignment both approximate text width
as `label.size() * 5.5f`. This is inconsistent across font sizes and character sets. Extract a
named constant `kCharWidthApprox9pt` at minimum; replace with actual ThorVG text metrics if they
become available.

### `M_PI` instead of project `kPi` (`tone_node.cpp:83`)
`fillBuffer` uses POSIX `M_PI` for the sine calculation while `types.h:47` defines `kPi` for
exactly this purpose. Replace `M_PI` with `kPi` for consistency and portability.

### ~~Replace grid dots with grid lines (`node_canvas.cpp:100-114`)~~ ✓
Each dot is a separate `tvg::Shape` — ~1,750 allocations per frame at default zoom. Replace with
a single `tvg::Shape` containing all horizontal and vertical lines as `moveTo`/`lineTo` subpaths
(~86 lines → 1 shape). Scale the grid step with zoom so lines never get closer than ~20 px on
screen: `float screenStep = step * camZoom; while (screenStep < 20.f) screenStep *= 2;` then
derive world step as `screenStep / camZoom`.

### Magic numbers in render code
Several raw literals should be named constants in `pal::` or adjacent:
- Grid dot color `{45, 45, 55, 110}` and step `24.f` (`node_canvas.cpp:101,110`)
- Palette background height `9999.f` — should use canvas height (`palette.cpp:112`)
- Spawn wrap threshold `500.f` world-space units (`palette.cpp:69`)
- Socket inner-dot ratio `0.35f` (`socket.h:44`)
- `dispAmp` smoothing coefficient `0.3f` (`tone_node.cpp:49`) — depends implicitly on the 1ms tick; should at least have a comment tying it to `kFixedStep`

### Wire save is O(W × N × S) (`graph_io.cpp:73-84`)
For each wire, the save loop re-scans all nodes and their sockets to find socket ownership. Build
a reverse map `SocketWidget* → {node_idx, sock_idx}` once before the wire loop to reduce it to
O(N×S + W).

### Redundant socket hit test in `onMouseDown` (`node_canvas.cpp:177-190`)
After the right-click branch returns, the left-click path calls `socketAt(pt)` at line 177, then
immediately calls `hitTest(pt)` and `dynamic_cast<SocketWidget*>(hit)` at line 186. The second
path can never fire because `socketAt` already handled it. Remove the `hitTest` + cast fallback.

### `WireLayer::findSocketAt` is dead code (`wire_layer.cpp:70-77`)
This method accepts a widget root list and searches for a socket, but `NodeCanvas::socketAt`
fills the same role and is what the canvas actually calls. `findSocketAt` is never invoked.
Delete it or consolidate so `NodeCanvas::socketAt` delegates to it.

### `Palette::contains` ignores Y axis (`palette.cpp:42-44`)
`(void)y; return x < kW;` means every click anywhere in the palette column — even below all
buttons — is consumed and blocked from reaching the canvas. Check `y < panelH` so the canvas
receives events below the panel.

### Audio callback uses fixed stack buffer (`tone_node.cpp:60-61`)
`float buf[4096]` is stack-allocated and `frames` is silently clamped to 4096. If SDL requests
more than 4096 frames the extra samples are dropped without any warning. Add an assertion, or
allocate the buffer dynamically based on `additionalBytes`.
