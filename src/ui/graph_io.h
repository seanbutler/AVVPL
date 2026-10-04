#pragma once
#include "node_canvas.h"
#include <string>

// Save the current graph to a JSON file. Returns false on error.
bool graphSave(const NodeCanvas& nc, const std::string& path);

// Clear the canvas and load a graph from a JSON file. Returns false on error.
bool graphLoad(NodeCanvas& nc, const std::string& path);
