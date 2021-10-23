IMAGE_PREFIX = i96751414/torrest-cpp
IMAGE_TAG ?= latest
CROSS_COMPILER_TAG ?= latest
ROOT_DIR = $(abspath $(lastword $(MAKEFILE_LIST))/../..)

PLATFORMS = dev \
	linux-x64 linux-x86 \
	windows-x64 windows-x86

BUILD_TARGETS = $(addprefix build-, $(PLATFORMS))
PUSH_TARGETS = $(addprefix push-, $(PLATFORMS))
.PHONY: build push $(BUILD_TARGETS) $(PUSH_TARGETS)

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
