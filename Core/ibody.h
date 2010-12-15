#pragma once
#include "Math/vector.h"

namespace CrashAndSqueeze
{
    namespace Core
    {
        // An interface for kinematic abstraction of body, having position, linear and angular velocities
        class IBody
        {
        public:
            virtual const Math::Vector & get_position() const = 0;
            virtual const Math::Vector & get_linear_velocity() const = 0;
            virtual const Math::Vector & get_angular_velocity() const = 0;
        };
    }
}
