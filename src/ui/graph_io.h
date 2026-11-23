#pragma once
#include "node_canvas.h"
#include <string>

// Save the current graph to a JSON file. Returns false on error.
bool graphSave(const NodeCanvas& nc, const std::string& path);

// Clear the canvas and load a graph from a JSON file. Returns false on error.
bool graphLoad(NodeCanvas& nc, const std::string& path);

// Merge a graph from a JSON file on top of the current canvas (no clear).
bool graphLoadAdditive(NodeCanvas& nc, const std::string& path);

// Duplicate a single node onto the canvas, offset by (dx, dy) world units.
void duplicateNode(NodeCanvas& nc, Node* n, float dx = 24.f, float dy = 24.f);
