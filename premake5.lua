-----------------------------------------------------------------
-- Copyright 2026 by Riddhiman Raguraman
-----------------------------------------------------------------

-- premake5.lua
-- Build configuration for the Vulkan_Baseline solution.
--
-- Regenerate the Visual Studio solution + projects by running:
--     GenerateProjects.bat            (or:  Vendor\bin\premake\premake5.exe vs2022)
--
-- This file is the single source of truth. The generated .sln/.vcxproj files
-- are disposable -- the CleanMe / UberCleanMe scripts delete them, and running
-- premake recreates them from what is described here.

workspace "Vulkan_Baseline"
	architecture "x64"				-- x64 only (32-bit dropped)
	startproject "Vulkan_Baseline"

	configurations { "Debug", "Release" }
	location "."

	-- Compile each project's .cpp files across all CPU cores (MSVC /MP).
	multiprocessorcompile "On"

	-- Edit-and-Continue debug info (/ZI) disables /MP, so use plain /Zi.
	-- Trade: no editing code mid-debug-session, but Debug builds go parallel.
	editandcontinue "Off"

	filter "system:windows"
		systemversion "latest"
	filter {}

-- Per-config output tag, e.g. "Debug-windows-x86_64".
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Everything (app .exe + lib .dll + test .exe) builds into ONE runtime folder
-- at the solution root: <solution>/x64/<cfg>. The app and tests then sit right
-- next to the DLLs they load, so no copy step is needed. This is the
-- "x64 folder in the solution folder" layout.
local outputBin = "%{wks.location}/x64/%{cfg.buildcfg}"

-- All intermediate files (.obj, vc143.pdb, ...) go under <solution>/obj,
-- split per project+config -- never scattered inside the project folders.
local outputObj = "%{wks.location}/obj/" .. outputdir .. "/%{prj.name}"

-- Framework is header-only (just Framework.h) and lives in the app folder.
-- Every project force-includes it, so put its folder on the include path.
local frameworkDir = "Vulkan_Baseline/Framework"

-----------------------------------------------------------------------------
-- VULKAN SDK -- CHANGE THIS IF YOUR INSTALL DIFFERS
-----------------------------------------------------------------------------
-- The Vulkan SDK installer sets a VULKAN_SDK environment variable, so that is
-- used automatically when present and the path below is only a fallback.
--
-- If you installed a different SDK version or a non-default location, either
-- re-open your shell so VULKAN_SDK is picked up, or edit the fallback path
-- here to match your install. Then re-run GenerateProjects.bat.
--
-- Expected layout under this folder:
--     Include\vulkan\vulkan.h     <- headers
--     Lib\vulkan-1.lib            <- loader import library (x64)
--
-- Note: vulkan-1.dll itself is NOT copied into our output. It ships with the
-- graphics driver and lives in C:\Windows\System32, so linking the import
-- library vulkan-1.lib below is all that is needed to use the loader DLL.
local vulkanSDK = os.getenv("VULKAN_SDK") or "C:/VulkanSDK/1.4.350.0"

--=============================================================================
-- Library helper: builds Math / File / AnimTime from source as its own DLL.
--   name      : project name / Libs subfolder (e.g. "Math")
--   apiPrefix : export-macro prefix (e.g. "MATH" -> MATH_USE_DLL, MATH_LIBRARY_EXPORTS)
--=============================================================================
local function defineLibrary(name, apiPrefix)
	project(name)
		location   ("Libs/" .. name)
		language   "C++"
		kind       "SharedLib"
		cppdialect "C++17"
		staticruntime "Off"
		characterset "MBCS"
		toolset    "v143"

		targetdir (outputBin)
		objdir    (outputObj)

		files {
			"Libs/" .. name .. "/include/**.h",
			"Libs/" .. name .. "/src/**.h",
			"Libs/" .. name .. "/src/**.cpp"
		}

		includedirs {
			"Libs/" .. name .. "/include",
			"Libs/" .. name .. "/src",
			frameworkDir
		}

		-- Precompile the large, force-included Framework.h via pch.h to cut
		-- build time. pch.h is force-included (not Framework.h) so the library
		-- sources compile unchanged -- no need to add #include "pch.h" to each .cpp.
		pchheader "pch.h"
		pchsource ("Libs/" .. name .. "/src/pch.cpp")
		forceincludes { "pch.h" }

		defines {
			apiPrefix .. "_USE_DLL",			-- turn on the dll interface
			apiPrefix .. "_LIBRARY_EXPORTS",	-- this project exports the symbols
			'WINDOWS_TARGET_PLATFORM="$(TargetPlatformVersion)"',
			'SOLUTION_DIR=R"($(SolutionDir))"',
			'TOOLS_VERSION=R"($(VCToolsVersion))"',
			'LOCAL_WORKING_DIR=R"($(LocalDebuggerWorkingDirectory))"'
		}

		filter "configurations:Debug"
			runtime "Debug"
			symbols "On"
			defines { "_DEBUG" }

		filter "configurations:Release"
			runtime "Release"
			optimize "On"
			defines { "NDEBUG" }
		filter {}
end

--=============================================================================
-- Unit-test helper: builds a library's test suite as a standalone console app
-- that links the lib DLL under test. Set it as the startup project to run the
-- tests, then switch back to Vulkan_Baseline.
--   name      : test project name   (e.g. "MathTest")
--   apiPrefix : consumed lib's macro prefix (e.g. "MATH")
--   libName   : lib project + Libs subfolder under test (e.g. "Math")
--=============================================================================
local function defineUnitTest(name, apiPrefix, libName)
	project(name)
		location   ("Libs/" .. libName)
		language   "C++"
		kind       "ConsoleApp"
		cppdialect "C++17"
		staticruntime "Off"
		characterset "MBCS"
		toolset    "v143"

		targetdir (outputBin)
		objdir    (outputObj)

		-- Some suites use "*_Group.cpp" unity files that #include the individual
		-- test .cpp files. Compiling both the group and the file it includes
		-- defines every test twice (LNK2005). So exclude exactly the files that a
		-- group #includes, and compile everything else -- the groups, main, and
		-- support files (e.g. Mat4Test.cpp).
		local testDir = "Libs/" .. libName .. "/Test/"
		local included = {}
		for _, g in ipairs(os.matchfiles(testDir .. "*_Group.cpp")) do
			for inc in (io.readfile(g) or ""):gmatch('#include%s+"([^"]-%.[cC][pP][pP])"') do
				included[inc:match("[^/\\]+$"):lower()] = true
			end
		end
		files { testDir .. "**.h" }
		for _, f in ipairs(os.matchfiles(testDir .. "**.cpp")) do
			if not included[f:match("[^/\\]+$"):lower()] then
				files { f }
			end
		end

		includedirs {
			"Libs/" .. libName .. "/include",
			"Libs/" .. libName .. "/Test",
			frameworkDir
		}

		links { libName }		-- consume the lib DLL under test

		-- Precompile Framework.h (same rationale as the library helper above).
		pchheader "pch.h"
		pchsource ("Libs/" .. libName .. "/Test/pch.cpp")
		forceincludes { "pch.h" }

		defines {
			apiPrefix .. "_USE_DLL",	-- consume via dllimport
			"_CONSOLE",
			'WINDOWS_TARGET_PLATFORM="$(TargetPlatformVersion)"',
			'SOLUTION_DIR=R"($(SolutionDir))"',
			'TOOLS_VERSION=R"($(VCToolsVersion))"',
			'LOCAL_WORKING_DIR=R"($(LocalDebuggerWorkingDirectory))"'
		}

		filter "configurations:Debug"
			runtime "Debug"
			symbols "On"
			defines { "_DEBUG" }

		filter "configurations:Release"
			runtime "Release"
			optimize "On"
			defines { "NDEBUG" }
		filter {}
end

--=============================================================================
-- Main application: Vulkan_Baseline (console app). Links the three DLL libs.
--=============================================================================
project "Vulkan_Baseline"
	location "Vulkan_Baseline"
	language "C++"
	kind "ConsoleApp"
	cppdialect "C++17"
	staticruntime "Off"
	characterset "MBCS"
	toolset "v143"

	targetdir (outputBin)
	objdir    (outputObj)

	files {
		"Vulkan_Baseline/**.h",
		"Vulkan_Baseline/**.hpp",
		"Vulkan_Baseline/**.cpp"
	}

	includedirs {
		"Vulkan_Baseline/Framework",
		"Vulkan_Baseline/Source",
		"Vulkan_Baseline/Window",
		"Vulkan_Baseline/Vulkan",
		"Libs/Math/include",
		"Libs/File/include",
		"Libs/AnimTime/include",
		vulkanSDK .. "/Include"			-- <vulkan/vulkan.h>
	}

	-- Link the source-built libraries. DLL linking is NOT transitive: any module
	-- that directly calls into a DLL needs that DLL's own import lib, so each lib
	-- the app uses must be listed here (and its *_USE_DLL define set below).
	links { "Math", "File", "AnimTime" }

	-- Vulkan loader. This is a prebuilt external library, not one of our
	-- projects, so it is just an import lib + include path -- nothing is built
	-- from source and no DLL is copied (see the vulkanSDK note at the top).
	libdirs { vulkanSDK .. "/Lib" }		-- x64 libs ("Lib32" would be 32-bit)
	links   { "vulkan-1" }

	-- The app force-includes Framework.h directly (no pch of its own).
	forceincludes { "Framework.h" }

	defines {
		"MATH_USE_DLL",			-- consume Math via dllimport
		"FILE_USE_DLL",			-- consume File via dllimport
		"ANIM_TIME_USE_DLL",	-- consume AnimTime via dllimport
		"_CONSOLE",

		-- Turns on the Win32 half of <vulkan/vulkan.h>: VkWin32SurfaceCreateInfoKHR
		-- and vkCreateWin32SurfaceKHR. Without this, VulkanSurface will not compile.
		"VK_USE_PLATFORM_WIN32_KHR",
		'WINDOWS_TARGET_PLATFORM="$(TargetPlatformVersion)"',
		'SOLUTION_DIR=R"($(SolutionDir))"',
		'TOOLS_VERSION=R"($(VCToolsVersion))"',
		'LOCAL_WORKING_DIR=R"($(LocalDebuggerWorkingDirectory))"'
	}

	filter "configurations:Debug"
		runtime "Debug"
		symbols "On"
		defines { "_DEBUG" }

	filter "configurations:Release"
		runtime "Release"
		optimize "On"
		defines { "NDEBUG" }

	-- Wipe any stale Azul output folder before building (matches the course's
	-- CleanMe scripts). Guarded by "if exist" so it never errors when absent.
	filter "action:vs*"
		prebuildcommands {
			'if exist "$(SolutionDir)Azul" rmdir /S /Q "$(SolutionDir)Azul"'
		}
	filter {}

--=============================================================================
-- Solution Explorer groups.
--=============================================================================
group "Libs"
	defineLibrary("Math", "MATH")
	defineLibrary("File", "FILE")
	defineLibrary("AnimTime", "ANIM_TIME")
group ""

group "Tests"
	defineUnitTest("MathTest", "MATH", "Math")
	defineUnitTest("FileTest", "FILE", "File")
group ""
