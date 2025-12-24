////////////////////////////////////////////
// AUTO GENERATED
////////////////////////////////////////////
#include "Sandbox.h"


static Meta::Field s_Fields[] = { 

    { "Math::float3", "position", 12, 0 },
    { "float", "speed", 4, 12 },
    { "bool", "enabled", 1, 16 },
    { "float", "fov", 4, 0 },
    { "bool", "isMain", 1, 4 } 
};
	
static Meta::Type s_Types[] = {

    { "Sandbox::Entity", "Entity", 20, 0, 3, s_Fields },
    { "Sandbox::Camera", "Camera", 8, 3, 2, s_Fields }
};
		
static Meta::TypeRegistry s_Registry{ 
	s_Types, std::size(s_Types), 
	s_Fields, std::size(s_Fields) 
};

const Meta::TypeRegistry& Meta::Sandbox::Registry() 
{ 
	return s_Registry; 
}
