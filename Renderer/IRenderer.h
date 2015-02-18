#pragma once
#include "main.h"

// A public interface of Renderer class (separated into an interface to remove circular dependency)
class IRenderer {
public:
    virtual ID3D11Device        * get_device()  const = 0;
    virtual ID3D11DeviceContext * get_context() const = 0;
};
