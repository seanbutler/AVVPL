# TODO

## Audio DSP

- [ ] Implement Oscillator node — sine/square/saw/tri at given frequency and amplitude
- [ ] Implement Filter node — low-pass/high-pass with cutoff and resonance
- [ ] Implement Output node — stereo mixer that routes to SDL audio device
- [ ] Implement Spectral node — per-band frequency shaping
- [ ] Audio buffer type — pass a frame of samples between nodes rather than a single float
- [ ] Multiple Tone nodes should share one SDL audio device and mix

## Graph editing

- [ ] Socket type system — Trigger / Float / Audio with colour coding and connection validation
- [ ] Canvas pan and zoom (middle-mouse drag or scroll wheel)
- [ ] Select multiple nodes (rubber-band drag) and move/delete as a group
- [ ] Duplicate node (e.g. Ctrl+D)
- [ ] Undo/redo

## Sequencing

- [ ] Sequencer node — steps through a list of values (frequencies/triggers) on each Tick
- [ ] Random node — outputs a random float on each trigger
- [ ] Quantise node — snaps a float to the nearest musical note (CV → Hz)

## UI polish

- [ ] Right-align output socket labels precisely (need ThorVG text metrics or pre-measured widths)
- [ ] Node resize handle
- [ ] Highlight compatible sockets while dragging a wire
- [ ] Show wire value on hover
- [ ] Dark/light theme toggle

## Infrastructure

- [ ] Remove stale `buildScene` function from main.cpp
- [ ] Socket `connected` flag should survive save/load (currently not serialised)
- [ ] Graceful handling of unknown node types on load (currently silently skipped)
