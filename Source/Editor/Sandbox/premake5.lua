project "Sandbox"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++latest"
    staticruntime "off"
    debugdir "%{wks.location}"
    targetdir (binOutputDir)
    objdir (IntermediatesOutputDir)

    Link.Runtime.Core()
    Link.Plugin.ImGui()

    prebuildcommands {
        RunHeaderTool(
            "%{prj.location}"                      -- [Project Source Directory]
            .. " %{prj.location}/Private/Meta.cpp" -- [Meta Output file]  
            .. " Sandbox"                          -- [Namespace] 
            .. " -I%{IncludeDir.Core}"             -- [Include]
            .. " -I%{IncludeDir.glm}"
            .. " -I%{IncludeDir.nvrhi}"
            .. " -I%{IncludeDir.tracy}"
            .. " -I%{IncludeDir.taskflow}"
            .. " -I%{IncludeDir.magic_enum}"
        ) 
    }

    files {

        "Private/**.cpp",
        "Private/**.h",
        "*.lua",
    }

    filter "configurations:Dist"
        kind "WindowedApp"

        postbuildcommands {

            "{COPY} %{HE}/ThirdParty/Premake/%{cfg.system}/premake5.exe " .. binOutputDir,
            "{COPYDIR} %{wks.location}/Resources " .. binOutputDir .. "/Resources",

            "{COPY} C:/Windows/System32/vcruntime140_1.dll " .. binOutputDir,
            "{COPY} C:/Windows/System32/vcruntime140.dll " .. binOutputDir,
            "{COPY} C:/Windows/System32/msvcp140_atomic_wait.dll " .. binOutputDir,
            "{COPY} C:/Windows/System32/msvcp140.dll " .. binOutputDir,

            'robocopy "%{wks.location}/Plugins" "%{binOutputDir}/Plugins" *.dll *.hplugin /S || exit /b 0',
        }
    filter {}
