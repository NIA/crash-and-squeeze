#pragma once
#include "Logging/logger.h"
#include "Collections/array.h"

#define CAS_VERSION "0.9.0-SNAPSHOT"

// ** Options (preprocessor switches) **

// Set to 0 to disable quadratic extensions
#define CAS_QUADRATIC_EXTENSIONS_ENABLED 1
// Set to 0 to disable quadratic plasticity (without disabling quadratic extensions)
#define CAS_QUADRATIC_PLASTICITY_ENABLED 1
// Set to 0 for more conservative graphical vertices update:
// use equilibrium transformation (R*Sp) instead of total deformation (A)
// to hide some oscillations (looks more realistic for metal-like objects)
#define CAS_GRAPHICAL_TRANSFORM_TOTAL    1
// Set to 0 to enable protection from flipping to inverse shape
// by checking that det A >= 0
#define CAS_ALLOW_SHAPE_FLIP             0
// Set to 0 to disable warnings appearing when the shape indices array
// passed to a ShapeDeformationReaction or a RegionReaction is empty.
// But anyway, it is useful to leave these warnings enabled for safety from bugs
#define CAS_WARN_EMPTY_REACTION_SHAPE    1

namespace CrashAndSqueeze
{
    namespace Core
    {
        // floating type for storing graphical vertex components in vertex buffer
        typedef float VertexFloat;
        // floating type for defining mass of physical vertex
        typedef double MassFloat;
        // integer type for storing cluster index of graphical vertex in vertex buffer
        typedef unsigned char ClusterIndex;

        typedef Collections::Array<int> IndexArray;

        template<class T>
        inline void ignore_unreferenced(T parameter) { parameter; }

        inline const void *add_to_pointer(const void *pointer, int offset)
        {
            return reinterpret_cast<const void*>( reinterpret_cast<const char*>(pointer) + offset );
        }
        inline void *add_to_pointer(void *pointer, int offset)
        {
            return reinterpret_cast<void*>( reinterpret_cast<char*>(pointer) + offset );
        }
    }
}
