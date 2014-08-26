#include "Core/regions.h"

namespace CrashAndSqueeze
{
    using Logging::Logger;
    using Math::Real;
    using Math::less_or_equal;
    using Math::greater_or_equal;
    using Math::Vector;
    using Math::VECTOR_SIZE;

    namespace Core
    {
        namespace
        {
            inline Real correct_radius(Real & value)
            {
                if( value < 0 )
                {
                    Logger::warning("setting radius for region: given value is less than 0, corrected to 0", __FILE__, __LINE__);
                    return 0;
                }
                return value;
            }
        }

        // -- EmptyRegion --

        bool EmptyRegion::contains(const Vector &point) const
        {
            ignore_unreferenced(point);
            return false;
        }

        void EmptyRegion::move(const Vector &vector)
        {
            ignore_unreferenced(vector);
        }

        Vector EmptyRegion::get_center() const
        {
            return Vector::ZERO;
        }

        // -- SphericalRegion --

        SphericalRegion::SphericalRegion(Vector center, Real radius)
        {
            set_center(center);
            set_radius(radius);
        }

        void SphericalRegion::set_radius(Real value)
        {
            radius = correct_radius(value);
        }

        bool SphericalRegion::contains(const Vector &point) const
        {
            return less_or_equal( distance(point, center), radius );
        }

        void SphericalRegion::move(const Vector &vector)
        {
            center += vector;
        }

        Vector SphericalRegion::get_center() const
        {
            return center;
        }

        // -- CylindricalRegion --

        CylindricalRegion::CylindricalRegion(const Vector & top_center,
                                             const Vector & bottom_center,
                                             Math::Real radius)
        {
            set_radius(radius);
            set_axis(top_center, bottom_center);
        }

        bool CylindricalRegion::set_axis(const Vector & top_center, const Vector & bottom_center)
        {
            if( top_center == bottom_center )
            {
                Logger::error("in CylindricalRegion::set_axis: top coincides with bottom", __FILE__, __LINE__);
                return false;
            }
            else
            {
                this->top_center = top_center;
                this->bottom_center = bottom_center;
                return true;
            }
        }

        void CylindricalRegion::set_radius(Real value)
        {
            radius = correct_radius(value);
        }

        bool CylindricalRegion::contains(const Vector &point) const
        {
            Vector normal_component;
            // axis is the vector from bottom_center to top_center
            Real projection = (point - bottom_center).project_to(get_axis(), &normal_component);
            return greater_or_equal( projection, 0 )
                   && less_or_equal( projection, get_axis().norm() )
                   && less_or_equal( normal_component.norm(), radius );
        }

        void CylindricalRegion::move(const Vector &vector)
        {
            set_axis(top_center + vector, bottom_center + vector);
        }

        Vector CylindricalRegion::get_center() const
        {
            return (top_center + bottom_center)/2;
        }

        // -- BoxRegion --

        BoxRegion::BoxRegion(const Vector & min_corner, const Vector & max_corner)
        {
            set_box(min_corner, max_corner);
        }
        
        void BoxRegion::set_box(const Vector & min_corner, const Vector & max_corner)
        {
            this->min_corner = min_corner;
            
            for(int i = 0; i < VECTOR_SIZE; ++i)
            {
                if( max_corner[i] < min_corner[i] )
                {
                    Logger::warning("in BoxRegion::set_box: some component of max_corner is less "
                                    "than that of min_corner; corrected the component of max_corner", __FILE__, __LINE__);
                    this->max_corner[i] = min_corner[i];
                }
                else
                {
                    this->max_corner[i] = max_corner[i];
                }
            }
        }

        bool BoxRegion::contains(const Vector &point) const
        {
            for(int i = 0; i < VECTOR_SIZE; ++i)
            {
                if( point[i] < min_corner[i] || point[i] > max_corner[i] )
                    return false;
            }
            return true;
        }

        void BoxRegion::move(const Vector &vector)
        {
            set_box(min_corner + vector, max_corner + vector);
        }

        Vector BoxRegion::get_center() const
        {
            return (min_corner + max_corner)/2;
        }

        Real BoxRegion::get_distance_to_border(const Math::Vector &point, int * side_index)
        {
            if (!contains(point))
                return 0;

            Real distances[VECTOR_SIZE*2];
            for (int i = 0; i < VECTOR_SIZE; ++i)
            {
                distances[2*i]   = abs(point[i] - min_corner[i]);
                distances[2*i+1] = abs(max_corner[i] - point[i]);
            }
            Real min_dist = distances[0];
            int min_dist_side = 0;
            for (int i = 1; i < 2*VECTOR_SIZE; ++i)
            {
                if (distances[i] < min_dist)
                {
                    min_dist = distances[i];
                    min_dist_side = i / 2;
                }
            }
            if (side_index != NULL)
                *side_index = min_dist_side;
            return min_dist;
        }

    }
}
