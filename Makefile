# Variable declarations
CXX = g++
SYS := $(shell gcc -dumpmachine)
BUILD_DIR := ./build
BIN_DIR := ./bin
SRC_DIRS := ./src
TARGET_EXEC := wanda.exe
CONVERTER_EXEC := converter.exe
SAT_SOLVER_REPO := https://github.com/deividrvale/minisat.git

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
# Build Wanda Executable macOS.
WANDA_OBJS := $(shell find $(BUILD_DIR) -name '*.cpp.o' ! -name 'converter.cpp.o')
$(BIN_DIR)/$(TARGET_EXEC): install_resources build_minisat $(OBJS) $(WANDA_OBJS)
	@echo "Building Wanda Executable..."
	@$(CXX) $(WANDA_OBJS) -o $@ $(LDFLAGS)
else
# Build Wanda Executable Linux.
WANDA_OBJS := $(shell find $(BUILD_DIR) -name '*.cpp.o' ! -name 'converter.cpp.o')
$(BUILD_DIR)/$(TARGET_EXEC): install_resources build_minisat $(OBJS) $(WANDA_OBJS)
	@echo "Building Wanda Executable."
	mkdir -p $(dir $@)
	@$(CXX) $(WANDA_OBJS) -o $@ $(LDFLAGS) - static
endif

# Build Converter Executable
CONVERTER_OBJS := $(shell find $(BUILD_DIR) -name '*.cpp.o' ! -name 'wanda.cpp.o')
$(BIN_DIR)/$(CONVERTER_EXEC): $(CONVERTER_OBJS)
	@echo 'Building Wanda Executable.\n'
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
	cd $(BUILD_DIR)/minisat && cmake . && $(MAKE)
	@echo "Installing minisat executable as satsolver in Wanda's resources folder."
	cp $(BUILD_DIR)/minisat/bin/minisat $(BIN_DIR)/resources/satsolver

install_resources :
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






# all :
# 	echo 'Setting build scheme to' ${SYS}

# goal : wanda.exe converter.exe




# ifneq (, $(findstring darwin, $(SYS)))
# wanda.exe : $(objects)
# 	g++ -o wanda.exe $(objects)
# else
# wanda.exe : $(objects)
# 	g++ -static -o wanda.exe $(objects)
# endif

# converter.exe : afs.o alphabet.o beta.o dependencypair.o environment.o inputreaderafs.o inputreaderafsm.o inputreaderatrs.o inputreaderatrs.o inputreaderfo.o matchrule.o outputmodule.o polynomial.o rule.o substitution.o term.o textconverter.o type.o typer.o typesubstitution.o varset.o xmlreader.o converter.cpp converter.h
# 	g++ -o converter.exe converter.cpp afs.o alphabet.o beta.o dependencypair.o environment.o inputreaderafs.o inputreaderafsm.o inputreaderatrs.o matchrule.o outputmodule.o polynomial.o rule.o substitution.o term.o textconverter.o type.o typer.o typesubstitution.o varset.o xmlreader.o

# afs.o : afs.cpp afs.h outputmodule.h substitution.h
# 	g++ -c afs.cpp

# afs.h : term.h alphabet.h environment.h matchrule.h
# 	touch afs.h

# alphabet.o : alphabet.cpp alphabet.h
# 	g++ -c alphabet.cpp

# alphabet.h : type.h term.h
# 	touch alphabet.h

# beta.o : beta.cpp beta.h substitution.h
# 	g++ -c beta.cpp

# beta.h : term.h rule.h
# 	touch beta.h

# bitblaster.o : bitblaster.cpp bitblaster.h polconstraintlist.h sat.h
# 	g++ -c bitblaster.cpp

# bitblaster.h : polynomial.h formula.h
# 	touch bitblaster.h

# dependencypair.o : dependencypair.cpp dependencypair.h environment.h
# 	g++ -c dependencypair.cpp

# dependencypair.h : term.h
# 	touch dependencypair.h

# dependencygraph.o : dependencygraph.cpp dependencygraph.h outputmodule.h substitution.h
# 	g++ -c dependencygraph.cpp

# dependencygraph.h : dependencypair.h typer.h matchrule.h
# 	touch dependencygraph.h

# dpframework.o : dpframework.cpp dpframework.h beta.h environment.h horpo.h outputmodule.h polymodule.h subcritchecker.h substitution.h term.h
# 	g++ -c dpframework.cpp

# dpframework.h : alphabet.h dependencypair.h dependencygraph.h firstorder.h orderingproblem.h rulesmanipulator.h
# 	touch dpframework.h

# environment.o : environment.cpp environment.h term.h
# 	g++ -c environment.cpp

# environment.h : type.h varset.h
# 	touch environment.h

# firstorder.o : firstorder.cpp firstorder.h environment.h inputreaderfo.h
# 	g++ -c firstorder.cpp

# firstorder.h : matchrule.h dependencypair.h alphabet.h
# 	touch firstorder.h

# formula.o : formula.cpp formula.h
# 	g++ -c formula.cpp

# horpo.o : horpo.cpp horpo.h horpojustifier.h outputmodule.h sat.h
# 	g++ -c horpo.cpp

# horpo.h : alphabet.h formula.h horpoconstraintlist.h orderingproblem.h
# 	touch horpo.h

# horpojustifier.o : horpojustifier.cpp horpo.h horpojustifier.h outputmodule.h
# 	g++ -c horpojustifier.cpp

# horpojustifier.h : alphabet.h environment.h orderingproblem.h
# 	touch horpojustifier.h

# horpoconstraintlist.o : horpoconstraintlist.cpp horpo.h substitution.h
# 	g++ -c horpoconstraintlist.cpp

# horpoconstraintlist.h : requirement.h formula.h
# 	touch horpoconstraintlist.h

# inputreaderafsm.o : inputreaderafsm.cpp inputreaderafsm.h
# 	g++ -c inputreaderafsm.cpp

# inputreaderafsm.h : textconverter.h matchrule.h outputmodule.h
# 	touch inputreaderafsm.h

# inputreaderatrs.o : inputreaderatrs.cpp inputreaderatrs.h afs.h outputmodule.h substitution.h typesubstitution.h
# 	g++ -c inputreaderatrs.cpp

# inputreaderatrs.h : textconverter.h matchrule.h
# 	touch inputreaderatrs.h

# inputreaderafs.o : inputreaderafs.cpp inputreaderafs.h substitution.h
# 	g++ -c inputreaderafs.cpp

# inputreaderafs.h : textconverter.h matchrule.h afs.h
# 	touch inputreaderafs.h

# inputreaderfo.o : inputreaderfo.cpp inputreaderfo.h outputmodule.h rulesmanipulator.h substitution.h
# 	g++ -c inputreaderfo.cpp

# inputreaderfo.h : textconverter.h matchrule.h
# 	touch inputreaderfo.h

# matchrule.o : matchrule.cpp matchrule.h environment.h substitution.h typesubstitution.h
# 	g++ -c matchrule.cpp

# matchrule.h : rule.h
# 	touch matchrule.h

# nonterminator.o : nonterminator.cpp nonterminator.h typesubstitution.h substitution.h beta.h environment.h outputmodule.h
# 	g++ -c nonterminator.cpp

# nonterminator.h : matchrule.h beta.h alphabet.h
# 	touch nonterminator.h

# orderingproblem.o : orderingproblem.cpp orderingproblem.h formula.h outputmodule.h rulesmanipulator.h substitution.h
# 	g++ -c orderingproblem.cpp

# orderingproblem.h : alphabet.h dependencypair.h formula.h matchrule.h requirement.h
# 	touch orderingproblem.h

# outputmodule.o : outputmodule.cpp outputmodule.h afs.h
# 	g++ -c outputmodule.cpp

# outputmodule.h : alphabet.h dependencypair.h matchrule.h polynomial.h requirement.h
# 	touch outputmodule.h

# polconstraintlist.o : polconstraintlist.cpp polconstraintlist.h outputmodule.h polymodule.h
# 	g++ -c polconstraintlist.cpp

# polconstraintlist.h : formula.h polynomial.h
# 	touch polconstraintlist.h

# polymodule.o : polymodule.cpp polymodule.h outputmodule.h smt.h substitution.h
# 	g++ -c polymodule.cpp

# polymodule.h : polynomial.h alphabet.h polconstraintlist.h orderingproblem.h
# 	touch polymodule.h

# polynomial.o : polynomial.cpp polynomial.h
# 	g++ -c polynomial.cpp

# polynomial.h : type.h
# 	touch polynomial.h

# requirement.h : environment.h formula.h term.h
# 	touch requirement.h

# rule.o : rule.cpp rule.h
# 	g++ -c rule.cpp

# rule.h : term.h
# 	touch rule.h

# ruleremover.o : ruleremover.cpp ruleremover.h requirement.h outputmodule.h polymodule.h horpo.h rulesmanipulator.h
# 	g++ -c ruleremover.cpp

# ruleremover.h : alphabet.h matchrule.h orderingproblem.h
# 	touch ruleremover.h

# rulesmanipulator.o : rulesmanipulator.cpp rulesmanipulator.h beta.h sat.h substitution.h typer.h typesubstitution.h
# 	g++ -c rulesmanipulator.cpp

# rulesmanipulator.h : alphabet.h dependencypair.h formula.h matchrule.h
# 	touch rulesmanipulator.h

# sat.o : sat.cpp sat.h
# 	g++ -c sat.cpp

# sat.h : formula.h
# 	touch sat.h

# smt.o : smt.cpp smt.h bitblaster.h
# 	g++ -c smt.cpp

# smt.h : formula.h polynomial.h polconstraintlist.h
# 	touch smt.h

# subcritchecker.o : subcritchecker.cpp subcritchecker.h environment.h outputmodule.h rulesmanipulator.h sat.h
# 	g++ -c subcritchecker.cpp

# subcritchecker.h : dependencypair.h formula.h
# 	touch subcritchecker.h

# substitution.o : substitution.cpp substitution.h environment.h
# 	g++ -c substitution.cpp

# substitution.h : term.h
# 	touch substitution.h

# textconverter.o : textconverter.cpp textconverter.h environment.h typesubstitution.h
# 	g++ -c textconverter.cpp

# textconverter.h : alphabet.h term.h type.h typer.h
# 	touch textconverter.h

# term.o : term.cpp term.h environment.h substitution.h
# 	g++ -c term.cpp

# term.h : type.h varset.h
# 	touch term.h

# type.o : type.cpp type.h typesubstitution.h
# 	g++ -c type.cpp

# type.h : varset.h
# 	touch type.h

# typer.o : typer.cpp typer.h typesubstitution.h
# 	g++ -c typer.cpp

# typer.h : term.h
# 	touch typer.h

# typesubstitution.o : typesubstitution.cpp typesubstitution.h
# 	g++ -c typesubstitution.cpp

# typesubstitution.h : type.h
# 	touch typesubstitution.h

# varset.o : varset.cpp varset.h term.h type.h
# 	g++ -c varset.cpp

# wanda.o : wanda.cpp wanda.h beta.h dpframework.h inputreaderafsm.h inputreaderatrs.h inputreaderafs.h inputreaderfo.h nonterminator.h outputmodule.h xmlreader.h ruleremover.h
# 	g++ -c wanda.cpp

# wanda.h : alphabet.h matchrule.h
# 	touch wanda.h

# xmlreader.o : xmlreader.cpp xmlreader.h
# 	g++ -c xmlreader.cpp

# .PHONY : clean
# clean :
# 	rm -f $(objects)
