#pragma once
#include "main.h"
#include "Vertex.h"
#include <vector>
#include <list>

class ObjMeshLoader
{
private:
    const TCHAR * filename;
    float4 color;
    float scale_;
    bool loaded;

    std::vector<Vertex> vertices;
    std::vector<Index>  indices;
    // Kind of hash table: vertex position index is used as hash (as index in `vertex_cache` array). Multiple vertices with same position index are stored as linked list
    typedef std::list<Index> CacheEntry; // TODO: can be changed to std::forward_list if using c++11 compliant compiler
    std::vector<CacheEntry> vertex_cache;

    // Returns index of `v` in `vertices` array: if it doesn't exists yet, it is added to the end
    Index find_or_add_vertex(const Vertex &v, Index hash);
    void check_loaded() const;
public:
    // Creates mesh loader for file `filename`
    // Each vertex is painted with `color` and its position is multiplied by `scale`
    ObjMeshLoader(const TCHAR * filename, float4 color, float scale = 1);

    // Loads model from file `filename`. NB: only one mesh part is supported!
    void load();

    // In addition to `scale` parameter of constructor, mesh can be scaled after loading
    void scale(float scale);

    // Gets max sizes of mesh along each axis
    float3 get_dimensions() const;

    const TCHAR * get_filename() const { return filename; }
    const std::vector<Vertex> & get_vertices() const;
    const std::vector<Index>  & get_indices() const;
};

