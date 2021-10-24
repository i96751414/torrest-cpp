IMAGE_PREFIX = i96751414/torrest-cpp
IMAGE_TAG ?= latest
CROSS_COMPILER_TAG ?= latest
ROOT_DIR = $(abspath $(lastword $(MAKEFILE_LIST))/../..)

PLATFORMS = dev \
	darwin-x64 \
	linux-x64 linux-x86 \
	windows-x64 windows-x86

BUILD_TARGETS = $(addprefix build-, $(PLATFORMS))
PUSH_TARGETS = $(addprefix push-, $(PLATFORMS))
TORREST_TARGETS = $(addprefix torrest-, $(PLATFORMS))
.PHONY: build push torrest clean $(BUILD_TARGETS) $(PUSH_TARGETS) $(TORREST_TARGETS)

build: $(BUILD_TARGETS)

$(BUILD_TARGETS):
	$(eval PLATFORM := $(@:build-%=%))
	docker build \
		--tag $(IMAGE_PREFIX)-$(PLATFORM):$(IMAGE_TAG) \
		--build-arg CROSS_COMPILER_TAG=$(CROSS_COMPILER_TAG) \
		--file ${ROOT_DIR}/docker/$(PLATFORM).Dockerfile \
		${ROOT_DIR}/scripts

push: $(PUSH_TARGETS)

$(PUSH_TARGETS):
	docker push $(IMAGE_PREFIX)-$(@:push-%=%):$(IMAGE_TAG)

torrest: $(TORREST_TARGETS)

$(TORREST_TARGETS):
	$(eval PLATFORM := $(@:torrest-%=%))
	[ "$(PLATFORM)" = dev ] \
		&& OPTS='-DCMAKE_BUILD_TYPE=Debug' \
		|| OPTS='-DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=$${CMAKE_TOOLCHAIN_FILE} -DBUILD_SHARED_LIBS=OFF'; \
	case "$(PLATFORM)" in windows*) EXECUTABLE="torrest.exe" ;; *) EXECUTABLE="torrest" ;; esac; \
	mkdir -p "$(ROOT_DIR)/build/$(PLATFORM)" && \
	docker run --rm -u "$$(id -u):$$(id -g)" -v "$(ROOT_DIR):/build" -w "/build" \
    	$(IMAGE_PREFIX)-$(PLATFORM):$(IMAGE_TAG) bash -c "\
    		cmake -B /tmp/cmake-build $${OPTS} && \
    		cmake --build /tmp/cmake-build --target torrest -- -j '$$(nproc)' && \
    		cp '/tmp/cmake-build/$${EXECUTABLE}' '/build/build/$(PLATFORM)/'"

clean:
	rm -rf "$(ROOT_DIR)/build"
