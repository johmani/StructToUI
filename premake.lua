-------------------------------------------------------------------------------------
-- Args
-------------------------------------------------------------------------------------
HE = "%{wks.location}"
projectLocation = "%{wks.location}/Build/IDE"

RHI = {}
RHI.enableD3D11  = true
RHI.enableD3D12  = true
RHI.enableVulkan = true

App = {}
App.Editor = true

include "build.lua"

-------------------------------------------------------------------------------------
-- Setup
-------------------------------------------------------------------------------------
CloneLibs(
    {
        windows = "https://github.com/johmani/HydraEngineLibs_Windows_x64",
    },
    "ThirdParty/Lib"
)

-------------------------------------------------------------------------------------
-- workspace
-------------------------------------------------------------------------------------
workspace "StructToUI"
    architecture "x86_64"
    configurations { "Debug", "Release", "Profile", "Dist" }
    startproject "Sandbox"
    flags {

      "MultiProcessorCompile",
    }

    group("Plugins")
    AddModules("Plugins")

    group("Tools")
    AddModules("Tools")

    group("ThirdParty")
    AddModules("ThirdParty")

    group("Runtime")
    AddModules("Source/Runtime")

    group("Editor")
    AddModules("Source/Editor")
