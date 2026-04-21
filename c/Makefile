CC = cc

CFLAGS_DEBUG = -Wall -Wextra -g -O0 -DDEBUG
DEPS_CFLAGS_DEBUG = -g -O0 -DDEBUG

CFLAGS_PROD = -Wall -Wextra -Wpedantic -Werror=return-type -Werror=address \
              -O2 -DNDEBUG -fstack-protector-strong -fno-omit-frame-pointer
DEPS_CFLAGS_PROD = -O2 -DNDEBUG
LDFLAGS_PROD_LINUX = -Wl,-z,relro,-z,now -Wl,--gc-sections
LDFLAGS_PROD_MACOS =

CFLAGS_FAST = -Wall -Wextra \
              -O3 -march=native -mtune=native -DNDEBUG -pipe \
              -fstack-protector -ftree-vectorize -funroll-loops
DEPS_CFLAGS_FAST = -O3 -march=native -DNDEBUG
LDFLAGS_FAST = -flto=thin
LDFLAGS_FAST_LINUX = -Wl,-O1 -Wl,--gc-sections
LDFLAGS_FAST_MACOS = -Wl,-dead_strip

CFLAGS_UNSAFE = -Wall \
                -Ofast -march=native -DNDEBUG -pipe \
                -fno-stack-protector -fomit-frame-pointer -ffast-math \
                -funroll-loops -fvisibility=hidden -fno-plt \
                -fno-common -fstrict-overflow -fno-trapping-math
DEPS_CFLAGS_UNSAFE = -Ofast -march=native -DNDEBUG
LDFLAGS_UNSAFE = -flto=full
LDFLAGS_UNSAFE_LINUX = -Wl,-O3 -Wl,--strip-all
LDFLAGS_UNSAFE_MACOS = -Wl,-dead_strip -Wl,-S

CFLAGS_COMMON = -I./src -Ideps/cJSON -Ideps/base64/include -Ideps/yyjson/src -I/opt/homebrew/include -std=c99 -pthread
LDFLAGS_BASE = -lm -pthread -lpcre2-8 -L/opt/homebrew/lib

MODE ?= debug
ARCH = $(shell uname -m)
UNAME_S = $(shell uname -s)

ifeq ($(UNAME_S),Linux)
    LDFLAGS_OS_PROD = $(LDFLAGS_PROD_LINUX)
    LDFLAGS_OS_FAST = $(LDFLAGS_FAST_LINUX)
    LDFLAGS_OS_UNSAFE = $(LDFLAGS_UNSAFE_LINUX)
else ifeq ($(UNAME_S),Darwin)
    LDFLAGS_OS_PROD = $(LDFLAGS_PROD_MACOS)
    LDFLAGS_OS_FAST = $(LDFLAGS_FAST_MACOS)
    LDFLAGS_OS_UNSAFE = $(LDFLAGS_UNSAFE_MACOS)
else
    LDFLAGS_OS_PROD =
    LDFLAGS_OS_FAST =
    LDFLAGS_OS_UNSAFE =
endif

ifeq ($(MODE), prod)
    CFLAGS_MODE = $(CFLAGS_PROD)
    LDFLAGS_OS = $(LDFLAGS_OS_PROD)
    LDFLAGS_EXTRA =
    BUILD_DIR = target/prod
    DEPS_DIR = target/deps/prod
    DEPS_CFLAGS = $(DEPS_CFLAGS_PROD)
else ifeq ($(MODE), fast)
    CFLAGS_MODE = $(CFLAGS_FAST)
    LDFLAGS_OS = $(LDFLAGS_OS_FAST)
    LDFLAGS_EXTRA = $(LDFLAGS_FAST)
    BUILD_DIR = target/fast
    DEPS_DIR = target/deps/fast
    DEPS_CFLAGS = $(DEPS_CFLAGS_FAST)
else ifeq ($(MODE), unsafe)
    CFLAGS_MODE = $(CFLAGS_UNSAFE)
    LDFLAGS_OS = $(LDFLAGS_OS_UNSAFE)
    LDFLAGS_EXTRA = $(LDFLAGS_UNSAFE)
    BUILD_DIR = target/unsafe
    DEPS_DIR = target/deps/unsafe
    DEPS_CFLAGS = $(DEPS_CFLAGS_UNSAFE)
else ifeq ($(MODE), release)
    CFLAGS_MODE = $(CFLAGS_PROD)
    LDFLAGS_OS = $(LDFLAGS_OS_PROD)
    LDFLAGS_EXTRA =
    BUILD_DIR = target/prod
    DEPS_DIR = target/deps/prod
    DEPS_CFLAGS = $(DEPS_CFLAGS_PROD)
else
    CFLAGS_MODE = $(CFLAGS_DEBUG)
    LDFLAGS_OS =
    LDFLAGS_EXTRA =
    BUILD_DIR = target/debug
    DEPS_DIR = target/deps/debug
    DEPS_CFLAGS = $(DEPS_CFLAGS_DEBUG)
endif

CFLAGS = $(CFLAGS_COMMON) $(CFLAGS_MODE)
LDFLAGS = $(LDFLAGS_BASE) $(LDFLAGS_OS) $(LDFLAGS_EXTRA)

SRC_DIR = src
SRCS = $(wildcard $(SRC_DIR)/*.c)
SRCS += main.c

MAIN_OBJ = $(BUILD_DIR)/main.o
OBJS = $(patsubst %.c, $(BUILD_DIR)/%.o, $(filter-out main.c, $(SRCS)))

DEPS_OBJS = $(DEPS_DIR)/cJSON.o $(DEPS_DIR)/libbase64.o $(DEPS_DIR)/yyjson.o

TARGET = $(BUILD_DIR)/benchmark

all: $(TARGET)

deps/cJSON:
	@echo "Cloning cJSON..."
	git clone https://github.com/DaveGamble/cJSON.git $@

deps/base64:
	@echo "Cloning base64..."
	git clone https://github.com/aklomp/base64.git $@

deps/yyjson:
	@echo "Cloning yyjson..."
	git clone https://github.com/ibireme/yyjson.git $@

deps: deps/cJSON deps/base64 deps/yyjson

$(DEPS_DIR):
	mkdir -p $@

$(DEPS_DIR)/cJSON.o: deps/cJSON | $(DEPS_DIR)
	@echo "Building cJSON with $(DEPS_CFLAGS)..."
	cd deps/cJSON && $(CC) $(DEPS_CFLAGS) -c cJSON.c && cd -
	cp deps/cJSON/cJSON.o $@

$(DEPS_DIR)/libbase64.o: deps/base64 | $(DEPS_DIR)
	@echo "Building base64 with $(DEPS_CFLAGS)..."
	cd deps/base64 && \
	make clean && \
	if [ "$(ARCH)" = "x86_64" ]; then \
		echo "Building for x86_64 with AVX2..."; \
		AVX2_CFLAGS=-mavx2 SSSE3_CFLAGS=-mssse3 SSE41_CFLAGS=-msse4.1 SSE42_CFLAGS=-msse4.2 AVX_CFLAGS=-mavx \
		CFLAGS="$(DEPS_CFLAGS)" make -i lib/libbase64.o; \
	elif [ "$(ARCH)" = "arm64" ] || [ "$(ARCH)" = "aarch64" ]; then \
		echo "Building for ARM64 with NEON..."; \
		CFLAGS="-arch arm64 $(DEPS_CFLAGS)" NEON64_CFLAGS=" " make -i lib/libbase64.o; \
	else \
		echo "Building generic for $(ARCH)..."; \
		CFLAGS="$(DEPS_CFLAGS)" make -i lib/libbase64.o; \
	fi && \
	cd - && \
	cp deps/base64/lib/libbase64.o $@

$(DEPS_DIR)/yyjson.o: deps/yyjson | $(DEPS_DIR)
	@echo "Building yyjson with $(DEPS_CFLAGS)..."
	cd deps/yyjson/src && $(CC) $(DEPS_CFLAGS) -c yyjson.c && cd -
	cp deps/yyjson/src/yyjson.o $@

$(BUILD_DIR) $(BUILD_DIR)/$(SRC_DIR):
	mkdir -p $@

$(BUILD_DIR)/main.o: main.c | $(BUILD_DIR) deps
	@echo "Compiling main.c..."
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(SRC_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)/$(SRC_DIR) deps
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS) $(MAIN_OBJ) $(DEPS_OBJS)
	@echo "Linking $(TARGET)..."
	$(CC) $^ -o $@ $(LDFLAGS)

# Очистка
clean:
	rm -rf target
	cd deps/base64 && make clean 2>/dev/null || true

clean-deps:
	rm -rf target/deps
	cd deps/base64 && make clean 2>/dev/null || true

distclean: clean
	rm -rf deps

clean-debug:
	rm -rf target/debug target/deps/debug

clean-prod:
	rm -rf target/prod target/deps/prod

clean-fast:
	rm -rf target/fast target/deps/fast

clean-unsafe:
	rm -rf target/unsafe target/deps/unsafe

run: $(TARGET)
	./$(TARGET)

debug:
	$(MAKE) MODE=debug

prod:
	$(MAKE) MODE=prod

fast:
	$(MAKE) MODE=fast

unsafe:
	$(MAKE) MODE=unsafe

# Совместимость
release: prod

info:
	@echo "Mode:        $(MODE)"
	@echo "OS:          $(UNAME_S)"
	@echo "Arch:        $(ARCH)"
	@echo "Build dir:   $(BUILD_DIR)"
	@echo "Deps dir:    $(DEPS_DIR)"
	@echo "Target:      $(TARGET)"
	@echo "CFLAGS:      $(CFLAGS)"
	@echo "Deps CFLAGS: $(DEPS_CFLAGS)"
	@echo "LDFLAGS:     $(LDFLAGS)"
	@echo "Sources:     $(notdir $(SRCS))"

.PHONY: all deps clean clean-deps distclean run info
.PHONY: debug prod fast unsafe release
.PHONY: clean-debug clean-prod clean-fast clean-unsafe