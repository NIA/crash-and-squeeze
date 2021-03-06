#pragma once
#include "main.h"

class Camera
{
private:
    float3 eye_spheric;  // a point, spheric coordinates
    float3 at;           // a point, Cartesian coordinates
    float3 up;           // a vector, Cartesian coordinates

    float4x4 mx;

    void check_coord_bounds();
    
public:
    void set_position(float rho, float theta, float phi, bool update_mx = true);
    void set_up_direction(float x, float y, float z, bool update_mx = true);
    void set_at_position(float x, float y, float z, bool update_mx = true);

    void set(float pos_rho, float pos_theta, float pos_phi, float at_x, float at_y, float at_z);

    void change_rho(float addition);
    void change_theta(float addition);
    void change_phi(float addition);
    
    // ... and wrapers for change_* functions:

    void move_nearer();
    void move_farther();
    void move_up();
    void move_down();
    void move_clockwise();
    void move_counterclockwise();

    Camera();
    Camera(float rho, float theta, float phi) { set(rho,theta,phi,0,0,0); }
    Camera(float pos_rho, float pos_theta, float pos_phi, float at_x, float at_y, float at_z) { set(pos_rho,pos_theta,pos_phi,at_x,at_y,at_z); }
    void update_matrices();

    const float4x4 & get_matrix() const;
    float3 get_eye() const;
};
