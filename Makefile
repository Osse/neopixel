FQBN = esp8266:esp8266:nodemcuv2
BUILD_DIR = build/esp8266.esp8266.nodemcuv2
SKETCH_NAME = $(notdir $(PWD))
SERIAL ?= /dev/ttyUSB0

all: $(SKETCH_NAME)

$(SKETCH_NAME): $(BUILD_DIR)/$(SKETCH_NAME).ino.bin

$(BUILD_DIR)/$(SKETCH_NAME).ino.bin: $(SKETCH_NAME).ino
	arduino-cli compile --fqbn $(FQBN) $(SKETCH_NAME)

# Since files are concatenated behind the scenes, $(SKETCH_NAME).ino
# should in effect be considered updated if any other .ino file is
$(SKETCH_NAME).ino: html.ino
	touch $(SKETCH_NAME).ino

html.ino: html.ino.in index.html style.css script.js
	sed -e '/^const char INDEX_HTML/r index.html' \
	    -e '/^const char STYLE_CSS/r style.css'   \
	    -e '/^const char SCRIPT_JS/r script.js'   \
	    html.ino.in > html.ino

.PHONY: clean upload

upload: $(SKETCH_NAME)
	arduino-cli upload --port $(SERIAL) --fqbn $(FQBN) $(SKETCH_NAME)

clean:
	rm -rf build html.ino
