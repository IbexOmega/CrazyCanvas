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
                "%{prj.name}/Include/Platform/macOS/**",
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
                "%{prj.name}/Include/Platform/Win32/**",
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
        
        -- Links
        links 
        {
            "Cocoa.framework"
        }

        -- Copy DLL into correct folder for windows builds
        filter { "system:windows", "platforms:x64_SharedLib" }
			postbuildcommands
			{
				("{COPY} %{cfg.buildtarget.relpath} \"../Build/bin/" .. outputdir .. "/Sandbox/\"")
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