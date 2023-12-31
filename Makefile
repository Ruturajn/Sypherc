# @brief:  Makefile for building the compiler.
# @author: Ruturaj A. Nanoti

CC=gcc
CFLAGS=-g -Wall -Werror -Wextra -pedantic
TARGET=sypherc
INCS=-I ../inc -I ../inc/arch -I ../inc/arch/x86_64

#==============================================================================

BUILD_DIR=build
BIN_DIR=bin
DOXYGEN_DIR=doxygen
FILE_PATH=./examples/simple.sy

#==============================================================================

SRCS = $(wildcard src/*.c src/arch/*.c src/arch/x86_64/*.c)
OBJS = $(addprefix ./$(BUILD_DIR)/,$(notdir $(SRCS:.c=.o)))

#==============================================================================

.PHONY: all clean build_and_bin_dir run doxygen help clean_doxygen test

all: build_and_bin_dir $(TARGET)

build_and_bin_dir:
	@printf "\033[1;35m[+] Creating build and bin directory ...\033[1;37m\n"
	mkdir -p $(BUILD_DIR) $(BIN_DIR)

compile_msg:
	@printf "\033[1;34m[+] Compiling into object files ...\033[1;37m\n"

$(TARGET):
	@$(MAKE) -C ./src
	@$(MAKE) -C ./src/arch
	@$(MAKE) -C ./src/arch/x86_64
	@printf "\033[1;32m[+] Linking into executable ...\033[1;37m\n"
	$(CC) $(CFLAGS) $(OBJS) $(INCS) -o ./$(BIN_DIR)/$(TARGET)
	@printf "\033[1;36m[+] DONE\033[1;37m\n"

clean:
	@printf "\033[1;33m[+] Cleaning generated build files ...\033[1;37m\n"
	rm -rf $(BUILD_DIR) $(BIN_DIR)

run: clean all
	@printf "\033[1;33m[+] Running the executable ...\033[1;37m\n"
	./$(BIN_DIR)/$(TARGET) $(FILE_PATH) -V

test:
	@printf "\033[1;33m[+] Running the executable ...\033[1;37m\n"
	./$(BIN_DIR)/$(TARGET) $(FILE_PATH)

doxygen:
	@printf "\033[1;34m[+] Re-creating docs directory ...\033[1;37m\n"
	rm -rf $(DOXYGEN_DIR)
	mkdir -p $(DOXYGEN_DIR)
	@printf "\033[1;34m[+] Generating Documentation ...\033[1;37m\n"
	@doxygen Doxyfile
	@$(MAKE) -C ./$(DOXYGEN_DIR)/latex/

clean_doxygen:
	@printf "\033[1;33m[+] Cleaning generated docs ...\033[1;37m\n"
	rm -rf $(DOXYGEN_DIR)

help:
	@printf "\n"
	@printf "\033[1;33mUSAGE:\033[1;37m\n"
	@printf "    make <TAGRET>\n"
	@printf "\n"
	@printf "\033[1;33mTARGETS:\033[1;37m\n"
	@printf "    \033[1;35mall\033[1;37m           - Complete build (Default Target).\n"
	@printf "    \033[1;35mclean\033[1;37m         - Remove build files and directories.\n"
	@printf "    \033[1;35mclean_doxygen\033[1;37m - Remove build files and directories.\n"
	@printf "    \033[1;35mrun\033[1;37m           - Executes all, clean and runs the executable.\n"
	@printf "                    Optionally provide 'FILE_PATH' for running\n"
	@printf "                    the executable.\n"
	@printf "    \033[1;35mdoxygen\033[1;37m       - Generates documentation, using doxygen.\n"
	@printf "    \033[1;35mhelp\033[1;37m          - Prints out this help menu.\n"
