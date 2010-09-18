#pragma once
#include "Math/floating_point.h"

namespace CrashAndSqueeze
{
    class Math::Vector;

    namespace Core
    {
        typedef void (* SpaceDeformationCallback)(const Math::Vector &pos, const Math::Vector &size, Math::Real value);
    }
}
