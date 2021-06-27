workspace "Volcano"
	architecture "x64"
	configurations { "Debug", "Release" }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Include premake5 script in glfw lib
include "Dependencies/glfw"

project	"volcano"
	location "volcano"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	systemversion "latest"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("intermediate/" .. outputdir .. "/%{prj.name}")

	includedirs { 
		"Dependencies/glfw/include",
		"Dependencies/vulkan/Include",
		"Dependencies/vulkan-hpp",
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
		links {
			"vulkan-1"
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

	filter "configurations:Debug"
		symbols "On"
		defines { "DEBUG" }

	filter "configurations:Release"
		defines { "RELEASE" }
		optimize "On"
