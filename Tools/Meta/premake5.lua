
function RunHeaderTool(args)
    local metaHeaderToolPath = path.join(binOutputDir, "Meta");
    return string.format("%s %s", metaHeaderToolPath, args)
end

project "Meta"
    kind "ConsoleApp"
    language "C++"
    cppdialect  "C++latest"
    staticruntime "Off"
    targetdir (binOutputDir)
    objdir (IntermediatesOutputDir)

    prebuildcommands {

        "{COPY} %{wks.location}/ThirdParty/Lib/libclang/bin/. %{binOutputDir}",
    }

    files
    {
        "Source/Meta/**.h",
        "Source/Meta/**.cpp",
        "*.lua",
    }

    defines
    {
        --"DISABLE_LOG",
    }

    includedirs
    {
        "%{wks.location}/ThirdParty/Lib/libclang/include",
    }

    libdirs 
    {
        "%{wks.location}/ThirdParty/Lib/libclang/lib",
    }

    links
    {
        "libclang",
    }
