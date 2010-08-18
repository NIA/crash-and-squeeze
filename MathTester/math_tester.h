#pragma once
#include <gtest/gtest.h>
#include "vector.h"

using namespace ::CrashAndSqueeze::Math;

inline std::ostream &operator<<(std::ostream &stream, const Vector &vector)
{
    return stream << "(" << vector[0] << ", " << vector[1] << ", " << vector[2] << ")";
}
