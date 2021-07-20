workspace "Volcano"
	architecture "x64"
	configurations { "Debug", "Release" }
	startproject "volcano"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

include "Dependencies/glfw"

project	"volcano"
	location "volcano"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	systemversion "latest"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("intermediate/" .. outputdir .. "/%{prj.name}")

	pchheader "volcano/src/volcanoPCH.h"
	pchsource "volcano/src/volcanoPCH.cpp"

	includedirs { 
		"Dependencies/glfw/include",
		"Dependencies/vulkan/Include",
		"Dependencies/glm",
	}

	links {
		"GLFW"
	}

	files { 
		"%{prj.name}/src/**.h", 
		"%{prj.name}/src/**.cpp",
	}

	filter "system:windows"
		defines {
			"WINDOWS_BUILD"
		}
		libdirs {
			"Dependencies/vulkan/Lib"
		}
		links {
			"vulkan-1"
		}
		postbuildcommands {
			("compile_shaders.bat")
		}

	filter "system:linux"
		defines {
			"LINUX_BUILD"
		}
		links {
			"dl",
			"pthread",
			"X11",
			"Xrandr",
			"Xi",
			"Xxf86vm",
			"vulkan"
		}
		postbuildcommands {
			("./compile_shaders.sh"),
			("cp -r ./shaders/ ../bin/" .. outputdir .. "/%{prj.name}")
		}
	filter "configurations:Debug"
		symbols "On"
		defines { "DEBUG" }

	filter "configurations:Release"
		optimize "On"
		defines { "RELEASE" }
