-------------------------------------------------------------------------------------
-- global variables
-------------------------------------------------------------------------------------
HE = path.getabsolute(".")

if RHI == nil then RHI = {} end
if RHI.enableD3D11  == nil then RHI.enableD3D11 = os.host() == "windows" end
if RHI.enableD3D12  == nil then RHI.enableD3D12 = os.host() == "windows" end
if RHI.enableVulkan == nil then RHI.enableVulkan = true end

outputdir = "%{CapitalizeFirstLetter(cfg.system)}-%{cfg.architecture}/%{cfg.buildcfg}"
binOutputDir = "%{wks.location}/Build/%{outputdir}/Bin"
libOutputDir = "%{wks.location}/Build/%{outputdir}/Lib/%{prj.name}"
IntermediatesOutputDir = "%{wks.location}/Build/Intermediates/%{outputdir}/%{prj.name}"

IncludeDir = {}
IncludeDir["glfw"] = "%{HE}/ThirdParty/glfw/include"
IncludeDir["spdlog"] = "%{HE}/ThirdParty/spdlog/include"
IncludeDir["glm"] = "%{HE}/ThirdParty/glm"
IncludeDir["stb_image"] = "%{HE}/ThirdParty/stb_image"
IncludeDir["nvrhi"] = "%{HE}/ThirdParty/nvrhi/include"
IncludeDir["NFDE"] = "%{HE}/ThirdParty/nativefiledialog-extended/src/include"
IncludeDir["ShaderMake"] = "%{HE}/ThirdParty/ShaderMake/include"
IncludeDir["tracy"] = "%{HE}/ThirdParty/tracy/public"
IncludeDir["taskflow"] = "%{HE}/ThirdParty/taskflow"
IncludeDir["Vulkan_Headers"] = "%{HE}/ThirdParty/Vulkan-Headers/Include"
IncludeDir["simdjson"] = "%{HE}/ThirdParty/simdjson"
IncludeDir["miniz"] = "%{HE}/ThirdParty/miniz"
IncludeDir["magic_enum"] = "%{HE}/ThirdParty/magic_enum/include"

IncludeDir["Core"] = "%{HE}/Source/Runtime/Core/Public"

LibDir = {}
LibDir["HE"] = "%{HE}/Build/%{outputdir}/Bin"

Tools = {}
Tools["ShaderMake"] = "%{binOutputDir}/ShaderMake"

Link = {}
Link.Runtime = {}
Link.Plugin = {}

-------------------------------------------------------------------------------------
-- helper functions
-------------------------------------------------------------------------------------
function CapitalizeFirstLetter(str)
    return str:sub(1,1):upper() .. str:sub(2)
end

function Download(url, output)
    if os.host() == "windows" then
        os.execute("powershell -Command \"Invoke-WebRequest -Uri '" .. url .. "' -OutFile '" .. output .. "'\"")
    else
        os.execute("curl -L -o " .. output .. " " .. url)
    end
end

function FindFxc()
    local sdkRoot = "C:/Program Files (x86)/Windows Kits/10/bin/"
    local versions = os.matchdirs(sdkRoot .. "*")
    table.sort(versions, function(a, b) return a > b end)
    for _, dir in ipairs(versions) do 
        local path = dir .. "/x64/fxc.exe" 
        if os.isfile(path) then  return path end
    end
    return nil
end

function BuildShaders(apiList ,src, cache, flags, includeDirs)
    includeDirs = includeDirs or {}
    local exeExt = (os.host() == "windows") and ".exe" or ""
    local sm    = "%{Tools.ShaderMake}" .. exeExt
    local cfg   = path.join(src, "shaders.cfg")
    local dxc   = "%{HE}/ThirdParty/Lib/dxc/bin/x64/dxc" .. exeExt
    local fxc   = FindFxc()
    local inc   = table.concat(includeDirs, "\" -I \"", 1, -1, "-I \"")
    local sep   = (os.host() == "windows") and " && " or " ; "
    apiList = apiList or { D3D11 = false, D3D12 = false,  VULKAN = false, METAL = false }

    local conf = {}
    if apiList.VULKAN then table.insert(conf, { out = "spirv", args = "--platform SPIRV --vulkanVersion 1.2 --tRegShift 0 --sRegShift 128 --bRegShift 256 --uRegShift 384 -D SPIRV --compiler \"" .. dxc .. "\"" }) end
    if os.host() == "windows" then
        if apiList.D3D11 then table.insert(conf, { out = "dxbc", args = "--platform DXBC --shaderModel 6_6 --compiler \"" .. fxc .. "\"" }) end
        if apiList.D3D12 then table.insert(conf, { out = "dxil", args = "--platform DXIL --shaderModel 6_6 --compiler \"" .. dxc .. "\"" }) end
    end

    local cmds = {}
    for _, c in ipairs(conf) do
        table.insert(cmds, string.format(
            "\"%s\" --config \"%s\" %s --outputExt .bin --colorize --verbose --out \"%s\" %s %s",
            sm, cfg, inc, path.join(cache, c.out), flags, c.args))
    end

    return table.concat(cmds, sep)
end

function SetupShaders(apiList ,src, cache, flags, includeDirs)
    filter { "files:**.hlsl" }
        buildcommands {
            BuildShaders(apiList ,src, cache, flags, includeDirs)
        }
        buildoutputs { "%{wks.location}/dumy" }
    filter {}
end

function AddModules(dir)
    local modulesDir = path.getabsolute(dir)
    local modules = os.matchdirs(modulesDir .. "/*")

    for _, module in ipairs(modules) do
        local premakeFile = path.join(module, "premake5.lua")
        if os.isfile(premakeFile) then
            include(module)
        end
    end
end

function CloneLibs(platformRepoURLs, targetDir, branchOrTag)
    local repoURL = platformRepoURLs[os.host()]
    if not repoURL then
        error("No repository URL defined for platform: " .. os.host())
    end

    targetDir = targetDir or "ThirdParty/Lib"

    if not os.isdir(targetDir) then
        local cloneCmd = string.format(
            'git clone --depth 1 %s"%s" "%s"',
            branchOrTag and ('--branch ' .. branchOrTag .. ' ') or '',
            repoURL,
            targetDir
        )

        print("Cloning " .. repoURL)
        os.execute(cloneCmd)
    end

    for _, z in ipairs(os.matchfiles(targetDir .. "/*.zip")) do
        print("Extracting " .. z .. " to " .. targetDir)
        zip.extract(z, targetDir)
        os.remove(z)
    end
end

function SetCoreFilters()
    filter "system:windows"
        systemversion "latest"

        if RHI.enableD3D11 then
            defines { "NVRHI_HAS_D3D11" }
        end

        if RHI.enableD3D12 then
            defines { "NVRHI_HAS_D3D12" }
        end

        if RHI.enableVulkan then
            defines { "NVRHI_HAS_VULKAN" }
        end

    filter "system:linux"
        systemversion "latest"

        if RHI.enableVulkan then
            defines { "NVRHI_HAS_VULKAN" }
        end

    filter "configurations:Debug"
        defines "CORE_DEBUG"
        runtime "Debug"
        symbols "On"

    filter "configurations:Release"
        defines "CORE_RELEASE"
        runtime "Release"
        optimize "On"

    filter "configurations:Profile"
        includedirs { "%{IncludeDir.tracy}" }
        defines { "CORE_PROFILE", "TRACY_IMPORTS" }
        runtime "Release"
        optimize "On"

    filter "configurations:Dist"
        defines "CORE_DIST"
        runtime "Release"
        optimize "Speed"
        symbols "Off"
    filter {}
end

function Link.Runtime.Core(extra)

    SetCoreFilters()

    libdirs {

        "%{LibDir.HE}",
    }

    links {

        "Core",
        "nvrhi",
    }

    defines {

        "NVRHI_SHARED_LIBRARY_INCLUDE",
        "SIMDJSON_USING_WINDOWS_DYNAMIC_LIBRARY",
        "_CRT_SECURE_NO_WARNINGS",
        "_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING",
    }

    includedirs {

        "%{IncludeDir.Core}",
        "%{IncludeDir.nvrhi}",
        "%{IncludeDir.glm}",
        "%{IncludeDir.taskflow}",
        "%{IncludeDir.simdjson}",
        "%{IncludeDir.tracy}",
        "%{IncludeDir.magic_enum}",
        "%{IncludeDir.Vulkan_Headers}",
    }

    dependson { "ShaderMake", "Meta" }
end

