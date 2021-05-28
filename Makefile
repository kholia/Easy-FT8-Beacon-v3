# https://github.com/arduino/arduino-cli/releases

port := $(shell python3 board_detect.py)

default:
	arduino-cli compile --fqbn=arduino:avr:nano:cpu=atmega328old Easy-FT8-Beacon-v3

upload:
	@# echo $(port)
	arduino-cli compile --fqbn=arduino:avr:nano:cpu=atmega328old Easy-FT8-Beacon-v3
	arduino-cli -v upload -p "${port}" --fqbn=arduino:avr:nano:cpu=atmega328old Easy-FT8-Beacon-v3

install_platform:
	arduino-cli core install arduino:avr

deps:
	arduino-cli lib install "Etherkit JTEncode"
	arduino-cli lib install "Etherkit Si5351"
	# arduino-cli lib install "Adafruit Si5351 Library"
	arduino-cli lib install "RTClib"
	arduino-cli lib install "Time"


install_arduino_cli:
	curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=~/.local/bin sh
