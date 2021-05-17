
OUTPUT=ibl

THIRD_PARTY_OBJS=\
	third_party/src/glad.o \
	third_party/src/stb_perlin.o \
	third_party/src/stb_image.o \
	third_party/src/tiny_obj_loader.o \
	third_party/src/imgui.o \
	third_party/src/imgui_demo.o \
	third_party/src/imgui_draw.o \
	third_party/src/imgui_tables.o \
	third_party/src/imgui_widgets.o \
	third_party/src/imgui_impl_glfw.o \
	third_party/src/imgui_impl_opengl3.o

USER_OBJS+=\
	src/camera.o \
	src/data.o \
	src/demo_fbo.o \
	src/demo_mipmap.o \
	src/demo_quad.o \
	src/demo_texture_3d.o \
	src/gl_helpers.o \
	src/main.o \
	src/mesh_builder.o

OBJS=$(THIRD_PARTY_OBJS) $(USER_OBJS)

TARGET?=$(shell $(CC) -dumpmachine)
CFLAGS=-O0 -g
CXXFLAGS=$(CFLAGS)
CPPFLAGS=-Ithird_party/include -MMD

$(USER_OBJS): CXXFLAGS+=-Wall

ifeq ($(TARGET), x86_64-w64-mingw32)
LDFLAGS=-Lthird_party/libs-$(TARGET)
LDLIBS=-lglfw3 -lgdi32
else
# Probably linux
LDLIBS=-lglfw -ldl
endif

DEPS=$(OBJS:.o=.d)

all: $(OUTPUT)

-include $(DEPS)

$(OUTPUT): $(OBJS)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

clean:
	rm -f $(OBJS) $(DEPS) $(OUTPUT)

copy_dll:
	ldd $(OUTPUT) | grep mingw | cut -d " " -f 3 | xargs -I{} cp {} .
