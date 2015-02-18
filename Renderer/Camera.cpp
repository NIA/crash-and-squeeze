#include "Camera.h"

using namespace DirectX;

namespace
{
    const float NEAR_CLIP = 1.2f;
    const float FAR_CLIP = 1e10f;
    const float COEFF_A = FAR_CLIP / (FAR_CLIP - NEAR_CLIP);
    const float COEFF_B = - COEFF_A * NEAR_CLIP;

    const float4x4 _PROJ_MX (  NEAR_CLIP,         0,       0,       0,
                                       0, NEAR_CLIP,       0,       0,
                                       0,         0, COEFF_A, COEFF_B,
                                       0,         0,       1,       0 );
    const XMMATRIX PROJ_MX = XMLoadFloat4x4(&_PROJ_MX);

    const float RHO_STEP = 0.1f;
    const float THETA_STEP = XM_PI/200.0f;
    const float PHI_STEP = XM_PI/100.0f;

    const float RHO_MIN = 0.5f + NEAR_CLIP; // // Suppose models are inside the cube 1x1x1 => in sphere with radius sqrt(3) (approx. 1,74).
    const float THETA_MIN = 0.01f;

    const float RHO_MAX = FAR_CLIP;
    const float THETA_MAX = XM_PI - 0.01f; // A little subtraction - to prevent looking parallel to the 'up' vector

    const float3 DEFAULT_POSITION(1.5f*RHO_MIN, XM_PI/3.0f, XM_PI/3.0f);
}

Camera::Camera()
{
    set(DEFAULT_POSITION.x, DEFAULT_POSITION.y, DEFAULT_POSITION.z, 0, 0, 0);
}

void Camera::set(float pos_rho, float pos_theta, float pos_phi, float at_x, float at_y, float at_z)
{
    set_position(pos_rho,pos_theta,pos_phi, false);
    set_at_position(at_x, at_y, at_z, false);
    set_up_direction(0,0,1);
}

void Camera::set_position(float rho, float theta, float phi, bool update_mx)
{
    eye_spheric = float3(rho, theta, phi);
    if( update_mx )
        update_matrices();
}

void Camera::set_at_position(float x, float y, float z, bool update_mx)
{
    at = float3(x, y, z);
    if( update_mx )
        update_matrices();
}

void Camera::set_up_direction(float x, float y, float z, bool update_mx)
{
    up = float3(x, y, z);
    if( update_mx )
        update_matrices();
}

void Camera::check_coord_bounds()
{
    if (eye_spheric.x < RHO_MIN)
    {
        eye_spheric.x = RHO_MIN;
    }
    if (eye_spheric.x > RHO_MAX)
    {
        eye_spheric.x = RHO_MAX;
    }
    if (eye_spheric.y < THETA_MIN)
    {
        eye_spheric.y = THETA_MIN;
    }
    if (eye_spheric.y > THETA_MAX)
    {
        eye_spheric.y = THETA_MAX;
    }
}

void Camera::change_rho(float addition)
{
    eye_spheric.x += addition;
    check_coord_bounds();
    update_matrices();
}

void Camera::change_theta(float addition)
{
    eye_spheric.y += addition;
    check_coord_bounds();
    update_matrices();
}

void Camera::change_phi(float addition)
{
    eye_spheric.z += addition;
    check_coord_bounds();
    update_matrices();
}

void Camera::move_nearer()
{
    change_rho( -RHO_STEP );
}
void Camera::move_farther()
{
    change_rho( RHO_STEP );
}
void Camera::move_up()
{
    change_theta( -THETA_STEP );
}
void Camera::move_down()
{
    change_theta( THETA_STEP );
}
void Camera::move_clockwise()
{
    change_phi( PHI_STEP );
}
void Camera::move_counterclockwise()
{
    change_phi( -PHI_STEP );
}

static float3 spheric_to_cartesian( const float3 & spheric )
{
    float rho = spheric.x;
    float theta = spheric.y;
    float phi = spheric.z;

    return float3( rho*sin(theta)*cos(phi), rho*sin(theta)*sin(phi), rho*cos(theta) );
}

void Camera::update_matrices()
{
    const float3 _eye = get_eye();
    const XMVECTOR eye = XMLoadFloat3(&_eye);
    XMVECTOR axis_x, axis_y, axis_z;
    axis_z = XMVector3Normalize( XMLoadFloat3(&at) - eye );
    axis_x = XMVector3Normalize( XMVector3Cross(XMLoadFloat3(&up), axis_z) );
    axis_y = XMVector3Normalize( XMVector3Cross(axis_z,            axis_x) );

    const float3 d( - XMVectorGetX(XMVector3Dot( eye, axis_x )),
                    - XMVectorGetX(XMVector3Dot( eye, axis_y )),
                    - XMVectorGetX(XMVector3Dot( eye, axis_z )) );

    const float4x4 view_mx( XMVectorGetX(axis_x), XMVectorGetY(axis_x), XMVectorGetZ(axis_x), d.x,
                            XMVectorGetX(axis_y), XMVectorGetY(axis_y), XMVectorGetZ(axis_y), d.y,
                            XMVectorGetX(axis_z), XMVectorGetY(axis_z), XMVectorGetZ(axis_z), d.z,
                                               0,                     0,                   0,   1 );
    XMStoreFloat4x4(&mx, PROJ_MX * XMLoadFloat4x4(&view_mx));
}

const float4x4 & Camera::get_matrix() const
{
    return mx;
}

float3 Camera::get_eye() const
{
    return spheric_to_cartesian( eye_spheric );
}
