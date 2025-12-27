////////////////////////////////////////////
// AUTO GENERATED
////////////////////////////////////////////
#include "Sandbox.h"


static Meta::Attribute s_Attributes[] = { 
	Meta::Attribute(), 
    Meta::Color(0.2f, 0.3f, 0.7f, 1.0f),
    Meta::UI::Slider,
    Meta::Range(0.0f, 5.0f),
    Meta::UI::Text,
 
};

static Meta::Field s_Fields[] = { 

    { "Math::float3", "position", 12, 0, 1, 1, s_Attributes },
    { "float", "speed", 4, 12, 2, 2, s_Attributes },
    { "bool", "enabled", 1, 16, 4, 1, s_Attributes },
    { "float", "fov", 4, 0, 0, 1, s_Attributes },
    { "bool", "isMain", 1, 4, 0, 1, s_Attributes } 
};
	
static Meta::Type s_Types[] = {

    { "Sandbox::Entity", "Entity", 20, 0, 3, s_Fields },
    { "Sandbox::Camera", "Camera", 8, 3, 2, s_Fields }
};
		
static Meta::TypeRegistry s_Registry{ 
	s_Types     , std::size(s_Types), 
	s_Attributes, std::size(s_Attributes),
	s_Fields    , std::size(s_Fields) 
};

const Meta::TypeRegistry& Meta::Sandbox::Registry() 
{ 
	return s_Registry; 
}
