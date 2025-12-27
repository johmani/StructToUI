#pragma once

#include "Core/Core.h"

MetaHeader(Sandbox)


namespace Sandbox {

    struct TYPE() Entity
    {
        PROPERTY(Meta::Color(0.2f, 0.3f, 0.7f, 1.0f))
        Math::float3 position;
        
        PROPERTY(Meta::UI::Slider)
        PROPERTY(Meta::Range(0.0f, 5.0f))
        float speed;
        
        PROPERTY(Meta::UI::Text)
        bool enabled;
    };

    struct TYPE() Camera
    {
        PROPERTY()
        float fov;

        PROPERTY()
        bool isMain;
    };
}

