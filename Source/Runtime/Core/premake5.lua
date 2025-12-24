project "Core"
    kind "SharedLib"
    language "C++"
    cppdialect "C++latest"
    staticruntime "off"
    targetdir (binOutputDir)
    objdir (IntermediatesOutputDir)

    files {

        "Private/Core/**.cpp",
        "Private/Core/**.h",

        "Public/**.h",

        "*.lua",

        "%{IncludeDir.simdjson}/**.cpp",
    }

    defines {

        --"CORE_FORCE_DISCRETE_GPU",
        "CORE_AS_SHAREDLIB",
        "CORE_BUILD",
        "GLFW_INCLUDE_NONE",
        "GLFW_DLL",
        "_CRT_SECURE_NO_WARNINGS",
        "NVRHI_SHARED_LIBRARY_INCLUDE",
        "SIMDJSON_BUILDING_WINDOWS_DYNAMIC_LIBRARY",
        "_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING",
    }

    includedirs {

        "Public",
        "%{IncludeDir.spdlog}",
        "%{IncludeDir.glfw}",
        "%{IncludeDir.glm}",
        "%{IncludeDir.NFDE}",
        "%{IncludeDir.taskflow}",
        "%{IncludeDir.stb_image}",
        "%{IncludeDir.simdjson}",
        "%{IncludeDir.nvrhi}",
        "%{IncludeDir.Vulkan_Headers}",
        "%{IncludeDir.ShaderMake}",
        "%{IncludeDir.miniz}",
        "%{IncludeDir.magic_enum}",
    }

    links {

        "glfw",
        "nvrhi",
        "NFDE",
        "ShaderMakeBlob",
    }

    filter "system:windows"
        systemversion "latest"
        files { "Private/Platform/WindowsPlatform.cpp" }

        links {

            "DXGI.lib",
            "dxguid.lib",
        }

        if RHI.enableD3D11 then
            links { "D3D11.lib" }
            defines { "NVRHI_HAS_D3D11" }
        end

        if RHI.enableD3D12 then
            links { "D3D12.lib" }
            defines { "NVRHI_HAS_D3D12" }
        end

        if RHI.enableVulkan then
            defines { "NVRHI_HAS_VULKAN" }
            files { "Private/Platform/VulkanDeviceManager.cpp" }
        end

    filter "system:linux"
        systemversion "latest"
        files { "Private/LinuxPlatform.cpp" }

        if RHI.enableVulkan then
            defines { "NVRHI_HAS_VULKAN" }
            files { "Private/Platform/VulkanDeviceManager.cpp" }
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
        runtime "Release"
        optimize "On"
        files { "%{IncludeDir.tracy}/TracyClient.cpp" }
        includedirs { "%{IncludeDir.tracy}" }
        defines { "CORE_PROFILE", "TRACY_EXPORTS" , "TRACY_ENABLE" }

    filter "configurations:Dist"
        defines "CORE_DIST"
        runtime "Release"
        optimize "Speed"
        symbols "Off"
