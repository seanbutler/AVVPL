# TODO

## Application

- [x] Save and Load should open a file dialog
- [ ] Additive load — merge a saved graph on top of the current canvas (Ctrl+Shift+O)
  - [ ] Assign each node a stable GUID (random uint64) at construction
  - [ ] Serialise GUID per node; resolve wires by GUID instead of array index
  - [ ] On additive load, incoming nodes keep their GUIDs; wires resolve correctly regardless of existing node count
  - [ ] Migrate existing save format: on load, if nodes lack GUIDs assign them sequentially so old files still work
- [ ] Node outline should pulse with Input Tick/CV etc
- [ ] Some Kind of Help System

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
- [x] Button node — manual Trig (press) / Rel (release); wire to Reset inputs to restart sequences
- [x] Waveform node — discrete waveform type selector (Sine/Sqr/Saw/Tri/Noise)
- [x] Sequencer CV — 16-step lane, CV output scaled by Max slider, Trig output per step
- [x] Sequencer Trig — 16-step gate sequencer, each step toggles on/off, fires Trig output
- [x] Sequencer Router — 16-step sequencer with one dedicated Trig output per step
- [x] Tempo node — BPM slider, Swing dial, Tick (eighth note) and Beat (quarter note) outputs
- [x] Clock Divider node — /1 /2 /4 /8 trigger outputs from a single master clock
- [x] Pulse node — converts a trigger to a timed gate with Gate and Rel outputs
- [x] Envelope node — ADSR with exponential decay/release, Gate and Rel inputs, up to 8s decay
- [ ] Random node — outputs a random float on each trigger
- [x] Quantise node — snaps Hz to the nearest note of a scale (Chrom/Major/Minor/Pentatonics/Blues) and root
- [ ] Euclidean sequencer — distributes K beats across N steps (Bjorklund algorithm), with rotation offset
- [ ] Probability sequencer — each of 16 steps has an independent chance to fire
- [ ] Arpeggiator — cycles through a set of held CV values on each trigger
- [ ] Shift register sequencer — Turing machine style; looping random pattern with lock control
- [ ] Variable loop length — all step sequencers should support a settable loop length (1–16); implemented as an integer-snapping slider stretched to the full width of the step lane, with cells beyond the active length dimmed

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



## Palette & visual clarity (prioritised)

Problem: every palette button has its own unrelated hue, the order is arbitrary, labels overflow, and nothing links palette colours to nodes on the canvas — the user has to read every label.

**P1 — structure (biggest readability win)**
- [x] Add a `category` field to `NodeTypeInfo` (Sound / Timing / Sequence / Utility / Display) and register nodes by category, replacing per-node colours
- [x] Order palette by category with small dim section headers (match README groupings; move Timer/Loop/Pulse next to Tempo/Clock Div)
- [x] One muted colour per category (Sound amber, Timing teal, Sequence violet, Utility slate, Display blue), defined as named `pal::` constants

**P2 — legibility**
- [x] Restyle buttons: dark neutral fill + 3–4px category-coloured left stripe + light text (fixes white-on-yellow contrast)
- [x] Fix label overflow: widen panel to ~130px and shorten labels ("Seq CV", "Seq Trig", "Seq Router")
- [x] Reduce button height 32 → ~24px so the full list fits comfortably
- [x] Separate Save/Load from node buttons: outline/ghost style, pinned to the bottom of the panel

**P3 — feedback & canvas link**
- [x] Hover highlight — `hoveredIdx` exists in `palette.h` but is never set; wire it from mouse motion
- [x] Category-coloured stripe/tint on node title bars so palette colours map to nodes on the canvas
- [x] `Palette::contains` should check `y < panelH` (see refactorings below)

**P4 — later**
- [ ] Collapsible categories and/or type-to-filter search once the node count grows
- [ ] Drag from palette to place a node instead of cascading spawn position
- [ ] Colour sockets/wires by signal type (audio / CV / trigger)
- [ ] Move category colours and palette metrics into the JSON style doc (above)

## Sliders & controls

- [x] Sliders display their current value alongside the label
- [x] Shift+drag on any slider snaps to discrete steps auto-scaled to the slider's range
- [x] Number node has configurable Min/Max sliders so its output range can be set per-use

## Infrastructure

- [x] Fixed timestep logic loop — node ticks run at 1ms independent of render frame rate
- [x] Zoom clamped to 0.35×–3× to prevent extreme SW rasterisation cost at low zoom
- [x] Remove stale `buildScene` function from main.cpp
- [x] Sequencer zero-value steps no longer fire a trigger (eliminates click artifacts)
- [ ] Socket `connected` flag should survive save/load (currently not serialised)
- [ ] Graceful handling of unknown node types on load (currently silently skipped)
- [ ] Switch ThorVG canvas from SW to GL backend — rendering currently CPU-only; all rasterisation happens in software and is uploaded as a texture each frame

