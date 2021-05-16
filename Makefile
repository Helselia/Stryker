CC := clang
RSRC := $(wildcard c/*.c)
SOURCES := $(patsubst c/%.c,build/%.o,$(RSRC))
CSRC := $(patsubst c/%.c,build/%.o,$(RSRC))
SO := libtoku_c.so

$(SO): build $(SOURCES)
	$(CC) $(CSRC) -o $(SO)

build/%.o: c/%.c
	$(CC) $< -c -o $@

build:
	@mkdir -p build

clean:
	@find . -name '*.o' -exec rm -rf {} \;
	@rm -rf $(SO)
	@rm -rf build
