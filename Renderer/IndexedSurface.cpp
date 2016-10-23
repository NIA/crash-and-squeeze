#include "IndexedSurface.h"
#include "main.h"

using CrashAndSqueeze::Core::ISurface;

class IndexedSurfaceTriangleIterator : public ISurface::TriangleIterator
{
private:
    const std::vector<ISurface::Triangle> &triangles;
    unsigned current_index;
public:
    IndexedSurfaceTriangleIterator(const std::vector<ISurface::Triangle> &triangles)
        : triangles(triangles)
    {
        current_index = 0;
    }
    // Check that we did not reach the end
    virtual bool has_value() const override
    {
        return current_index < triangles.size();
    }
    // Get current triangle
    virtual ISurface::Triangle operator*() const override
    {
        return triangles[current_index];
    }
    // Move to next triangle
    virtual void operator++() override
    {
        ++current_index;
    }

private:
    DISABLE_COPY(IndexedSurfaceTriangleIterator)
};


IndexedSurface::IndexedSurface(const Index * indices, unsigned indices_count, D3D_PRIMITIVE_TOPOLOGY prim_topology)
{
    if (prim_topology == D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP || prim_topology == D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST ||
        prim_topology == D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ || prim_topology == D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ)
    {
        Index current_index = 0;
        bool is_strip = (prim_topology == D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP || prim_topology == D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ);;
        bool swap_indices = false; // if is_strip, each even triangle should have indices swapped

        while (current_index + ISurface::VERTICES_PER_TRIANGLE - 1 < indices_count) {
            ISurface::Triangle res;
            for (int i = 0; i < ISurface::VERTICES_PER_TRIANGLE; ++i)
                res.indices[i] = indices[current_index + i];
            if (is_strip)
            {
                if (swap_indices)
                    std::swap(res.indices[0], res.indices[1]); // each even triangle should have indices swapped in order to preserve orientation
            }
            triangles.push_back(res);

            if (is_strip)
            {
                current_index += 1;
                swap_indices = !swap_indices;
            }
            else
            {
                current_index += ISurface::VERTICES_PER_TRIANGLE;
            }
        }
    }
}


ISurface::TriangleIterator * IndexedSurface::get_triangles()
{
    return new IndexedSurfaceTriangleIterator(triangles);
}


IndexedSurface::~IndexedSurface()
{
}
