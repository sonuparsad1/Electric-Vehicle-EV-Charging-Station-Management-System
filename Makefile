APP=EVChargingApp.exe
SRC=app/src/main.c app/src/gui.c app/src/logic.c app/src/storage.c app/src/billing.c
INC=-Iapp/include

all:
	@echo "Use Windows MinGW to build GUI app:"
	@echo "gcc $(SRC) $(INC) -o build/$(APP) -mwindows -lcomctl32"

windows-build:
	mkdir -p build
	gcc $(SRC) $(INC) -o build/$(APP) -mwindows -lcomctl32

windows-build-fallback:
	mkdir -p build
	gcc $(SRC) $(INC) -o build/$(APP) -mwindows

clean:
	rm -rf build dist

.PHONY: all windows-build windows-build-fallback clean
