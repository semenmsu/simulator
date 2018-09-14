MKDIR_P := mkdir -p
OUT_DIR := build

.PHONY: clean

all: clean directories main

directories: $(OUT_DIR)

$(OUT_DIR):
	$(MKDIR_P) $(OUT_DIR)
	
main:
	g++ -o build/main main.cpp

clean:
	-rm main