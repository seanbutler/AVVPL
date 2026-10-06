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
- [x] Sequencer node — 8-step lane, CV output scaled by Max slider, Trig output per step
- [x] Tempo node — BPM slider, Swing dial, Tick (eighth note) and Beat (quarter note) outputs
- [x] Clock Divider node — /1 /2 /4 /8 trigger outputs from a single master clock
- [x] Pulse node — converts a trigger to a timed gate with Gate and Rel outputs
- [x] Envelope node — ADSR with exponential decay/release, Gate and Rel inputs, up to 8s decay
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
