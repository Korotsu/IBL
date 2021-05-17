
OUTPUT=ibl-paul.dll

CFLAGS=-O3
CXXFLAGS=$(CFLAGS)
CPPFLAGS=-Ithird_party/include -Isrc -MMD

OBJS=src-priv/demo_paul.o src-priv/prototypes_paul.o
OBJS+=src/gl_helpers.o
OBJS+=third_party/src/glad.o third_party/src/stb_perlin.o third_party/src/stb_image.o third_party/src/imgui.o third_party/src/imgui_demo.o third_party/src/imgui_draw.o third_party/src/imgui_tables.o third_party/src/imgui_widgets.o

DEPS=$(OBJS:.o=.d)

all: $(OUTPUT)

-include $(DEPS)

$(OUTPUT): $(OBJS)
	$(CXX) -shared $^ -o $@

clean:
	rm -f $(OBJS) $(DEPS) $(OUTPUT) *.dll

copy_dll:
	ldd $(OUTPUT) | grep mingw | cut -d " " -f 3 | xargs -I{} cp {} .