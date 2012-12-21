solution "advanced_opengl_imac3_exercises"
   configurations { "Debug", "Release" }
   platforms {"native", "x64", "x32"}

   -- TD1 Exercicse 1
   project "td1"
      kind "ConsoleApp"
      language "C++"
      files { "td1/1.cpp", "common/*.hpp", "common/*.cpp", "common/*.h" }
      includedirs { "lib/glfw/include", "src", "common", "lib/" }
      links {"glfw", "glew", "stb_image"}
      defines { "GLEW_STATIC" }
	  
      configuration { "linux" }
         links {"X11","Xrandr", "rt", "GL", "GLU", "pthread"}
		 
      configuration { "windows" }
         links {"glu32","opengl32", "gdi32", "winmm", "user32"}

      configuration { "macosx" }
         linkoptions { "-framework OpenGL", "-framework Cocoa", "-framework IOKit" }
		 
      configuration "Debug"
         defines { "DEBUG" }
         flags { "Symbols" }
         targetdir "bin/debug/td1/"

      configuration "Release"
         defines { "NDEBUG" }
         flags { "Optimize"}    
         targetdir "bin/release/td1/"

   -- TD1 Exercicse 1 Corrected
   project "td1_corrected"
      kind "ConsoleApp"
      language "C++"
      files { "td1/1_corrected.cpp", "common/*.hpp", "common/*.cpp", "common/*.h" }
      includedirs { "lib/glfw/include", "src", "common", "lib/" }
      links {"glfw", "glew", "stb_image"}
      defines { "GLEW_STATIC" }
     
      configuration { "linux" }
         links {"X11","Xrandr", "rt", "GL", "GLU", "pthread"}
       
      configuration { "windows" }
         links {"glu32","opengl32", "gdi32", "winmm", "user32"}

      configuration { "macosx" }
         linkoptions { "-framework OpenGL", "-framework Cocoa", "-framework IOKit" }
       
      configuration "Debug"
         defines { "DEBUG" }
         flags { "Symbols" }
         targetdir "bin/debug/td1/"

      configuration "Release"
         defines { "NDEBUG" }
         flags { "Optimize"}    
         targetdir "bin/release/td1/"         

   -- TD2 Exercicse Forward Corrected
   project "td2_forward"
      kind "ConsoleApp"
      language "C++"
      files { "td2/2_forward.cpp", "common/*.hpp", "common/*.cpp", "common/*.h" }
      includedirs { "lib/glfw/include", "src", "common", "lib/" }
      links {"glfw", "glew", "stb_image"}
      defines { "GLEW_STATIC" }
     
      configuration { "linux" }
         links {"X11","Xrandr", "rt", "GL", "GLU", "pthread"}
       
      configuration { "windows" }
         links {"glu32","opengl32", "gdi32", "winmm", "user32"}

      configuration { "macosx" }
         linkoptions { "-framework OpenGL", "-framework Cocoa", "-framework IOKit" }
       
      configuration "Debug"
         defines { "DEBUG" }
         flags { "Symbols" }
         targetdir "bin/debug/td2/"

      configuration "Release"
         defines { "NDEBUG" }
         flags { "Optimize"}    
         targetdir "bin/release/td2/"     

   -- TD2 Exercicse Forward Corrected
   project "td2_forward_corrected"
      kind "ConsoleApp"
      language "C++"
      files { "td2/2_forward_corrected.cpp", "common/*.hpp", "common/*.cpp", "common/*.h" }
      includedirs { "lib/glfw/include", "src", "common", "lib/" }
      links {"glfw", "glew", "stb_image"}
      defines { "GLEW_STATIC" }
     
      configuration { "linux" }
         links {"X11","Xrandr", "rt", "GL", "GLU", "pthread"}
       
      configuration { "windows" }
         links {"glu32","opengl32", "gdi32", "winmm", "user32"}

      configuration { "macosx" }
         linkoptions { "-framework OpenGL", "-framework Cocoa", "-framework IOKit" }
       
      configuration "Debug"
         defines { "DEBUG" }
         flags { "Symbols" }
         targetdir "bin/debug/td2/"

      configuration "Release"
         defines { "NDEBUG" }
         flags { "Optimize"}    
         targetdir "bin/release/td2/"      

   -- TD2 Exercicse Deferred 
   project "td2_deferred"
      kind "ConsoleApp"
      language "C++"
      files { "td2/2_deferred.cpp", "common/*.hpp", "common/*.cpp", "common/*.h" }
      includedirs { "lib/glfw/include", "src", "common", "lib/" }
      links {"glfw", "glew", "stb_image"}
      defines { "GLEW_STATIC" }
     
      configuration { "linux" }
         links {"X11","Xrandr", "rt", "GL", "GLU", "pthread"}
       
      configuration { "windows" }
         links {"glu32","opengl32", "gdi32", "winmm", "user32"}

      configuration { "macosx" }
         linkoptions { "-framework OpenGL", "-framework Cocoa", "-framework IOKit" }
       
      configuration "Debug"
         defines { "DEBUG" }
         flags { "Symbols" }
         targetdir "bin/debug/td2/"

      configuration "Release"
         defines { "NDEBUG" }
         flags { "Optimize"}    
         targetdir "bin/release/td2/"        

   -- TD2 Exercicse Deferred
   project "td2_deferred_corrected"
      kind "ConsoleApp"
      language "C++"
      files { "td2/2_deferred_corrected.cpp", "common/*.hpp", "common/*.cpp", "common/*.h" }
      includedirs { "lib/glfw/include", "src", "common", "lib/" }
      links {"glfw", "glew", "stb_image"}
      defines { "GLEW_STATIC" }
     
      configuration { "linux" }
         links {"X11","Xrandr", "rt", "GL", "GLU", "pthread"}
       
      configuration { "windows" }
         links {"glu32","opengl32", "gdi32", "winmm", "user32"}

      configuration { "macosx" }
         linkoptions { "-framework OpenGL", "-framework Cocoa", "-framework IOKit" }
       
      configuration "Debug"
         defines { "DEBUG" }
         flags { "Symbols" }
         targetdir "bin/debug/td2/"

      configuration "Release"
         defines { "NDEBUG" }
         flags { "Optimize"}    
         targetdir "bin/release/td2/"        

   -- TD3 Exercicse Shadow Mapping
   project "td3"
      kind "ConsoleApp"
      language "C++"
      files { "td3/3.cpp", "common/*.hpp", "common/*.cpp", "common/*.h" }
      includedirs { "lib/glfw/include", "src", "common", "lib/" }
      links {"glfw", "glew", "stb_image"}
      defines { "GLEW_STATIC" }
     
      configuration { "linux" }
         links {"X11","Xrandr", "rt", "GL", "GLU", "pthread"}
       
      configuration { "windows" }
         links {"glu32","opengl32", "gdi32", "winmm", "user32"}

      configuration { "macosx" }
         linkoptions { "-framework OpenGL", "-framework Cocoa", "-framework IOKit" }
       
      configuration "Debug"
         defines { "DEBUG" }
         flags { "Symbols" }
         targetdir "bin/debug/td3/"

      configuration "Release"
         defines { "NDEBUG" }
         flags { "Optimize"}    
         targetdir "bin/release/td3/"  

   -- GLFW Library
   project "glfw"
      kind "StaticLib"
      language "C"
      files { "lib/glfw/lib/*.h", "lib/glfw/lib/*.c", "lib/glfw/include/GL/glfw.h" }
      includedirs { "lib/glfw/lib", "lib/glfw/include"}

      configuration {"linux"}
         files { "lib/glfw/lib/x11/*.c", "lib/glfw/x11/*.h" }
         includedirs { "lib/glfw/lib/x11" }
         defines { "_GLFW_USE_LINUX_JOYSTICKS", "_GLFW_HAS_XRANDR", "_GLFW_HAS_PTHREAD" ,"_GLFW_HAS_SCHED_YIELD", "_GLFW_HAS_GLXGETPROCADDRESS" }
         buildoptions { "-pthread" }
       
      configuration {"windows"}
         files { "lib/glfw/lib/win32/*.c", "lib/glfw/win32/*.h" }
         includedirs { "lib/glfw/lib/win32" }
         defines { "_GLFW_USE_LINUX_JOYSTICKS", "_GLFW_HAS_XRANDR", "_GLFW_HAS_PTHREAD" ,"_GLFW_HAS_SCHED_YIELD", "_GLFW_HAS_GLXGETPROCADDRESS" }
       
      configuration {"Macosx"}
         files { "lib/glfw/lib/cocoa/*.c", "lib/glfw/lib/cocoa/*.h", "lib/glfw/lib/cocoa/*.m" }
         includedirs { "lib/glfw/lib/cocoa" }
         defines { }
         buildoptions { " -fno-common" }
         linkoptions { "-framework OpenGL", "-framework Cocoa", "-framework IOKit" }

      configuration "Debug"
         defines { "DEBUG" }
         flags { "Symbols" }
         targetdir "bin/debug"

      configuration "Release"
         defines { "NDEBUG" }
         flags { "Optimize" }    
         targetdir "bin/release"

   -- GLEW Library         
   project "glew"
      kind "StaticLib"
      language "C"
      files {"lib/glew/*.c", "lib/glew/*.h"}
      defines { "GLEW_STATIC" }

      configuration "Debug"
         defines { "DEBUG" }
         flags { "Symbols" }
         targetdir "bin/debug"

      configuration "Release"
         defines { "NDEBUG" }
         flags { "Optimize" }    
         targetdir "bin/release"

   -- stb_image Library         
   project "stb_image"
      kind "StaticLib"
      language "C"
      files {"lib/stb_image/*.c", "lib/stb_image/*.h"}

      configuration "Debug"
         defines { "DEBUG" }
         flags { "Symbols" }
         targetdir "bin/debug"

      configuration "Release"
         defines { "NDEBUG" }
         flags { "Optimize" }    
         targetdir "bin/release"
