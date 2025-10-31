#                                                                  
#    ███    ███  █████  ██   ██ ███████ ███████ ██ ██      ███████ 
#    ████  ████ ██   ██ ██  ██  ██      ██      ██ ██      ██      
#    ██ ████ ██ ███████ █████   █████   █████   ██ ██      █████   
#    ██  ██  ██ ██   ██ ██  ██  ██      ██      ██ ██      ██      
#    ██      ██ ██   ██ ██   ██ ███████ ██      ██ ███████ ███████ 
#                                                                  

#if CONFIG is not defined it will be set to debug
CONFIG?=debug
MAIN?=src/main

#(uname -s is used to get the system kernel)
#(windows does not support uname command so the resulting error is redirected to dump.txt file)
#(uname -s) on linux will print Linux
#(uname -s) on macOS will print Darwin
#(descriptors 0 -> stdin, 1 -> stdout, 2 -> stderr)
#('> file' refers move stdout output to file)
#('2> file' refers move stderr output to file)
#(win'2>NUL'/ linux'2>/dev/null' refers move stderr output to null or no file)
#(win'>NUL 2>&1'/ linux'>/dev/null 2>&1' refers move stdout output to null and move stderr output to stdout which moves to null, this silences both stdout and stderr)
#(when we move output to null or other file the descriptor form which it is moved from will have null as output means nothing will be printed)

#check the operating system
ifeq ($(OS),Windows_NT)
	SHELL=cmd.exe
	PLATFORM=windows
	CHECK_CLANG=$(shell where clang 2>NUL)
	CHECK_C_HEADERS=$(shell (echo \#include^<stdio.h^> && echo int main^(^){return 0;})>temp_check.c\
					&& clang temp_check.c -o temp_check.out 2>NUL \
					&& echo TRUE || echo FALSE \
					&& del /Q temp_check.c temp_check.out 2>NUL)
	REMOVE_DIR=@if exist "$(1)" rmdir /Q /S "$(1)" #(here /Q -> quite mode (dont ask for yes or no) /S -> subdirectories recursive delete)
	REMOVE_FILE=@if exist "$(1)" del /Q "$(1)"
	MAKE_DIR=@if not exist "$(1)" mkdir "$(1)"
	TARGET_EXTENSION=.exe
	LIBS=
	ifeq ($(PROCESSOR_ARCHITEW6432),AMD64)
		ARCH=-m64
	else ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
		ARCH=-m64
	else ifeq ($(PROCESSOR_ARCHITECTURE),x86)
		ARCH=-m32
	else
		$(error machine arch not supported)
	endif
else ifeq ($(shell uname -s),Linux)
		PLATFORM=linux
		CHECK_CLANG=$(shell which clang 2>/dev/null)
		CHECK_C_HEADERS=$(shell printf "\#include<stdio.h>\\nint main(){return 0;}">temp_check.c\
						&& clang temp_check.c -o temp_check.out 2>/dev/null \
						&& echo TRUE || echo FALSE \
						&& rm -f temp_check.c temp_check.out 2>/dev/null)
		REMOVE_DIR=rm -fr "$(1)" #(here -r is recursive remove and -f is force ignore non existing)
		REMOVE_FILE=rm -f "$(1)" 
		MAKE_DIR=mkdir -p "$(1)"
		TARGET_EXTENSION=
		MACHINE_ARCH=$(shell uname -m)
		LIBS=-lrt
		ifeq ($(MACHINE_ARCH),x86_64)
			ARCH=-m64
		else ifeq ($(MACHINE_ARCH),i386)
			ARCH=-m32
		else
			$(error machine arch not supported)
		endif
else
	$(error platform not supported)
endif

#check for clang compiler
ifeq ($(CHECK_CLANG),)
	$(error clang is not installed (or) if installed environment variable 'PATH' must contain directory \
	 path of executable clang file)
endif

#check for c headers
ifeq ($(CHECK_C_HEADERS),)
	$(error system does not have required c headers \
		and run command "clang -xc -E -v -" to see how clang will search for headers by default \
		(on windows install visual stdio build tools for c/c++ development))
endif

#format $(call GET_FILES,dirs,%file_extension)
GET_FILES=$(foreach DIR,$(1),\
				$(foreach ITEM,$(wildcard $(DIR)/*),\
					$(if $(wildcard $(ITEM)/*),$(call GET_FILES,$(ITEM),$(2)),\
						$(if $(filter $(2),$(ITEM)),$(ITEM))\
					)\
				)\
			)


BIN=build/bin
BIN_INT=build/bin-int/$(PLATFORM)/$(CONFIG)
SOURCE_DIRS=src/core src/$(MAIN)

CFILES=$(strip $(call GET_FILES,$(SOURCE_DIRS),%.c))
ifeq ($(CFILES),)
	$(error no C source files found in $(SOURCE_DIRS))
endif
OFILES=$(patsubst %.c,$(BIN_INT)/%.o,$(CFILES))
DFILES=$(patsubst %.c,$(BIN_INT)/%.d,$(CFILES))
HFILES=$(strip $(call GET_FILES,$(SOURCE_DIRS),%.h))
INCLUDE_FLAGS=$(sort $(foreach K,$(HFILES),-I$(dir $(K))))


#debug flags
ifeq ($(CONFIG),debug)
	CONFIG_FLAGS=-O0 -g -DDEBUG
else ifeq ($(CONFIG),release)
	CONFIG_FLAGS=-O3 -march=native -ffast-math -DNDEBUG
else
	$(error UNKNOWN CONFIG : $(CONFIG))
endif

#compiler
CC=clang

#c_version
C_VERSION=-std=gnu99
#using gnu99 instead of c99 to access gnu __builtin functions

#compiler_flags
CFLAGS=-xc -c -Wall -Wextra -Werror -MMD -MP -Wno-unused-parameter $(ARCH) $(C_VERSION) $(CONFIG_FLAGS) $(INCLUDE_FLAGS)
#-Wall -> enable all warnings 
#-Wextra -> enable extra warnings 
#-Werrors -> treat warnings as errors
#-MP -> create phony target for each dependency (other than main file)
#-MMD -> write a depfile containing user headers
#-xc -> treat input as C , ignore file extensions
#-Wno-unused-parameter -> dont genereate warnings for unused function parameters

#linker flags
LFLAGS=$(ARCH) $(C_VERSION) $(CONFIG_FLAGS)

#executable target file
TARGET=$(BIN)/pbrt$(TARGET_EXTENSION)



.PHONY:all create_dirs clean info

all:$(TARGET)
	@echo "running..."
	@$(TARGET)

$(TARGET):create_dirs $(OFILES)
	@echo "linking..."
	@$(CC) $(LFLAGS) $(LIBS) $(OFILES) -o $(TARGET)

$(BIN_INT)/%.o:%.c
	@$(call MAKE_DIR,$(dir $@))
	@$(CC) $(CFLAGS) $< -o $@
	@echo "compiled $(basename $(notdir $<))"

create_dirs:
	@$(call MAKE_DIR,$(BIN))
	@$(call MAKE_DIR,$(BIN_INT))

-include $(DFILES)

clean:
	$(call REMOVE_DIR,build)
	$(call REMOVE_DIR,.cache)
	$(call REMOVE_DIR,.vscode)
	$(call REMOVE_FILE,compile_commands.json)

info:
	@echo "PLATFORM=$(PLATFORM)"
	@echo "ARCH=$(ARCH)"
	@echo "CONFIG=$(CONFIG)"
	@echo "BIN=$(BIN)"
	@echo "BIN_INT=$(BIN_INT)"
	@echo "CFILES=$(CFILES)"
	@echo "DFILES=$(DFILES)"
	@echo "OFILES=$(OFILES)"
	@echo "HFILES=$(HFILES)"
	@echo "CC=$(CC)"
	@echo "C_VERSION=$(C_VERSION)"
	@echo "CFLAGS=$(CFLAGS)"
	@echo "LFLAGS=$(LFLAGS)"
	@echo "TARGET=$(TARGET)"
