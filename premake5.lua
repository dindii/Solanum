workspace "Solanum"
		architecture "x64"
		startproject "Solanum"


configurations
{
	"Debug",
	"Release",
	"Dist"
}

characterset ("MBCS")

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

 IncludeDir = {}
  IncludeDir["ImGui"] = "third_party/imgui"
  IncludeDir["stbi"] = "third_party/stbi"
  IncludeDir["glm"] = "third_party/glm/glm"	

  group "Dependencies"
  include "third_party/imgui"
  include "third_party/glm/glm"
  group ""

project "Solanum"
	location "Solanum"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"
	toolset "v143"  

	targetdir("bin/" .. outputdir .. "/%{prj.name}")
	objdir("bin-int/" .. outputdir .. "/%{prj.name}")

	flags { "NoPCH" }

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
		--"%{prj.name}/third_party/imgui/backends/imgui_impl_vulkan.cpp",
		--"%{prj.name}/third_party/imgui/backends/imgui_impl_vulkan.h"
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/third_party/imgui",

		"$(VULKAN_SDK)/Include",
        "%{IncludeDir.ImGui}",
		"%{IncludeDir.stbi}",
		"%{IncludeDir.glm}"
	}
	
	links
	{
        "ImGui",
		"$(VULKAN_SDK)/Lib/vulkan-1.lib"
	}

	filter "system:windows"
	systemversion "10.0.22621.0"

	defines
	{
		"SOL_PLATFORM_WINDOWS",
		"WIN32_LEAN_AND_MEAN"
	}

	filter "configurations:Debug"
	defines "SOL_DEBUG"
	runtime "Debug"
	symbols "on"

	filter "configurations:Release"
	defines "SOL_DEBUG"
	runtime "Release"
	optimize "Debug"

	filter "configurations:Dist"
	defines "SOL_DIST"
	runtime "Release"
	symbols "Off"
	optimize "Full"
	