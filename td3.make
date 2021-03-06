# GNU Make project makefile autogenerated by Premake
ifndef config
  config=debug
endif

ifndef verbose
  SILENT = @
endif

ifndef CC
  CC = gcc
endif

ifndef CXX
  CXX = g++
endif

ifndef AR
  AR = ar
endif

ifeq ($(config),debug)
  OBJDIR     = obj/Debug/td3
  TARGETDIR  = bin/debug/td3
  TARGET     = $(TARGETDIR)/td3
  DEFINES   += -DGLEW_STATIC -DDEBUG
  INCLUDES  += -Ilib/glfw/include -Isrc -Icommon -Ilib -Ial
  CPPFLAGS  += -MMD -MP $(DEFINES) $(INCLUDES)
  CFLAGS    += $(CPPFLAGS) $(ARCH) -g
  CXXFLAGS  += $(CFLAGS) 
  LDFLAGS   += -Lbin/debug
  LIBS      += -lglfw -lglew -lstb_image -lX11 -lXrandr -lrt -lGL -lGLU -lpthread -lassimp3.0 -lalut -lopenal
  RESFLAGS  += $(DEFINES) $(INCLUDES) 
  LDDEPS    += bin/debug/libglfw.a bin/debug/libglew.a bin/debug/libstb_image.a
  LINKCMD    = $(CXX) -o $(TARGET) $(OBJECTS) $(LDFLAGS) $(RESOURCES) $(ARCH) $(LIBS)
  define PREBUILDCMDS
  endef
  define PRELINKCMDS
  endef
  define POSTBUILDCMDS
  endef
endif

ifeq ($(config),release)
  OBJDIR     = obj/Release/td3
  TARGETDIR  = bin/release/td3
  TARGET     = $(TARGETDIR)/td3
  DEFINES   += -DGLEW_STATIC -DNDEBUG
  INCLUDES  += -Ilib/glfw/include -Isrc -Icommon -Ilib
  CPPFLAGS  += -MMD -MP $(DEFINES) $(INCLUDES)
  CFLAGS    += $(CPPFLAGS) $(ARCH) -O2
  CXXFLAGS  += $(CFLAGS) 
  LDFLAGS   += -s -Lbin/release
  LIBS      += -lglfw -lglew -lstb_image -lX11 -lXrandr -lrt -lGL -lGLU -lpthread -lalut
  RESFLAGS  += $(DEFINES) $(INCLUDES) 
  LDDEPS    += bin/release/libglfw.a bin/release/libglew.a bin/release/libstb_image.a
  LINKCMD    = $(CXX) -o $(TARGET) $(OBJECTS) $(LDFLAGS) $(RESOURCES) $(ARCH) $(LIBS)
  define PREBUILDCMDS
  endef
  define PRELINKCMDS
  endef
  define POSTBUILDCMDS
  endef
endif

ifeq ($(config),debug64)
  OBJDIR     = obj/x64/Debug/td3
  TARGETDIR  = bin/debug/td3
  TARGET     = $(TARGETDIR)/td3
  DEFINES   += -DGLEW_STATIC -DDEBUG
  INCLUDES  += -Ilib/glfw/include -Isrc -Icommon -Ilib
  CPPFLAGS  += -MMD -MP $(DEFINES) $(INCLUDES)
  CFLAGS    += $(CPPFLAGS) $(ARCH) -g -m64
  CXXFLAGS  += $(CFLAGS) 
  LDFLAGS   += -m64 -L/usr/lib64 -Lbin/debug
  LIBS      += -lglfw -lglew -lstb_image -lX11 -lXrandr -lrt -lGL -lGLU -lpthread -lalut
  RESFLAGS  += $(DEFINES) $(INCLUDES) 
  LDDEPS    += bin/debug/libglfw.a bin/debug/libglew.a bin/debug/libstb_image.a
  LINKCMD    = $(CXX) -o $(TARGET) $(OBJECTS) $(LDFLAGS) $(RESOURCES) $(ARCH) $(LIBS)
  define PREBUILDCMDS
  endef
  define PRELINKCMDS
  endef
  define POSTBUILDCMDS
  endef
endif

ifeq ($(config),release64)
  OBJDIR     = obj/x64/Release/td3
  TARGETDIR  = bin/release/td3
  TARGET     = $(TARGETDIR)/td3
  DEFINES   += -DGLEW_STATIC -DNDEBUG
  INCLUDES  += -Ilib/glfw/include -Isrc -Icommon -Ilib
  CPPFLAGS  += -MMD -MP $(DEFINES) $(INCLUDES)
  CFLAGS    += $(CPPFLAGS) $(ARCH) -O2 -m64
  CXXFLAGS  += $(CFLAGS) 
  LDFLAGS   += -s -m64 -L/usr/lib64 -Lbin/release
  LIBS      += -lglfw -lglew -lstb_image -lX11 -lXrandr -lrt -lGL -lGLU -lpthread -lalut -lopenal -lassimp3.0
  RESFLAGS  += $(DEFINES) $(INCLUDES) 
  LDDEPS    += bin/release/libglfw.a bin/release/libglew.a bin/release/libstb_image.a
  LINKCMD    = $(CXX) -o $(TARGET) $(OBJECTS) $(LDFLAGS) $(RESOURCES) $(ARCH) $(LIBS)
  define PREBUILDCMDS
  endef
  define PRELINKCMDS
  endef
  define POSTBUILDCMDS
  endef
endif

ifeq ($(config),debug32)
  OBJDIR     = obj/x32/Debug/td3
  TARGETDIR  = bin/debug/td3
  TARGET     = $(TARGETDIR)/td3
  DEFINES   += -DGLEW_STATIC -DDEBUG
  INCLUDES  += -Ilib/glfw/include -Isrc -Icommon -Ilib
  CPPFLAGS  += -MMD -MP $(DEFINES) $(INCLUDES)
  CFLAGS    += $(CPPFLAGS) $(ARCH) -g -m32
  CXXFLAGS  += $(CFLAGS) 
  LDFLAGS   += -m32 -L/usr/lib32 -Lbin/debug
  LIBS      += -lglfw -lglew -lstb_image -lX11 -lXrandr -lrt -lGL -lGLU -lpthread -lalut
  RESFLAGS  += $(DEFINES) $(INCLUDES) 
  LDDEPS    += bin/debug/libglfw.a bin/debug/libglew.a bin/debug/libstb_image.a
  LINKCMD    = $(CXX) -o $(TARGET) $(OBJECTS) $(LDFLAGS) $(RESOURCES) $(ARCH) $(LIBS)
  define PREBUILDCMDS
  endef
  define PRELINKCMDS
  endef
  define POSTBUILDCMDS
  endef
endif

ifeq ($(config),release32)
  OBJDIR     = obj/x32/Release/td3
  TARGETDIR  = bin/release/td3
  TARGET     = $(TARGETDIR)/td3
  DEFINES   += -DGLEW_STATIC -DNDEBUG
  INCLUDES  += -Ilib/glfw/include -Isrc -Icommon -Ilib
  CPPFLAGS  += -MMD -MP $(DEFINES) $(INCLUDES)
  CFLAGS    += $(CPPFLAGS) $(ARCH) -O2 -m32
  CXXFLAGS  += $(CFLAGS) 
  LDFLAGS   += -s -m32 -L/usr/lib32 -Lbin/release
  LIBS      += -lglfw -lglew -lstb_image -lX11 -lXrandr -lrt -lGL -lGLU -lpthread -lalut
  RESFLAGS  += $(DEFINES) $(INCLUDES) 
  LDDEPS    += bin/release/libglfw.a bin/release/libglew.a bin/release/libstb_image.a
  LINKCMD    = $(CXX) -o $(TARGET) $(OBJECTS) $(LDFLAGS) $(RESOURCES) $(ARCH) $(LIBS)
  define PREBUILDCMDS
  endef
  define PRELINKCMDS
  endef
  define POSTBUILDCMDS
  endef
endif

OBJECTS := \
	$(OBJDIR)/3.o \
	$(OBJDIR)/imguiRenderGL.o \
	$(OBJDIR)/Transform.o \
	$(OBJDIR)/imgui.o \
	$(OBJDIR)/Camera.o \
	$(OBJDIR)/LinearAlgebra.o \
	$(OBJDIR)/FramebufferGL.o \
	$(OBJDIR)/ShaderGLSL.o \

RESOURCES := \

SHELLTYPE := msdos
ifeq (,$(ComSpec)$(COMSPEC))
  SHELLTYPE := posix
endif
ifeq (/bin,$(findstring /bin,$(SHELL)))
  SHELLTYPE := posix
endif

.PHONY: clean prebuild prelink

all: $(TARGETDIR) $(OBJDIR) prebuild prelink $(TARGET)
	@:

$(TARGET): $(GCH) $(OBJECTS) $(LDDEPS) $(RESOURCES)
	@echo Linking td3
	$(SILENT) $(LINKCMD)
	$(POSTBUILDCMDS)

$(TARGETDIR):
	@echo Creating $(TARGETDIR)
ifeq (posix,$(SHELLTYPE))
	$(SILENT) mkdir -p $(TARGETDIR)
else
	$(SILENT) mkdir $(subst /,\\,$(TARGETDIR))
endif

$(OBJDIR):
	@echo Creating $(OBJDIR)
ifeq (posix,$(SHELLTYPE))
	$(SILENT) mkdir -p $(OBJDIR)
else
	$(SILENT) mkdir $(subst /,\\,$(OBJDIR))
endif

clean:
	@echo Cleaning td3
ifeq (posix,$(SHELLTYPE))
	$(SILENT) rm -f  $(TARGET)
	$(SILENT) rm -rf $(OBJDIR)
else
	$(SILENT) if exist $(subst /,\\,$(TARGET)) del $(subst /,\\,$(TARGET))
	$(SILENT) if exist $(subst /,\\,$(OBJDIR)) rmdir /s /q $(subst /,\\,$(OBJDIR))
endif

prebuild:
	$(PREBUILDCMDS)

prelink:
	$(PRELINKCMDS)

ifneq (,$(PCH))
$(GCH): $(PCH)
	@echo $(notdir $<)
	-$(SILENT) cp $< $(OBJDIR)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -c "$<"
endif

$(OBJDIR)/3.o: td3/3.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -c "$<"
$(OBJDIR)/imguiRenderGL.o: common/imguiRenderGL.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -c "$<"
$(OBJDIR)/Transform.o: common/Transform.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -c "$<"
$(OBJDIR)/imgui.o: common/imgui.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -c "$<"
$(OBJDIR)/Camera.o: common/Camera.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -c "$<"
$(OBJDIR)/LinearAlgebra.o: common/LinearAlgebra.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -c "$<"
$(OBJDIR)/FramebufferGL.o: common/FramebufferGL.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -c "$<"
$(OBJDIR)/ShaderGLSL.o: common/ShaderGLSL.cpp
	@echo $(notdir $<)
	$(SILENT) $(CXX) $(CXXFLAGS) -o "$@" -c "$<"

-include $(OBJECTS:%.o=%.d)
