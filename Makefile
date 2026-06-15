PLUGIN_NAME := plain_eq
PROJECT := Builds/MacOSX/$(PLUGIN_NAME).xcodeproj
CONFIG ?= Debug

SCHEME_VST3 := $(PLUGIN_NAME) - VST3
SCHEME_STANDALONE := $(PLUGIN_NAME) - Standalone Plugin
SCHEME_ALL := $(PLUGIN_NAME) - All

.PHONY: help list vst3 standalone run all clean

help:
	@echo "Available commands:"
	@echo "  make list          - List Xcode targets/schemes"
	@echo "  make vst3          - Build VST3"
	@echo "  make standalone    - Build standalone app"
	@echo "  make run           - Build and run standalone app"
	@echo "  make all           - Build all plugin formats"
	@echo "  make clean         - Clean build"
	@echo ""
	@echo "Use CONFIG=Release for release builds:"
	@echo "  make vst3 CONFIG=Release"

list:
	xcodebuild -list -project "$(PROJECT)"

vst3:
	xcodebuild \
		-project "$(PROJECT)" \
		-scheme "$(SCHEME_VST3)" \
		-configuration "$(CONFIG)" \
		build

standalone:
	xcodebuild \
		-project "$(PROJECT)" \
		-scheme "$(SCHEME_STANDALONE)" \
		-configuration "$(CONFIG)" \
		build

run: standalone
	@APP_PATH=$$(xcodebuild \
		-project "$(PROJECT)" \
		-scheme "$(SCHEME_STANDALONE)" \
		-configuration "$(CONFIG)" \
		-showBuildSettings | \
		awk -F'= ' '/TARGET_BUILD_DIR/ { dir=$$2 } /FULL_PRODUCT_NAME/ { name=$$2 } END { print dir "/" name }'); \
	echo "Opening: $$APP_PATH"; \
	open "$$APP_PATH"

all:
	xcodebuild \
		-project "$(PROJECT)" \
		-scheme "$(SCHEME_ALL)" \
		-configuration "$(CONFIG)" \
		build

clean:
	xcodebuild \
		-project "$(PROJECT)" \
		-scheme "$(SCHEME_ALL)" \
		-configuration "$(CONFIG)" \
		clean