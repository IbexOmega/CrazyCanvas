function array_to_string(array)
	local resultingString = ""
	for _, value in ipairs(array) do
		resultingString = resultingString .. value .. " "
	end

	return string.format("[ %s ]", resultingString)
end

function get_vk_sdk_path()
	local sdkPathVars = {"VK_SDK_PATH", "VULKAN_SDK"}
	for _, sdkPathVar in ipairs(sdkPathVars) do
		sdkPath = os.getenv(sdkPathVar)
		if sdkPath ~= nil then
			return sdkPath
		end
	end

	print(string.format('No environment variables for path to Vulkan SDK are set: %s', array_to_string(sdkPathVars)))
	return ""
end

VK_SDK_PATH = get_vk_sdk_path()

workspace "LambdaEngine"
	startproject "Sandbox"
	architecture "x64"
	warnings "extra"

	-- Set output dir
	outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}-%{cfg.platform}"

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
			"LAMBDA_CONFIG_DEBUG",
		}
	filter "configurations:Release"
		symbols "on"
		runtime "Release"
		optimize "Full"
		defines
		{
			"NDEBUG",
			"LAMBDA_CONFIG_RELEASE",
		}
	filter "configurations:Production"
		symbols "off"
		runtime "Release"
		optimize "Full"
		defines
		{
			"NDEBUG",
			"LAMBDA_CONFIG_PRODUCTION",
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
		include "Dependencies/WavLib"
		include "Dependencies/imgui"

		-- tinyobjloader Project
		project "tinyobjloader"
			kind "StaticLib"
			language "C++"
			cppdialect "C++17"
			systemversion "latest"
			location "Dependencies/projectfiles/tinyobjloader"

			filter "configurations:Debug or Release"
				symbols "on"
				runtime "Release"
				optimize "Full"
			filter{}

			filter "configurations:Production"
				symbols "off"
				runtime "Release"
				optimize "Full"
			filter{}

			-- Targets
			targetdir ("Dependencies/bin/tinyobjloader/" .. outputdir)
			objdir ("Dependencies/bin-int/tinyobjloader/" .. outputdir)

			-- Files
			files
			{
				"Dependencies/tinyobjloader/tiny_obj_loader.h",
				"Dependencies/tinyobjloader/tiny_obj_loader.cc",
			}

		-- imnodes Project
		project "imnodes"
			kind "StaticLib"
			language "C++"
			cppdialect "C++17"
			systemversion "latest"
			location "Dependencies/projectfiles/imnodes"

			filter "configurations:Debug or Release"
				symbols "on"
				runtime "Release"
				optimize "Full"
			filter{}

			filter "configurations:Production"
				symbols "off"
				runtime "Release"
				optimize "Full"
			filter{}

			sysincludedirs
			{
				"Dependencies/imgui",
			}

			-- Targets
			targetdir ("Dependencies/bin/imnodes/" .. outputdir)
			objdir ("Dependencies/bin-int/imnodes/" .. outputdir)

			-- Files
			files
			{
				"Dependencies/imnodes/imnodes.h",
				"Dependencies/imnodes/imnodes.cpp",
			}
	group ""

    -- Engine Project
    project "LambdaEngine"
        language "C++"
        cppdialect "C++17"
        systemversion "latest"
        location "LambdaEngine"
        
		-- Pre-Compiled Headers
		pchheader "PreCompiled.h"
		pchsource "%{prj.name}/PreCompiled.cpp"

		forceincludes  
		{ 
			"PreCompiled.h"
		}

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
		    files
            {
                "%{prj.name}/**.natvis",
            }
            removefiles
            {
                "%{prj.name}/Include/Application/Mac/**",
                "%{prj.name}/Source/Application/Mac/**",

				"%{prj.name}/Include/Input/Mac/**",
				"%{prj.name}/Source/Input/Mac/**",

				"%{prj.name}/Include/Networking/Mac/**",
				"%{prj.name}/Source/Networking/Mac/**",

				"%{prj.name}/Include/Threading/Mac/**",
				"%{prj.name}/Source/Threading/Mac/**",

				"%{prj.name}/Include/Memory/Mac/**",
				"%{prj.name}/Source/Memory/Mac/**",
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

				"%{prj.name}/Include/Networking/Win32/**",
				"%{prj.name}/Source/Networking/Win32/**",

				"%{prj.name}/Include/Threading/Win32/**",
				"%{prj.name}/Source/Threading/Win32/**",

				"%{prj.name}/Include/Memory/Win32/**",
				"%{prj.name}/Source/Memory/Win32/**",
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
			"%{prj.name}",
			"%{prj.name}/Include",
		}

		sysincludedirs
		{
			"Dependencies/glm",
			"Dependencies/tinyobjloader",
			"Dependencies/WavLib",
			"Dependencies/imgui",
			"Dependencies/stb",
			"Dependencies/portaudio/include",
			"Dependencies/glslang/include",
			"Dependencies/imnodes",
			"Dependencies/rapidjson/include",
			"Dependencies/ordered-map/include",
		}

		links
		{
			"tinyobjloader",
			"WavLib",
			"ImGui",
			"imnodes",
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
				-- Vulkan
				VK_SDK_PATH .. "/Lib",

				-- FMOD
				"C:/FMOD Studio API Windows/api/core/lib/x64",
				"C:/Program Files (x86)/FMOD SoundSystem/FMOD Studio API Windows/api/core/lib/x64",
				"D:/Program Files (x86)/FMOD SoundSystem/FMOD Studio API Windows/api/core/lib/x64",
				"D:/FMOD Studio API Windows/api/core/lib/x64",

				-- PortAudio
				"Dependencies/portaudio/lib",

				-- imgui-node-editor
				"Dependencies/imgui-node-editor/lib",
			}

			sysincludedirs
			{
				-- Vulkan
				VK_SDK_PATH .. "/Include",

				-- FMOD
				"C:/FMOD Studio API Windows/api/core/inc",
				"C:/Program Files (x86)/FMOD SoundSystem/FMOD Studio API Windows/api/core/inc",
				"D:/Program Files (x86)/FMOD SoundSystem/FMOD Studio API Windows/api/core/inc",
				"D:/FMOD Studio API Windows/api/core/inc",
			}
		filter { "system:windows", "configurations:Debug" }
			links
			{
				-- Audio
				"portaudio_x64_d.lib",

				-- Shader Compilation
				"glslangd.lib",
				"MachineIndependentd.lib",
				"GenericCodeGend.lib",
				"SPIRVd.lib",
				"SPIRV-Toolsd.lib",
				"SPIRV-Tools-optd.lib",
				"OSDependentd.lib",
				"OGLCompilerd.lib",
				"HLSLd.lib",
			}
		filter { "system:windows", "configurations:Release or Production" }
			links
			{
				-- Audio
				"portaudio_x64.lib",

				-- Shader Compilation
				"glslang.lib",
				"MachineIndependent.lib",
				"GenericCodeGen.lib",
				"SPIRV.lib",
				"SPIRV-Tools.lib",
				"SPIRV-Tools-opt.lib",
				"OSDependent.lib",
				"OGLCompiler.lib",
				"HLSL.lib",
			}
		-- Mac
		filter { "system:macosx" }
			libdirs
			{
				"/usr/local/lib",
				"../FMODProgrammersAPI/api/core/lib",
			}

			sysincludedirs
			{
				"/usr/local/include",
				"../FMODProgrammersAPI/api/core/inc",
			}

			links
			{
				-- Vulkan
				"vulkan.1",
				"vulkan.1.2.135",

				-- Audio
				"portaudio",
				"fmodL",

				-- Shader Compilation
				"glslang",
				"SPIRV",
				"SPIRV-Tools",
				"SPIRV-Tools-opt",
				"OSDependent",
				"OGLCompiler",
				"HLSL",

				-- Native
				"Cocoa.framework",
				"MetalKit.framework",
			}
		filter {}

		-- Copy .dylib into correct folder on mac builds
		-- filter { "system:macosx"}
		--	postbuildcommands
		--	{
		--		("{COPY} \"../FMODProgrammersAPI/api/core/lib/libfmodL.dylib\" \"../Build/bin/" .. outputdir .. "/Sandbox/\""),
		--		("{COPY} \"../FMODProgrammersAPI/api/core/lib/libfmodL.dylib\" \"../Build/bin/" .. outputdir .. "/Client/\""),
		--		("{COPY} \"../FMODProgrammersAPI/api/core/lib/libfmodL.dylib\" \"../Build/bin/" .. outputdir .. "/Server/\""),
		--	}

		-- Copy DLL into correct folder for windows builds
		-- FMOD
		filter { "system:windows"}
			postbuildcommands
			{
				("{COPY} \"D:/FMOD Studio API Windows/api/core/lib/x64/fmodL.dll\" \"../Build/bin/" .. outputdir .. "/CrazyCanvas/\""),
				("{COPY} \"D:/FMOD Studio API Windows/api/core/lib/x64/fmodL.dll\" \"../Build/bin/" .. outputdir .. "/Sandbox/\""),
				("{COPY} \"D:/FMOD Studio API Windows/api/core/lib/x64/fmodL.dll\" \"../Build/bin/" .. outputdir .. "/Client/\""),
				("{COPY} \"D:/FMOD Studio API Windows/api/core/lib/x64/fmodL.dll\" \"../Build/bin/" .. outputdir .. "/Server/\""),
				("{COPY} \"D:/Program Files (x86)/FMOD SoundSystem/FMOD Studio API Windows/api/core/lib/x64/fmodL.dll\" \"../Build/bin/" .. outputdir .. "/CrazyCanvas/\""),
				("{COPY} \"D:/Program Files (x86)/FMOD SoundSystem/FMOD Studio API Windows/api/core/lib/x64/fmodL.dll\" \"../Build/bin/" .. outputdir .. "/Sandbox/\""),
				("{COPY} \"D:/Program Files (x86)/FMOD SoundSystem/FMOD Studio API Windows/api/core/lib/x64/fmodL.dll\" \"../Build/bin/" .. outputdir .. "/Client/\""),
				("{COPY} \"D:/Program Files (x86)/FMOD SoundSystem/FMOD Studio API Windows/api/core/lib/x64/fmodL.dll\" \"../Build/bin/" .. outputdir .. "/Server/\""),
				("{COPY} \"C:/Program Files (x86)/FMOD SoundSystem/FMOD Studio API Windows/api/core/lib/x64/fmodL.dll\" \"../Build/bin/" .. outputdir .. "/CrazyCanvas/\""),
				("{COPY} \"C:/Program Files (x86)/FMOD SoundSystem/FMOD Studio API Windows/api/core/lib/x64/fmodL.dll\" \"../Build/bin/" .. outputdir .. "/Sandbox/\""),
				("{COPY} \"C:/Program Files (x86)/FMOD SoundSystem/FMOD Studio API Windows/api/core/lib/x64/fmodL.dll\" \"../Build/bin/" .. outputdir .. "/Client/\""),
				("{COPY} \"C:/Program Files (x86)/FMOD SoundSystem/FMOD Studio API Windows/api/core/lib/x64/fmodL.dll\" \"../Build/bin/" .. outputdir .. "/Server/\""),
				("{COPY} \"C:/FMOD Studio API Windows/api/core/lib/x64/fmodL.dll\" \"../Build/bin/" .. outputdir .. "/CrazyCanvas/\""),
				("{COPY} \"C:/FMOD Studio API Windows/api/core/lib/x64/fmodL.dll\" \"../Build/bin/" .. outputdir .. "/Sandbox/\""),
				("{COPY} \"C:/FMOD Studio API Windows/api/core/lib/x64/fmodL.dll\" \"../Build/bin/" .. outputdir .. "/Client/\""),
				("{COPY} \"C:/FMOD Studio API Windows/api/core/lib/x64/fmodL.dll\" \"../Build/bin/" .. outputdir .. "/Server/\"")
			}
		-- LambdaEngine
		filter { "system:windows", "platforms:x64_SharedLib" }
			postbuildcommands
			{
				("{COPY} %{cfg.buildtarget.relpath} \"../Build/bin/" .. outputdir .. "/CrazyCanvas/\""),
				("{COPY} %{cfg.buildtarget.relpath} \"../Build/bin/" .. outputdir .. "/Sandbox/\""),
				("{COPY} %{cfg.buildtarget.relpath} \"../Build/bin/" .. outputdir .. "/Client/\""),
				("{COPY} %{cfg.buildtarget.relpath} \"../Build/bin/" .. outputdir .. "/Server/\""),
			}
		-- Portaudio
		filter { "system:windows", "configurations:Debug"}
			postbuildcommands
			{
				("{COPY} \"../Dependencies/portaudio/dll/debug/portaudio_x64.dll\" \"../Build/bin/" .. outputdir .. "/CrazyCanvas/\""),
				("{COPY} \"../Dependencies/portaudio/dll/debug/portaudio_x64.dll\" \"../Build/bin/" .. outputdir .. "/Sandbox/\""),
				("{COPY} \"../Dependencies/portaudio/dll/debug/portaudio_x64.dll\" \"../Build/bin/" .. outputdir .. "/Client/\""),
				("{COPY} \"../Dependencies/portaudio/dll/debug/portaudio_x64.dll\" \"../Build/bin/" .. outputdir .. "/Server/\"")
			}
		filter { "system:windows", "configurations:Release or Production"}
			postbuildcommands
			{
				("{COPY} \"../Dependencies/portaudio/dll/release/portaudio_x64.dll\" \"../Build/bin/" .. outputdir .. "/CrazyCanvas/\""),
				("{COPY} \"../Dependencies/portaudio/dll/release/portaudio_x64.dll\" \"../Build/bin/" .. outputdir .. "/Sandbox/\""),
				("{COPY} \"../Dependencies/portaudio/dll/release/portaudio_x64.dll\" \"../Build/bin/" .. outputdir .. "/Client/\""),
				("{COPY} \"../Dependencies/portaudio/dll/release/portaudio_x64.dll\" \"../Build/bin/" .. outputdir .. "/Server/\"")
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
			"Dependencies/imgui",
			"Dependencies/ordered-map/include",
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
			"ImGui",
		}

	project "*"

	-- CrazyCanvas Project
	project "CrazyCanvas"
		kind "WindowedApp"
		language "C++"
		cppdialect "C++17"
		systemversion "latest"
		location "CrazyCanvas"

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
			"Dependencies/imgui",
			"Dependencies/ordered-map/include",
			"Dependencies/rapidjson/include",
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
			"ImGui",
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

		sysincludedirs
		{
			"Dependencies/glm",
			"Dependencies/imgui",
			"Dependencies/ordered-map/include",
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
			"ImGui",
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

		sysincludedirs
		{
			"Dependencies/glm",
			"Dependencies/imgui",
			"Dependencies/ordered-map/include",
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
			"ImGui",
		}
	project "*"