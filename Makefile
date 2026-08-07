SANITIZE ?=
CMAKE_FLAGS =

ifneq ($(SANITIZE),)
  CMAKE_FLAGS += -DCMAKE_CXX_FLAGS="-fsanitize=$(SANITIZE) -fno-omit-frame-pointer" \
                 -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=$(SANITIZE)"
endif

.PHONY: all build test clean stress

all: build test

build:
	cmake -B build -S . $(CMAKE_FLAGS)
	cmake --build build

test: build
	cd build && ctest --output-on-failure

stress:
	$(MAKE) clean
	$(MAKE) test SANITIZE=address,undefined

clean:
	rm -rf build tree.txt