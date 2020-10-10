FQBN = esp8266:esp8266:nodemcuv2
BUILD_DIR = build/esp8266.esp8266.nodemcuv2
SKETCH_NAME = neopixel

all: $(SKETCH_NAME)

$(SKETCH_NAME): $(BUILD_DIR)/$(SKETCH_NAME).ino.bin

$(BUILD_DIR)/$(SKETCH_NAME).ino.bin: $(SKETCH_NAME).ino
	arduino-cli compile --fqbn $(FQBN) $(SKETCH_NAME)

# Since files are concatenated behind the scenes, $(SKETCH_NAME).ino
# should in effect be considered updated if any other .ino file is
$(SKETCH_NAME).ino: html.ino
	touch $(SKETCH_NAME).ino

html.ino: html.ino.in index.html
	sed '/^const char INDEX_HTML/r index.html' html.ino.in > html.ino

upload: $(SKETCH_NAME)
	arduino-cli upload --port /dev/ttyUSB0 --fqbn $(FQBN) $(SKETCH_NAME)

clean:
	rm -rf build html.ino
