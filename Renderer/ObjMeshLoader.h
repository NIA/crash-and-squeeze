#pragma once
#include "main.h"
#include "Vertex.h"
#include <vector>

class ObjMeshLoader
{
private:
    const char * filename;
    float4 color;
    bool loaded;

    std::vector<Vertex> vertices;
    std::vector<Index>  indices;

    // Returns index of `v` in `vertices` array: if it doesn't exists yet, it is added to the end
    Index find_or_add_vertex(const Vertex &v);
public:
    // Creates mesh loader for file `filename`
    ObjMeshLoader(const char * filename, float4 color);

    // Loads model from file `filename`. NB: only one mesh part is supported!
    void load();

    const char * get_filename() const { return filename; }
    const std::vector<Vertex> & get_vertices() const;
    const std::vector<Index>  & get_indices() const;
};

