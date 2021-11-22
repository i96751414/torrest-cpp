IMAGE_PREFIX = i96751414/torrest-cpp
IMAGE_TAG ?= latest
CROSS_COMPILER_TAG ?= latest
ROOT_DIR = $(abspath $(lastword $(MAKEFILE_LIST))/../..)
BUILD_DIR = build
BINARIES_DIR = $(BUILD_DIR)/binaries
VERSION ?= $(shell cut -s -d'"' -f2 "$(ROOT_DIR)/src/version.h")

PLATFORMS = dev \
	android-arm android-arm64 android-x64 android-x86 \
	darwin-x64 \
	linux-arm64 linux-armv7 linux-x64 linux-x86 \
	windows-x64 windows-x86

BUILD_TARGETS = $(addprefix build-, $(PLATFORMS))
PULL_TARGETS = $(addprefix pull-, $(PLATFORMS))
PUSH_TARGETS = $(addprefix push-, $(PLATFORMS))
TORREST_TARGETS = $(addprefix torrest-, $(PLATFORMS))
ZIP_TARGETS = $(addprefix zip-, $(PLATFORMS))
.PHONY: build pull push torrest zip clean \
	$(BUILD_TARGETS) $(PULL_TARGETS) $(PUSH_TARGETS) $(TORREST_TARGETS) $(ZIP_TARGETS)

build: $(BUILD_TARGETS)

$(BUILD_TARGETS):
	$(eval PLATFORM := $(@:build-%=%))
	docker build \
		--tag "$(IMAGE_PREFIX)-$(PLATFORM):$(IMAGE_TAG)" \
		--build-arg CROSS_COMPILER_TAG="$(CROSS_COMPILER_TAG)" \
		--file "${ROOT_DIR}/docker/$(PLATFORM).Dockerfile" \
		"${ROOT_DIR}/scripts"

pull: $(PULL_TARGETS)

$(PULL_TARGETS):
	docker pull "$(IMAGE_PREFIX)-$(@:pull-%=%):$(IMAGE_TAG)"

push: $(PUSH_TARGETS)

$(PUSH_TARGETS):
	docker push "$(IMAGE_PREFIX)-$(@:push-%=%):$(IMAGE_TAG)"

torrest: $(TORREST_TARGETS)

$(TORREST_TARGETS):
	$(eval PLATFORM := $(@:torrest-%=%))
	[ "$(PLATFORM)" = dev ] \
		&& OPTS='-DCMAKE_BUILD_TYPE=Debug' \
		|| OPTS='-DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=$${CMAKE_TOOLCHAIN_FILE} -DBUILD_SHARED_LIBS=OFF'; \
	case "$(PLATFORM)" in \
		windows*) OPTS="$${OPTS} -DCMAKE_EXE_LINKER_FLAGS='-static-libgcc -static-libstdc++ -static'" ;; \
	esac; \
	mkdir -p "$(ROOT_DIR)/$(BUILD_DIR)/$(PLATFORM)" && \
	docker run --rm -u "$$(id -u):$$(id -g)" -v "$(ROOT_DIR):/workspace" -w "/workspace" \
    	"$(IMAGE_PREFIX)-$(PLATFORM):$(IMAGE_TAG)" bash -c "\
    		cmake -B /tmp/cmake-build $${OPTS} -Dtarget_path='/workspace/$(BUILD_DIR)/$(PLATFORM)/' && \
    		cmake --build /tmp/cmake-build --target torrest -- -j '$$(nproc)'"

zip: $(ZIP_TARGETS)

$(ZIP_TARGETS):
	$(eval PLATFORM := $(@:zip-%=%))
	mkdir -p "$(ROOT_DIR)/$(BINARIES_DIR)" && \
	cd "$(ROOT_DIR)/$(BUILD_DIR)/$(PLATFORM)" && \
	zip -9 -r "$(ROOT_DIR)/$(BINARIES_DIR)/torrest.$(VERSION).$(subst -,_,$(PLATFORM)).zip" .

clean:
	rm -rf "$(ROOT_DIR)/$(BUILD_DIR)"
