#pragma once

#include "Core/Core.h"

MetaHeader(Sandbox)

namespace Sandbox {

    struct TYPE() Entity
    {
        PROPERTY() Math::float3 position;
        PROPERTY() float speed;
        PROPERTY() bool enabled;
    };

    struct TYPE() Camera
    {
        PROPERTY() float fov;
        PROPERTY() bool isMain;
    };
}

