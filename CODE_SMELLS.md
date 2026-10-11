
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
Each dot is a separate `tvg::Shape` — ~1,750 allocations per frame at default zoom. Replace with a single `tvg::Shape` containing all horizontal and vertical lines as `moveTo`/`lineTo` subpaths (~86 lines → 1 shape). Scale the grid step with zoom so lines never get closer than ~20 px on screen: `float screenStep = step * camZoom; while (screenStep < 20.f) screenStep *= 2;` then derive world step as `screenStep / camZoom`.

### Magic numbers in render code
Several raw literals should be named constants in `pal::` or adjacent:
- Grid dot color `{45, 45, 55, 110}` and step `24.f` (`node_canvas.cpp:101,110`)
- Palette background height `9999.f` — should use canvas height (`palette.cpp:112`)
- Spawn wrap threshold `500.f` world-space units (`palette.cpp:69`)
- Socket inner-dot ratio `0.35f` (`socket.h:44`)
- `dispAmp` smoothing coefficient `0.3f` (`tone_node.cpp:49`) — depends implicitly on the 1ms tick; should at least have a comment tying it to `kFixedStep`

### Wire save is O(W × N × S) (`graph_io.cpp:73-84`)
For each wire, the save loop re-scans all nodes and their sockets to find socket ownership. Build a reverse map `SocketWidget* → {node_idx, sock_idx}` once before the wire loop to reduce it to O(N×S + W).

### Redundant socket hit test in `onMouseDown` (`node_canvas.cpp:177-190`)
After the right-click branch returns, the left-click path calls `socketAt(pt)` at line 177, then immediately calls `hitTest(pt)` and `dynamic_cast<SocketWidget*>(hit)` at line 186. The second path can never fire because `socketAt` already handled it. Remove the `hitTest` + cast fallback.

### `WireLayer::findSocketAt` is dead code (`wire_layer.cpp:70-77`)
This method accepts a widget root list and searches for a socket, but `NodeCanvas::socketAt` fills the same role and is what the canvas actually calls. `findSocketAt` is never invoked. Delete it or consolidate so `NodeCanvas::socketAt` delegates to it.

### ~~`Palette::contains` ignores Y axis~~ ✓ (`palette.cpp:42-44`)
`(void)y; return x < kW;` means every click anywhere in the palette column — even below all
buttons — is consumed and blocked from reaching the canvas. Check `y < panelH` so the canvas receives events below the panel.

### Audio callback uses fixed stack buffer (`tone_node.cpp:60-61`)
`float buf[4096]` is stack-allocated and `frames` is silently clamped to 4096. If SDL requests more than 4096 frames the extra samples are dropped without any warning. Add an assertion, or allocate the buffer dynamically based on `additionalBytes`.
