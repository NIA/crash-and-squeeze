#include "ObjMeshLoader.h"
#include <string>
#include <cstring> // for memcmp

// undefinde max macro from windows.h so that numeric_limits::max can be used
#undef max

// TODO: use TCHAR/wchar_t instead of char
ObjMeshLoader::ObjMeshLoader(const char * filename, float4 color)
    : filename(filename), color(color), loaded(false)
{
}

void ObjMeshLoader::load()
{
    if(loaded)
        return;

    // Loader code inspired by SO answer http://stackoverflow.com/a/21954790/693538

    std::ifstream in(filename);
    if ( ! in )
        throw MeshError(filename, "Failed to open mesh file ");
    
    std::vector<float3> positions;
    std::vector<float3> normals;
    // std::vector<float2> texcoords; // TODO: support texture coordinates

    std::string cmd;
    for(;;)
    {
        in >> cmd;
        if (!in)
            break;

        if ("v" == cmd)
        {
            float x, y, z;
            in >> x >> y >> z;
            if ( ! in )
                throw MeshError(filename, "Failed to read vertex position from mesh file "); // TODO: line number
            positions.push_back(float3(x, y, z));
        }
        /* TODO: support texture coordinates
        else if ("vt" == cmd)
        {
            float u, v, w;
            in >> u >> v >> w; // TODO: what if `w` is not specified?
            texcoords.push_back(float2(u, v));
        }*/
        else if ("vn" == cmd)
        {
            float x, y, z;
            in >> x >> y >> z;
            if ( ! in )
                throw MeshError(filename, "Failed to read vertex normal from mesh file "); // TODO: line number
            normals.push_back(float3(x, y, z));
        }
        else if ("f" == cmd )
        {
            Index pos_index, tex_index, nrm_index;
            Vertex vertex;
            vertex.color = color;
            for (int iFace = 0; iFace < VERTICES_PER_TRIANGLE; ++iFace)
            {
                in >> pos_index;
                if ( ! in )
                    throw MeshError(filename, "Failed to read face position index from mesh file "); // TODO: line number
                if (pos_index < 1 || pos_index > positions.size())
                    throw MeshError(filename, "Incorrect face position index in mesh file "); // TODO: line number
                vertex.pos = positions[pos_index - 1]; // subtract 1 because OBJ uses 1-based arrays

                if ('/' == in.peek())
                {
                    in.ignore(); // skip '/'

                    if ('/' != in.peek()) // optional texture coordinate
                    {
                        in >> tex_index;
                        if ( ! in )
                            throw MeshError(filename, "Failed to read face texcoord index from mesh file "); // TODO: line number
                        /* if (tex_index < 1 || tex_index > texcoords.size())
                            throw MeshError(filename, "Incorrect face texcoord position index in mesh file "); // TODO: line number
                        vertex.texcoord = texcoords[tex_index - 1]; */ // TODO: support texture coordinates
                    }

                    if ('/' == in.peek())
                    {
                        in.ignore(); // skip '/'
                        in >> nrm_index;
                        if ( ! in )
                            throw MeshError(filename, "Failed to read face normal index from mesh file "); // TODO: line number
                        if (nrm_index < 1 || nrm_index > normals.size())
                            throw MeshError(filename, "Incorrect face normal index in mesh file "); // TODO: line number
                        vertex.set_normal(normals[nrm_index - 1]);
                    }
                }

                indices.push_back(find_or_add_vertex(vertex, pos_index));
            }
        }
        else
        {
            // Comment or unimplemented/unrecognized command - ignore
        }
        // In any case, skip everything that is left until the end of line
        in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    loaded = true;
    vertex_cache.clear();
}

namespace
{
    // it is the same vertex if it has same pos and normal [and texcoord]
    bool same_vertex(const Vertex &a, const Vertex &b)
    {
        return 0 == memcmp(&a.pos, &b.pos, sizeof(a.pos)) && 0 == memcmp(&a.normal, &b.normal, sizeof(a.normal)); // && a.texcoord = b.texcoord // TODO: support texture coordinates
    }
}

Index ObjMeshLoader::find_or_add_vertex(const Vertex &v, Index hash)
{
    // Hash table optimization taken from %DirectXSDK%\Samples\C++\Direct3D10\MeshFromOBJ10\MeshLoader10.cpp
    if ( vertex_cache.size() > hash )
    {
        // Indices of vertices with same hash will be in the list `entry`
        const CacheEntry & entry = vertex_cache[hash];
        // So go throw this indices until we find the same vertex
        for (CacheEntry::const_iterator it = entry.begin(), end = entry.end(); it != end; ++it) {
            const Index index = *it;
            const Vertex & old_vertex = vertices[index];
            if (same_vertex(old_vertex, v))
                return index;
        }
    }
    else
    {
        // if hash value is more than cache size => grow cache
        vertex_cache.resize(hash + 1);
    }

    // If we got here => no vertex with such hash value is in cache.
    // Add it to vertices array...
    vertices.push_back(v);
    // ...and its index (which is last index) to the cache
    Index index = vertices.size() - 1;
    vertex_cache[hash].push_back(index);
    return index;
}

const std::vector<Vertex> & ObjMeshLoader::get_vertices() const
{
    if ( ! loaded )
        throw MeshError(filename, "Trying to work with mesh, but not loaded mesh file ");
    return vertices;
}

const std::vector<Index> & ObjMeshLoader::get_indices() const
{
    if ( ! loaded )
        throw MeshError(filename, "Trying to work with mesh, but not loaded mesh file ");
    return indices;
}
