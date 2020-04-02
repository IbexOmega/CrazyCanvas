workspace "LambdaEngine"
    startproject "Sandbox"
    architecture "x64"
    warnings "extra"    

    -- Set output dir
    outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

    -- Platform
	platforms
	{
		"x64_SharedLib",
		"x64_StaticLib",
    }

    filter "platforms:x64_SharedLib"
        defines
        {
            "LAMBDA_SHARED_LIB",
        }
    filter {}

    -- Configurations
    configurations
    {
        "Debug",
        "Release",
        "Production",
    }

    filter "configurations:Debug"
        symbols "on"
        runtime "Debug"
        defines
        {
            "_DEBUG",
            "LAMBDA_DEBUG",
            "LAMBDA_DEVELOP",
        }
    filter "configurations:Release"
        symbols "on"
        runtime "Release"
        optimize "Full"
        defines
        {
            "NDEBUG",
            "LAMBDA_RELEASE",
            "LAMBDA_DEVELOP",
        }
    filter "configurations:Production"
        symbols "off"
        runtime "Release"
        optimize "Full"
        defines
        {
            "NDEBUG",
            "LAMBDA_PRODUCTION",
        }
    filter {}

    -- Compiler option
	filter "action:vs*"
        defines
        {
            "LAMBDA_VISUAL_STUDIO",
            "_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING",
            "_CRT_SECURE_NO_WARNINGS",
        }
    filter { "action:vs*", "configurations:Debug" }
		defines
		{
			"_CRTDBG_MAP_ALLOC",
        }
        
    filter "action:xcode4"
        defines
        {
            "LAMBDA_XCODE"
        }
    filter {}

    -- OS
	filter "system:macosx"
        defines
        {
            "LAMBDA_PLATFORM_MACOS",
        }

    filter "system:windows"
        defines
        {
            "LAMBDA_PLATFORM_WINDOWS",
        }

    filter "system:macosx or windows"
        defines
        {
            "LAMBDA_DESKTOP"
        }
    filter {}

	-- Dependencies
	group "Dependencies"
		-- tinyobjloader Project
		project "tinyobjloader"
			kind "StaticLib"
			language "C++"
			cppdialect "C++17"
			systemversion "latest"
			location "Dependencies/tinyobjloader"
			
			filter "configurations:Debug"
				symbols "on"
				runtime "Debug"
				optimize "Full"
			filter{}
			
			-- Targets
			targetdir ("Dependencies/tinyobjloader/bin/")
			objdir ("Dependencies/tinyobjloader/bin-int/")
					
			-- Files
			files 
			{
				"Dependencies/tinyobjloader/tiny_obj_loader.h",
				"Dependencies/tinyobjloader/tiny_obj_loader.cc",
			}
		project "*"
	group "*"

    -- Engine Project
    project "LambdaEngine"
        language "C++"
        cppdialect "C++17"
        systemversion "latest"
        location "LambdaEngine"
        
        -- Platform
		filter "platforms:x64_SharedLib"
            kind "SharedLib"
        filter "platforms:x64_StaticLib"
            kind "StaticLib"
        filter {}

        -- Targets
		targetdir 	("Build/bin/" .. outputdir .. "/%{prj.name}")
		objdir 		("Build/bin-int/" .. outputdir .. "/%{prj.name}")	

        -- Engine specific defines
		defines 
		{ 
			"LAMBDA_EXPORT",
		}
        
        -- Files to include
		files 
		{ 
			"%{prj.name}/**.h",
			"%{prj.name}/**.hpp",
			"%{prj.name}/**.inl",
			"%{prj.name}/**.c",
			"%{prj.name}/**.cpp",
			"%{prj.name}/**.hlsl",
        }
        removefiles
        {
            "%{prj.name}/Source/Launch/**",
        }
        
        -- Remove files not available for windows builds
		filter "system:windows"
            removefiles
            {
                "%{prj.name}/Include/Application/Mac/**",
                "%{prj.name}/Source/Application/Mac/**",

                "%{prj.name}/Include/Input/Mac/**",
                "%{prj.name}/Source/Input/Mac/**",

                "%{prj.name}/Include/Network/Mac/**",
                "%{prj.name}/Source/Network/Mac/**",
            }
        -- Remove files not available for macos builds
        filter "system:macosx"
            files
            {
                "%{prj.name}/**.m",
                "%{prj.name}/**.mm",
            }
            removefiles
            {
                "%{prj.name}/Include/Application/Win32/**",
                "%{prj.name}/Source/Application/Win32/**",

                "%{prj.name}/Include/Input/Win32/**",
                "%{prj.name}/Source/Input/Win32/**",

                "%{prj.name}/Include/Network/Win32/**",
                "%{prj.name}/Source/Network/Win32/**",
            }
        filter {}

        -- We do not want to compile HLSL files so exclude them from project
        excludes 
        {	
            "**.hlsl",
        }

        -- Includes
		includedirs
		{
			"%{prj.name}/Include",
        }
		
		sysincludedirs
		{
			"Dependencies/glm",
			"Dependencies/tinyobjloader",
		}
        
		links 
		{ 
			"tinyobjloader",
		}
		
		-- Win32
		filter { "system:windows" }
			links
			{
                "vulkan-1",
				"fmodL_vc.lib",
			}
			
			libdirs
			{
				"C:/VulkanSDK/1.2.131.2/Lib",
				"D:/VulkanSDK/1.2.131.2/Lib",
				"D:/Vulkan/1.2.131.2/Lib",
				"C:/FMOD Studio API Windows/api/core/lib/x64",
			}
			
			sysincludedirs
			{
				"C:/VulkanSDK/1.2.131.2/Include",
				"D:/VulkanSDK/1.2.131.2/Include",
				"D:/Vulkan/1.2.131.2/Include",
				"C:/FMOD Studio API Windows/api/core/inc",
			}
		-- Mac
		filter { "system:macosx" }
			libdirs
			{
				"/usr/local/lib",
			}
			
			sysincludedirs
			{
				"/usr/local/include",
			}
			
			links 
			{
                "vulkan.1",
				"vulkan.1.2.131",
                "Cocoa.framework",
                "MetalKit.framework",
			}
		filter {}

        -- Copy DLL into correct folder for windows builds
        filter { "system:windows", "platforms:x64_SharedLib" }
			postbuildcommands
			{
                ("{COPY} %{cfg.buildtarget.relpath} \"../Build/bin/" .. outputdir .. "/Sandbox/\""),
                ("{COPY} %{cfg.buildtarget.relpath} \"../Build/bin/" .. outputdir .. "/Client/\""),
                ("{COPY} %{cfg.buildtarget.relpath} \"../Build/bin/" .. outputdir .. "/Server/\""),
				
				("{COPY} \"C:/FMOD Studio API Windows/api/core/lib/x64/fmodL.dll\" \"../Build/bin/" .. outputdir .. "/Sandbox/\""),
                ("{COPY} \"C:/FMOD Studio API Windows/api/core/lib/x64/fmodL.dll\" \"../Build/bin/" .. outputdir .. "/Client/\""),
                ("{COPY} \"C:/FMOD Studio API Windows/api/core/lib/x64/fmodL.dll\" \"../Build/bin/" .. outputdir .. "/Server/\"")
			}
		filter {}
    project "*"

    -- Sandbox Project
    project "Sandbox"
        kind "WindowedApp"
        language "C++"
		cppdialect "C++17"
		systemversion "latest"
        location "Sandbox"
        
        -- Targets
		targetdir ("Build/bin/" .. outputdir .. "/%{prj.name}")
		objdir ("Build/bin-int/" .. outputdir .. "/%{prj.name}")
		
		--Includes
		includedirs
		{
            "LambdaEngine/Include",
            "%{prj.name}/Include",
        }
		
		sysincludedirs
		{
			"Dependencies/glm",
			"Dependencies/tinyobjloader",
		}
        
        -- Files
		files 
		{
            "LambdaEngine/Source/Launch/**",
			"%{prj.name}/**.hpp",
			"%{prj.name}/**.h",
			"%{prj.name}/**.inl",
			"%{prj.name}/**.cpp",
			"%{prj.name}/**.c",
			"%{prj.name}/**.hlsl",
		}
		-- We do not want to compile HLSL files
		excludes 
		{	
			"**.hlsl",
		}
		-- Linking
		links 
		{ 
			"LambdaEngine",
		}

    project "*"

    -- Client Project
    project "Client"
        kind "WindowedApp"
        language "C++"
		cppdialect "C++17"
		systemversion "latest"
        location "Client"
        
        -- Targets
		targetdir ("Build/bin/" .. outputdir .. "/%{prj.name}")
		objdir ("Build/bin-int/" .. outputdir .. "/%{prj.name}")
		
		--Includes
		includedirs
		{
            "LambdaEngine/Include",
            "%{prj.name}/Include",
        }
        
        -- Files
		files 
		{
            "LambdaEngine/Source/Launch/**",
			"%{prj.name}/**.hpp",
			"%{prj.name}/**.h",
			"%{prj.name}/**.inl",
			"%{prj.name}/**.cpp",
			"%{prj.name}/**.c",
			"%{prj.name}/**.hlsl",
		}
		-- We do not want to compile HLSL files
		excludes 
		{	
			"**.hlsl",
		}
		-- Linking
		links 
		{ 
			"LambdaEngine",
		}

    project "*"

    -- Server Project
    project "Server"
        kind "WindowedApp"
        language "C++"
		cppdialect "C++17"
		systemversion "latest"
        location "Server"
        
        -- Targets
		targetdir ("Build/bin/" .. outputdir .. "/%{prj.name}")
		objdir ("Build/bin-int/" .. outputdir .. "/%{prj.name}")
		
		--Includes
		includedirs
		{
            "LambdaEngine/Include",
            "%{prj.name}/Include",
        }
        
        -- Files
		files 
		{
            "LambdaEngine/Source/Launch/**",
			"%{prj.name}/**.hpp",
			"%{prj.name}/**.h",
			"%{prj.name}/**.inl",
			"%{prj.name}/**.cpp",
			"%{prj.name}/**.c",
			"%{prj.name}/**.hlsl",
		}
		-- We do not want to compile HLSL files
		excludes 
		{	
			"**.hlsl",
		}
		-- Linking
		links 
		{ 
			"LambdaEngine",
		}
    project "*"