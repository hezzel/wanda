# Variable declarations
CXX = g++
SYS := $(shell gcc -dumpmachine)
BUILD_DIR := ./build
BIN_DIR := ./bin
SRC_DIRS := ./src
TARGET_EXEC := wanda.exe
CONVERTER_EXEC := converter.exe
SAT_SOLVER_REPO := https://github.com/deividrvale/minisat.git
NATT_SRC_ZIP := NaTT.2.3.tar.gz

# All sources compose Wanda, execept 'converter.cpp' which is a secondary utility.
SRCS := $(shell find $(SRC_DIRS) -name '*.cpp')

# List of object files based on the list of source files.
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

# List of dependency files from objects.
DEPS := $(OBJS:.o=.d)

# Giving every folder in .src to gcc so it can solve dependency automatically.
INC_DIRS := $(shell find $(SRC_DIRS) -type d)
# Add a prefix to INC_DIRS.
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

# The -MMD and -MP flags together generate Makefiles for us!
# These files will have .d instead of .o as the output.
CPPFLAGS := $(INC_FLAGS) -MMD -MP

all: $(OBJS)
	@echo "Build scheme set to " $(SYS).
	@$(MAKE) $(BIN_DIR)/$(TARGET_EXEC)
	@$(MAKE) $(BIN_DIR)/$(CONVERTER_EXEC)


# ################################################################################
# # In order to compile Wanda on macOS a.k.a Darwin Systems,
# # dynamic linking is necessary.
# # Static linking will not work on Mac OS X unless all libraries
# # (including libgcc.a) have also been compiled with -static.
# # Since neither a static version of libSystem.dylib nor crt0.o are provided,
# # this option will not be supported on macs.
# # Below, there is an updated version of the linking for macs and non-macs.
# # The linking remain static on other systems.
# # Below we check if the system is darwin, which will use dynamic linking,
# # or any other system (Tested on Linux and Windows/WinGW) which will use static.
# ################################################################################

ifneq (, $(findstring darwin, $(SYS)))
WANDA_OBJS := $(shell find $(BUILD_DIR) -name '*.cpp.o' ! -name 'converter.cpp.o')
# Build Wanda Executable macOS.
$(BIN_DIR)/$(TARGET_EXEC): install_resources build_minisat build_natt $(OBJS) $(WANDA_OBJS)
	@echo "Building Wanda macOs Executable..."
	@$(CXX) $(WANDA_OBJS) -o $@ $(LDFLAGS)
else ifneq (, $(findstring linux, $(SYS)))
# Build Wanda Executable Linux.
WANDA_OBJS := $(shell find $(BUILD_DIR) -name '*.cpp.o' ! -name 'converter.cpp.o')
$(BIN_DIR)/$(TARGET_EXEC): install_resources $(OBJS) $(WANDA_OBJS)
	@echo "Building Wanda Linux Executable..."
	@mkdir -p $(dir $@)
	@$(CXX) -static -o $@ $(WANDA_OBJS) $(LDFLAGS)
else
# other platforms will be added later
endif

# Build Converter Executable
CONVERTER_OBJS := $(shell find $(BUILD_DIR) -name '*.cpp.o' ! -name 'wanda.cpp.o')
$(BIN_DIR)/$(CONVERTER_EXEC): $(CONVERTER_OBJS)
	@echo "Building Converter Utility Executable..."
	@mkdir -p $(dir $@)
	$(CXX) $(CONVERTER_OBJS) -o $@ $(LDFLAGS)

# Build step for C++ source
$(BUILD_DIR)/%.cpp.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

# Alias for $(BUILD_DIR)/minisat/bin/minisat.
build_minisat: $(BUILD_DIR)/minisat/bin/minisat

# Clone minisat from github
$(BUILD_DIR)/minisat/bin/minisat:
	@mkdir -p $(dir $(BUILD_DIR))
	@echo "Cloning minisat from github..."
	@rm -rf $(BUILD_DIR)/minisat
	git clone $(SAT_SOLVER_REPO) $(BUILD_DIR)/minisat
	@echo "Building minisat..."
	cd $(BUILD_DIR)/minisat && cmake . && $(MAKE)
	@echo "Installing minisat executable as satsolver in Wanda's resources folder."
	@mkdir -p $(BIN_DIR)/resources
	cp $(BUILD_DIR)/minisat/bin/minisat $(BIN_DIR)/resources/satsolver

# Alias for $(BUILD_DIR)/NaTT
build_natt : $(BUILD_DIR)/NaTT

$(BUILD_DIR)/NaTT:
	@echo "Extracting NaTT source files."
	@mkdir -p $(dir $(BUILD_DIR))
	tar -xf resources/$(NATT_SRC_ZIP) -C $(BUILD_DIR)
	@echo "Building NaTT using opam..."
	opam install ocamlfind ocamlgraph re
	opam install xml-light
	cd $(BUILD_DIR)/NaTT && $(MAKE)
	@echo "Done."
	@echo "Installing NaTT as the firstorder prover in Wanda's resources folder."
	rm -rf $(BIN_DIR)/resources/natt/NaTT.exe
	cp $(BUILD_DIR)/NaTT/bin/NaTT.exe $(BIN_DIR)/resources/natt

install_resources : $(BIN_DIR)/resources

$(BIN_DIR)/resources:
	@echo "Installing resources to binary folder..."
	@mkdir -p $(BIN_DIR)
	cp -r resources $(BIN_DIR)

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(BIN_DIR)

# Include the .d makefiles. The - at the front suppresses the errors of missing
# Makefiles. Initially, all the .d files will be missing, and we don't want those
# errors to show up.
-include $(DEPS)
