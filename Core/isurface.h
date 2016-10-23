#pragma once
namespace CrashAndSqueeze
{
    namespace Core
    {

        // An interface of 3D object mesh surface, made of triangles
        // Knowing surface is not required for computing deformation, but it is needed
        // for generating normals from scratch (is needed for correct updating normals if CAS_QUADRATIC_EXTENSIONS_ENABLED)
        class ISurface {
        public:
            // Triangle is a triple of indices referring to vertex array
            static const int VERTICES_PER_TRIANGLE = 3;
            struct Triangle
            {
                unsigned indices[VERTICES_PER_TRIANGLE];
                unsigned & operator[](unsigned i) { return indices[i]; }
            };
            // An iterator to loop through triangles of model
            class TriangleIterator
            {
            public:
                // Check that we did not reach the end
                virtual bool has_value() const = 0;
                // Get current triangle
                virtual Triangle operator*() const = 0;
                // Move to next triangle
                virtual void operator++() = 0;
                // Release resources that are held by this iterator
                virtual ~TriangleIterator() {};
            };

            // Returns new TriangleIterator, initialized to the beginning
            virtual TriangleIterator * get_triangles() = 0;

            // Deallocates the iterator when it is no longer needed
            virtual void destroy_iterator(TriangleIterator * iterator) = 0;
        };
    }
}