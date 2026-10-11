# VISUALS TODO

A visual compositing pipeline built on the same node-graph system as the audio engine.
CV outputs from Tempo, Sequencer, Envelope etc. drive visual parameters directly,
keeping graphics synchronised to the music.

## Architecture

- [ ] Add a **Visual** category to the node palette
- [ ] Define a `ShapeValue` type alongside `Value` — carries a `tvg::Shape*` or scene subtree pointer between visual nodes
- [ ] Render Target node — owns a ThorVG SW canvas, collects incoming scene tree, rasterises to an SDL texture each frame
- [ ] Visual nodes push to a `tvg::Scene` during `tick()` rather than generating audio samples
- [ ] Decide on coordinate system — normalised 0–1 UV space vs. pixel space

## Primitive nodes (sources)

- [ ] Rectangle — X, Y, Width, Height, Corner Radius, Fill colour CV inputs
- [ ] Circle — CX, CY, Radius, Fill colour CV inputs
- [ ] Polygon — already exists; promote to visual pipeline (Sides, Radius, Rotation, Fill)
- [ ] Line — X1, Y1, X2, Y2, Stroke Width, Colour
- [ ] Text — string input, X, Y, Size, Colour
- [ ] Image — loads a file, X, Y, Scale, Rotation
- [ ] Gradient — linear or radial, Start/End colour, Angle CV input

## Transform nodes

- [ ] Translate — X, Y offsets applied to incoming shape
- [ ] Rotate — angle CV input (degrees); pivot X, Y
- [ ] Scale — uniform or non-uniform X/Y scale factor
- [ ] Repeat / Tile — duplicate incoming shape N times in a grid or radial pattern

## Constructive combination (CSG)

- [ ] Union — merge two shapes into one
- [ ] Intersection — output only the overlapping region of two shapes
- [ ] Difference — subtract one shape from another
- [ ] Blend / Dissolve — opacity mix between two scenes (Alpha CV input)
- [ ] Mask — use one shape as a clip region for another

## Colour & effects

- [ ] Colour node — HSL or RGB sliders with CV inputs per channel; outputs a colour value
- [ ] Hue Shift — rotates hue of incoming scene by CV amount
- [ ] Brightness / Contrast — CV-driven post-processing on a rendered texture

## Render output

- [ ] Render Target node — the visual sink; rasterises the incoming scene tree to a texture displayed on canvas
- [x] Fullscreen output mode — F11 toggles fullscreen
- [ ] Record to PNG sequence — dump frames to disk at a set frame rate

## CV linkage (music → visuals)

- [ ] All visual parameter inputs accept the same CV float signals as audio nodes
- [ ] Envelope → shape Scale or Opacity for beat-synced pulse effects
- [ ] Sequencer CV → Polygon Sides or Rotation for per-step shape changes
- [ ] Tempo Tick → Translate or Hue Shift trigger for strobing effects
- [ ] Scope node waveform → Line primitive path for audio-reactive visuals
