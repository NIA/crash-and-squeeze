#include "Math/vector.h"

namespace CrashAndSqueeze
{
    namespace Math
    {
        const Vector Vector::ZERO(0, 0, 0);

        // returns projection to `direction'. Second parameter is optional, and when
        // it is not null, the normal component is returned through it.
        Real Vector::project_to(const Vector &direction, /*out*/ Vector *normal_component) const
        {
            Vector axis = direction.normalized();
            Real projection = (*this)*axis;

            if( 0 != normal_component )
                *normal_component = *this - projection*axis;
            
            return projection;
        }
    }
}
