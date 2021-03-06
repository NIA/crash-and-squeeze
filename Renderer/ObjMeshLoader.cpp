#include "ObjMeshLoader.h"
#include <string>
#include <fstream> // for ifstream
#include <cstring> // for memcmp, strtok
#include <cstdlib> // for atoi
#include "fast_atof.h"

// undefinde max macro from windows.h so that numeric_limits::max can be used
#undef max

using std::ifstream;
using std::vector;
using std::string;
using Assimp::fast_atof;

ObjMeshLoader::ObjMeshLoader(const TCHAR * filename, float4 color, float scale)
    : filename(filename), color(color), loaded(false), scale_(scale)
{
}

namespace
{

const char * FACE_DELIMITER = "/";

inline bool is_empty(const char * str)
{
    return str == nullptr || str[0] == 0;
}

inline float3 parse_float3(const string &x, const string &y, const string &z)
{
    return float3(fast_atof(x.c_str()),
                  fast_atof(y.c_str()),
                  fast_atof(z.c_str()));
}

}

void ObjMeshLoader::load()
{
    if(loaded)
        return;

    // Loader code inspired by SO answer http://stackoverflow.com/a/21954790/693538

    ifstream in(filename);
    if ( ! in )
        throw MeshError(filename, "Failed to open mesh file");
    
    vector<float3> positions;
    vector<float3> normals;
    // std::vector<float2> texcoords; // TODO: support texture coordinates

    string cmd;
    unsigned line_no = 1;
    for(;;)
    {
        in >> cmd;
        if (!in)
            break;

        if ("v" == cmd)
        {
            string x, y, z;
            in >> x >> y >> z;
            if ( ! in )
                throw MeshError(filename, "Failed to read vertex position from mesh file", line_no);
            positions.push_back(parse_float3(x, y, z)*scale_);
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
            string x, y, z;
            in >> x >> y >> z;
            if ( ! in )
                throw MeshError(filename, "Failed to read vertex normal from mesh file", line_no);
            normals.push_back(parse_float3(x, y, z));
        }
        else if ("f" == cmd )
        {
            Index pos_index, /*tex_index,*/ nrm_index;
            string face_str;
            char * ind_str;
            Vertex vertex;
            vertex.color = color;
            for (int iFace = 0; iFace < VERTICES_PER_TRIANGLE; ++iFace)
            {
                in >> face_str;
                if ( ! in )
                    throw MeshError(filename, "Failed to read face indices from mesh file", line_no);
                
                // Read position index. It must not be empty.
                // NB: modifying internal string buffer is not safe, but we are brave (strtok should not go out of initial string length)
                char *next_token = nullptr; // `context` argument of strtok_s
                ind_str = strtok_s(&face_str[0], FACE_DELIMITER, &next_token);
                pos_index = atoi(ind_str);
                if (pos_index < 1 || pos_index > positions.size())
                    throw MeshError(filename, "Incorrect face position index in mesh file", line_no);
                vertex.pos = positions[pos_index - 1]; // subtract 1 because OBJ uses 1-based arrays

                // Try to read (optional) texture coordinate index.
                ind_str = strtok_s(nullptr, FACE_DELIMITER, &next_token);
                if ( ! is_empty(ind_str) )
                {
                    // TODO: support texture coordinates
                    /* tex_index = atoi(ind_str));
                    if (tex_index < 1 || tex_index > texcoords.size())
                        throw MeshError(filename, "Incorrect face texcoord position index in mesh file", line_no);
                    vertex.texcoord = texcoords[tex_index - 1]; */ // TODO: support texture coordinates
                }

                // Try to read (optional) normal index
                ind_str = strtok_s(nullptr, FACE_DELIMITER, &next_token);
                if ( ! is_empty(ind_str) )
                {
                    nrm_index = atoi(ind_str);
                    if (nrm_index < 1 || nrm_index > normals.size())
                        throw MeshError(filename, "Incorrect face normal index in mesh file", line_no);
                    vertex.set_normal(normals[nrm_index - 1]);
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
        ++line_no;
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
        for (const Index & index : entry) {
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

void ObjMeshLoader::check_loaded() const
{
    if ( ! loaded )
        throw MeshError(filename, "Trying to work with mesh, but not loaded mesh file ");
}

void ObjMeshLoader::scale(float scale)
{
    check_loaded();
    for (Vertex & v: vertices)
        v.pos = v.pos * scale;
}

float3 ObjMeshLoader::get_dimensions() const
{
    check_loaded();
    float3 min_pos, max_pos;
    bool first_vertex = true;

    // find min and max pos
    for (const Vertex & v: vertices)
    {
        if (first_vertex)
        {
            min_pos = max_pos = v.pos;
            first_vertex = false;
        }
        else
        {
            if (v.pos.x < min_pos.x)
                min_pos.x = v.pos.x;
            if (v.pos.x > max_pos.x)
                max_pos.x = v.pos.x;
            if (v.pos.y < min_pos.y)
                min_pos.y = v.pos.y;
            if (v.pos.y > max_pos.y)
                max_pos.y = v.pos.y;
            if (v.pos.z < min_pos.z)
                min_pos.z = v.pos.z;
            if (v.pos.z > max_pos.z)
                max_pos.z = v.pos.z;
        }
    }

    return max_pos - min_pos;
}

const std::vector<Vertex> & ObjMeshLoader::get_vertices() const
{
    check_loaded();
    return vertices;
}

const std::vector<Index> & ObjMeshLoader::get_indices() const
{
    check_loaded();
    return indices;
}
