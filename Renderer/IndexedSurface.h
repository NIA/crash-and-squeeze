#pragma once
#include <vector>
#include "Core/isurface.h"
#include "Buffer.h"

class IndexedSurface : public CrashAndSqueeze::Core::ISurface
{
private:
    std::vector<CrashAndSqueeze::Core::ISurface::Triangle> triangles;
public:
    IndexedSurface(const Index * indices, unsigned indices_count, D3D_PRIMITIVE_TOPOLOGY prim_topology);

    // Returns new TriangleIterator, initialized to the beginning
    virtual TriangleIterator * get_triangles() override;

    // Deallocates the iterator when it is no longer needed
    virtual void destroy_iterator(TriangleIterator * iterator) override { delete iterator; }

    virtual ~IndexedSurface();
};

