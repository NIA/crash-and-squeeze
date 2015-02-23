#include "ObjMeshLoader.h"
#include <string>

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
        in >> cmd; // TODO: maybe read line-by-line with std::getline, like in http://stackoverflow.com/a/7868998/693538 ?
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
            Index index;
            Vertex vertex;
            vertex.color = color;
            for (int iFace = 0; iFace < VERTICES_PER_TRIANGLE; iFace++)
            {
                in >> index;
                if ( ! in )
                    throw MeshError(filename, "Failed to read face position index from mesh file "); // TODO: line number
                if (index < 1 || index > positions.size())
                    throw MeshError(filename, "Incorrect face position index in mesh file "); // TODO: line number
                vertex.pos = positions[index - 1];
                in.ignore(); // skip '/'

                in >> index;
                if ( ! in )
                    throw MeshError(filename, "Failed to read face texcoord index from mesh file "); // TODO: line number
                /* if (index < 1 || index > texcoords.size())
                    throw MeshError(filename, "Incorrect face texcoord position index in mesh file "); // TODO: line number
                vertex.texcoord = texcoords[index - 1]; */ // TODO: support texture coordinates
                in.ignore(); // skip '/'

                in >> index;
                if ( ! in )
                    throw MeshError(filename, "Failed to read face normal index from mesh file "); // TODO: line number
                if (index < 1 || index > normals.size())
                    throw MeshError(filename, "Incorrect face normal index in mesh file "); // TODO: line number
                vertex.set_normal(normals[index - 1]);

                indices.push_back(find_or_add_vertex(vertex));
            }
        }
    }
    loaded = true;
}

namespace
{
    // it is the same vertex if it has same pos and normal [and texcoord]
    bool same_vertex(const Vertex &a, const Vertex &b)
    {
        return a.pos == b.pos && a.normal == b.normal; // && a.texcoord = b.texcoord // TODO: support texture coordinates
    }
}

Index ObjMeshLoader::find_or_add_vertex(const Vertex &v)
{
    const Index vertices_count = vertices.size();
    for (int i = vertices_count - 1; i >= 0; --i)
    {
        if (same_vertex(vertices[i], v))
            return i;
    }
    // if not found - add new
    vertices.push_back(v);
    return vertices_count; // return old vertex count which is now last index
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
