project "ImGui"
    kind "StaticLib"
    language "C++"
	staticruntime "on"
    toolset "v143"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "imconfig.h",
        "imgui.h",
        "imgui.cpp",
        "imgui_draw.cpp",
        "imgui_internal.h",
        "imgui_widgets.cpp",
        "imstb_rectpack.h",
        "imstb_textedit.h",
        "imstb_truetype.h",
        "imgui_demo.cpp",
        "imgui_tables.cpp",
        "backends/imgui_impl_vulkan.cpp",
        "backends/imgui_impl_vulkan.h",
    }

    includedirs
    {
     "../imgui",
     "$(VULKAN_SDK)/Include"
    }
    
    filter "system:windows"
        systemversion "10.0.22621.0"
 
     defines
	 {
	 	 "SOL_PLATFORM_WINDOWS"
	 }
 
     filter "configurations:Debug"
	 	defines "SOL_DEBUG"
        runtime "Debug"
        symbols "on"               -- Deixamos on os debug symbols
        
     filter "configurations:Release"
	 	defines "SOL_DEBUG"
        runtime "Release"
        optimize "on"            -- Ripamos todas as configurações de debug e otimizamos o projeto

     filter "configurations:Dist"
	 defines "SOL_DIST"
	 runtime "Release"
	 symbols "Off"
	 optimize "Full"
