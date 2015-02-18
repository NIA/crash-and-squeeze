#pragma once

#include "main.h"

// Transposes matrix so that it is column-major (as it is expected for shaders).
inline DirectX::XMMATRIX fix(const DirectX::XMMATRIX & m) { return DirectX::XMMatrixTranspose(m); }

inline DirectX::XMMATRIX shift_matrix(const float3 & shift)
{
    return fix(DirectX::XMMatrixTranslation(shift.x, shift.y, shift.z));
}

inline DirectX::XMMATRIX scale_matrix(float scale)
{
    return fix(DirectX::XMMatrixScaling(scale, scale, scale));
}

inline DirectX::XMMATRIX rotate_x_matrix(float angle)
{
    return fix(DirectX::XMMatrixRotationX(angle));
}

inline DirectX::XMMATRIX rotate_y_matrix(float angle)
{
    return fix(DirectX::XMMatrixRotationY(angle));
}

inline DirectX::XMMATRIX rotate_z_matrix(float angle)
{
    return fix(DirectX::XMMatrixRotationZ(angle));
}

inline DirectX::XMMATRIX rotate_and_shift_matrix(const float3 & angles, const float3 & shift)
{
    return shift_matrix(shift)*rotate_z_matrix(angles.z)*rotate_y_matrix(angles.y)*rotate_x_matrix(angles.x);
}
