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

	print(string.format("No environment variables for path to Vulkan SDK are set: %s", array_to_string(sdkPathVars)))
	return ""
end

-- libFolder is either checked or release
function get_physx_copy_commands(libFolder, outputdir)
	libDir = "../Dependencies/PhysX/lib/" .. libFolder

	DLL_FILES = {
		"PhysX_64.dll",
		"PhysXCommon_64.dll",
		"PhysXCooking_64.dll",
		"PhysXFoundation_64.dll",
		"PhysXGpu_64.dll"
	}

	targetPaths = { "\"../Build/bin/" .. outputdir .. "/CrazyCanvas/\"" }

	copyCommands = {}

	for _, dllFile in ipairs(DLL_FILES) do
		for _, targetPath in ipairs(targetPaths) do
			copyCommand = string.format("{COPY} \"%s/%s\" %s", libDir, dllFile, targetPath)
			table.insert(copyCommands, copyCommand)
		end
	end

	return copyCommands
end

function get_fmod_dll_path()
	local driveLetters = {"C", "D"}
	local potentialPaths = {
		":/FMOD Studio API Windows/api/core/lib/x64/fmodL.dll",
		":/Program Files (x86)/FMOD SoundSystem/FMOD Studio API Windows/api/core/lib/x64/fmodL.dll"
	}

	for _, path in ipairs(potentialPaths) do
		for _, driveLetter in ipairs(driveLetters) do
			local fullPath = driveLetter .. path
			if os.isfile(fullPath) then
				return "\"" .. fullPath .. "\""
			end
		end
	end

	print("fmodL.dll was not found, ensure that FMOD engine is installed or modify premake5.lua")
end

VK_SDK_PATH		= get_vk_sdk_path()
FMOD_DLL_PATH	= get_fmod_dll_path()

workspace "LambdaEngine"
	startproject "CrazyCanvas"
	architecture "x64"
	warnings "extra"
	flags { "MultiProcessorCompile" }
	
	-- Disable C4201 nonstandard extension used: nameless struct/union
	disablewarnings { "4201" }

	-- Link warning suppression
	-- LNK4006: Sympbol already defined in another library will pick first definition
	-- LNK4099: Debugging Database file (pdb) missing for given obj
	-- LNK4098: defaultlib 'library' conflicts with use of other libs; use /NODEFAULTLIB:library
	linkoptions { "-IGNORE:4006,4099,4098" }

	-- Set output dir
	outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}-%{cfg.platform}"

	-- Platform
	platforms
	{
		"x64"
	}
	defines
	{
		"NS_STATIC_LIBRARY"
	}
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
		-- Disable C100 Unused parameter
		disablewarnings { "4100" }
	filter "configurations:Production"
		symbols "off"
		runtime "Release"
		optimize "Full"
		defines
		{
			"NDEBUG",
			"LAMBDA_CONFIG_PRODUCTION",
		}
		-- Disable C100 Unused parameter
		disablewarnings { "4100" }
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
		include "Dependencies/imgui"

		-- imnodes Project
		project "imnodes"
			kind "StaticLib"
			language "C++"
			cppdialect "C++latest"
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
		cppdialect "C++latest"
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
		kind "StaticLib"

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
			"Dependencies/",
			"Dependencies/assimp/include",
			"Dependencies/imgui",
			"Dependencies/imnodes",
			"Dependencies/glm",
			"Dependencies/glslang/include",
			"Dependencies/ordered-map/include",
			"Dependencies/PhysX/include",
			"Dependencies/rapidjson/include",
			"Dependencies/stb",
			"Dependencies/NoesisGUI/Include",
			"Dependencies/NoesisGUI/Providers/Include",
		}

		links
		{
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

				-- imgui-node-editor
				"Dependencies/imgui-node-editor/lib",

				-- Assimp
				"Dependencies/assimp/bin",

				-- PhysX
				"Dependencies/PhysX/lib",
				-- NoesisGUI
				"Dependencies/NoesisGUI/Lib",
			}

			sysincludedirs
			{
				-- FMOD
				"C:/FMOD Studio API Windows/api/core/inc",
				"C:/Program Files (x86)/FMOD SoundSystem/FMOD Studio API Windows/api/core/inc",
				"D:/Program Files (x86)/FMOD SoundSystem/FMOD Studio API Windows/api/core/inc",
				"D:/FMOD Studio API Windows/api/core/inc",
			}
		filter { "system:windows", "configurations:Debug" }
			links
			{
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

				-- Assimp
				"/debug/assimp-vc142-mtd.lib",
				"/debug/IrrXMLd.lib",
				"/debug/zlibstaticd.lib",

				-- PhysX
				"/checked/PhysX_64.lib",
				"/checked/PhysXCharacterKinematic_dynamic_64.lib",
				"/checked/PhysXCommon_64.lib",
				"/checked/PhysXCooking_64.lib",
				"/checked/PhysXExtensions_dynamic_64.lib",
				"/checked/PhysXFoundation_64.lib",
				"/checked/PhysXPvdSDK_dynamic_64.lib",
				--NoesisGUI
				"Noesis.lib",
				"NoesisApp.lib",
			}
		filter { "system:windows", "configurations:Release or Production" }
			links
			{
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

				-- Assimp
				"/release/assimp-vc142-mt.lib",
				"/release/IrrXML.lib",
				"/release/zlibstatic.lib",

				-- PhysX
				"/release/PhysX_64.lib",
				"/release/PhysXCharacterKinematic_dynamic_64.lib",
				"/release/PhysXCommon_64.lib",
				"/release/PhysXCooking_64.lib",
				"/release/PhysXExtensions_dynamic_64.lib",
				"/release/PhysXFoundation_64.lib",
				--NoesisGUI
				"Noesis.lib",
				"NoesisApp.lib",
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

		-- FMOD
		filter { "system:windows"}
			postbuildcommands
			{
				("{COPY} " .. FMOD_DLL_PATH .. " \"../Build/bin/" .. outputdir .. "/CrazyCanvas/\"")
			}
			postbuildcommands
			{
				("{COPY} " .. "../Dependencies/NoesisGUI/Lib/Noesis.dll" .. " \"../Build/bin/" .. outputdir .. "/CrazyCanvas/\""),
				("{COPY} " .. "../Dependencies/NoesisGUI/Lib/NoesisApp.dll" .. " \"../Build/bin/" .. outputdir .. "/CrazyCanvas/\"")
			}
		-- PhysX
		filter { "system:windows", "configurations:Debug" }
			postbuildcommands
			{
				get_physx_copy_commands("checked", outputdir)
			}
			filter { "system:windows", "configurations:Release or Production" }
			postbuildcommands
			{
				get_physx_copy_commands("release", outputdir)
			}
		filter {}
	project "*"

	-- CrazyCanvas, Sandbox and Server are identical projects besides from their debugargs
	-- The variables beneath are used in all projects
	prj_include_dirs =
	{
		"LambdaEngine/Include",
		"CrazyCanvas/Include",
	}

	prj_sys_include_dirs =
	{
		"Dependencies/",
		"Dependencies/glm",
		"Dependencies/imgui",
		"Dependencies/NoesisGUI/Include",
		"Dependencies/ordered-map/include",
		"Dependencies/PhysX/include",
		"Dependencies/rapidjson/include",
		"Dependencies/NoesisGUI/Include",
		-- FMOD
		"C:/FMOD Studio API Windows/api/core/inc",
		"C:/Program Files (x86)/FMOD SoundSystem/FMOD Studio API Windows/api/core/inc",
		"D:/Program Files (x86)/FMOD SoundSystem/FMOD Studio API Windows/api/core/inc",
		"D:/FMOD Studio API Windows/api/core/inc",
	}

	prj_files =
	{
		"LambdaEngine/Source/Launch/**",
		"CrazyCanvas/**.hpp",
		"CrazyCanvas/**.h",
		"CrazyCanvas/**.inl",
		"CrazyCanvas/**.cpp",
		"CrazyCanvas/**.c",
		"CrazyCanvas/**.hlsl",
	}

	prj_excludes = { "**.hlsl" }

	prj_links =
	{
		"LambdaEngine",
		"ImGui",
	}

	prj_links_multiplayer =
	{
		"LambdaEngine",
		"ImGui",
		"Server",
	}

	-- CrazyCanvas Project
	project "CrazyCanvas"
		kind "WindowedApp"
		language "C++"
		cppdialect "C++latest"
		systemversion "latest"
		location "CrazyCanvas"

		-- Targets
		targetdir ("Build/bin/" .. outputdir .. "/CrazyCanvas")
		objdir ("Build/bin-int/" .. outputdir .. "/CrazyCanvas")

		debugargs { "--state=crazycanvas" }

		--Includes
		includedirs { prj_include_dirs }
		sysincludedirs { prj_sys_include_dirs }

		-- Files
		files { prj_files }
		-- We do not want to compile HLSL files
		excludes { prj_excludes }
		-- Linking
		links { prj_links_multiplayer }

	project "*"

	-- Sandbox Project
	project "Sandbox"
		kind "WindowedApp"
		language "C++"
		cppdialect "C++latest"
		systemversion "latest"
		location "CrazyCanvas"

		-- Targets
		targetdir ("Build/bin/" .. outputdir .. "/CrazyCanvas")
		objdir ("Build/bin-int/" .. outputdir .. "/CrazyCanvas")

		debugargs { "--state=sandbox" }

		--Includes
		includedirs { prj_include_dirs }
		sysincludedirs { prj_sys_include_dirs }

		-- Files
		files { prj_files }
		-- We do not want to compile HLSL files
		excludes { prj_excludes }
		-- Linking
		links { prj_links }

	project "*"

	-- Server Project
	project "Server"
		kind "WindowedApp"
		language "C++"
		cppdialect "C++latest"
		systemversion "latest"
		location "CrazyCanvas"

		-- Targets
		targetdir ("Build/bin/" .. outputdir .. "/CrazyCanvas")
		objdir ("Build/bin-int/" .. outputdir .. "/CrazyCanvas")

		debugargs { "--state=server" }

		--Includes
		includedirs { prj_include_dirs }
		sysincludedirs { prj_sys_include_dirs }

		-- defines
		defines
		{
			"BUILD_SERVER"
		}

		-- Files
		files { prj_files }
		-- We do not want to compile HLSL files
		excludes { prj_excludes }
		-- Linking
		links { prj_links }

	project "*"
