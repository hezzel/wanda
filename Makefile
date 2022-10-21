objects = afs.o alphabet.o beta.o dependencypair.o dependencygraph.o dpframework.o environment.o firstorder.o formula.o horpo.o horpoconstraintlist.o horpojustifier.o inputreaderafs.o inputreaderafsm.o inputreaderatrs.o inputreaderfo.o matchrule.o nonterminator.o orderingproblem.o outputmodule.o polconstraintlist.o polymodule.o bitblaster.o polynomial.o rule.o rulesmanipulator.o ruleremover.o sat.o smt.o subcritchecker.o substitution.o term.o textconverter.o type.o typer.o typesubstitution.o varset.o wanda.o xmlreader.o

goal : wanda.exe converter.exe

################################################################################
# In order to compile Wanda on macOS a.k.a Darwin Systems,
# dynamic linking is necessary.
# Static linking will not work on Mac OS X unless all libraries
# (including libgcc.a) have also been compiled with -static.
# Since neither a static version of libSystem.dylib nor crt0.o are provided,
# this option will not be supported on macs.
# Below, there is an updated version of the linking for macs and non-macs.
# The linking remain static on other systems.
# Below we check if the system is darwin, which will use dynamic linking,
# or any other system (Tested on Linux and Windows/WinGW) which will use static.
################################################################################

SYS := $(shell gcc -dumpmachine)
ifneq (, $(findstring darwin, $(SYS)))
wanda.exe : $(objects)
	g++ -o wanda.exe $(objects)
else
wanda.exe : $(objects)
	g++ -static -o wanda.exe $(objects)
endif

converter.exe : afs.o alphabet.o beta.o dependencypair.o environment.o inputreaderafs.o inputreaderafsm.o inputreaderatrs.o inputreaderatrs.o inputreaderfo.o matchrule.o outputmodule.o polynomial.o rule.o substitution.o term.o textconverter.o type.o typer.o typesubstitution.o varset.o xmlreader.o converter.cpp converter.h
	g++ -o converter.exe converter.cpp afs.o alphabet.o beta.o dependencypair.o environment.o inputreaderafs.o inputreaderafsm.o inputreaderatrs.o matchrule.o outputmodule.o polynomial.o rule.o substitution.o term.o textconverter.o type.o typer.o typesubstitution.o varset.o xmlreader.o

afs.o : afs.cpp afs.h outputmodule.h substitution.h
	g++ -c afs.cpp

afs.h : term.h alphabet.h environment.h matchrule.h
	touch afs.h

alphabet.o : alphabet.cpp alphabet.h
	g++ -c alphabet.cpp

alphabet.h : type.h term.h
	touch alphabet.h

beta.o : beta.cpp beta.h substitution.h
	g++ -c beta.cpp

beta.h : term.h rule.h
	touch beta.h

bitblaster.o : bitblaster.cpp bitblaster.h polconstraintlist.h sat.h
	g++ -c bitblaster.cpp

bitblaster.h : polynomial.h formula.h
	touch bitblaster.h

dependencypair.o : dependencypair.cpp dependencypair.h environment.h
	g++ -c dependencypair.cpp

dependencypair.h : term.h
	touch dependencypair.h

dependencygraph.o : dependencygraph.cpp dependencygraph.h outputmodule.h substitution.h
	g++ -c dependencygraph.cpp

dependencygraph.h : dependencypair.h typer.h matchrule.h
	touch dependencygraph.h

dpframework.o : dpframework.cpp dpframework.h beta.h environment.h horpo.h outputmodule.h polymodule.h subcritchecker.h substitution.h term.h
	g++ -c dpframework.cpp

dpframework.h : alphabet.h dependencypair.h dependencygraph.h firstorder.h orderingproblem.h rulesmanipulator.h
	touch dpframework.h

environment.o : environment.cpp environment.h term.h
	g++ -c environment.cpp

environment.h : type.h varset.h
	touch environment.h

firstorder.o : firstorder.cpp firstorder.h environment.h inputreaderfo.h
	g++ -c firstorder.cpp

firstorder.h : matchrule.h dependencypair.h alphabet.h
	touch firstorder.h

formula.o : formula.cpp formula.h
	g++ -c formula.cpp

horpo.o : horpo.cpp horpo.h horpojustifier.h outputmodule.h sat.h
	g++ -c horpo.cpp

horpo.h : alphabet.h formula.h horpoconstraintlist.h orderingproblem.h
	touch horpo.h

horpojustifier.o : horpojustifier.cpp horpo.h horpojustifier.h outputmodule.h
	g++ -c horpojustifier.cpp

horpojustifier.h : alphabet.h environment.h orderingproblem.h
	touch horpojustifier.h

horpoconstraintlist.o : horpoconstraintlist.cpp horpo.h substitution.h
	g++ -c horpoconstraintlist.cpp

horpoconstraintlist.h : requirement.h formula.h
	touch horpoconstraintlist.h

inputreaderafsm.o : inputreaderafsm.cpp inputreaderafsm.h
	g++ -c inputreaderafsm.cpp

inputreaderafsm.h : textconverter.h matchrule.h outputmodule.h
	touch inputreaderafsm.h

inputreaderatrs.o : inputreaderatrs.cpp inputreaderatrs.h afs.h outputmodule.h substitution.h typesubstitution.h
	g++ -c inputreaderatrs.cpp

inputreaderatrs.h : textconverter.h matchrule.h
	touch inputreaderatrs.h

inputreaderafs.o : inputreaderafs.cpp inputreaderafs.h substitution.h
	g++ -c inputreaderafs.cpp

inputreaderafs.h : textconverter.h matchrule.h afs.h
	touch inputreaderafs.h

inputreaderfo.o : inputreaderfo.cpp inputreaderfo.h outputmodule.h rulesmanipulator.h substitution.h
	g++ -c inputreaderfo.cpp

inputreaderfo.h : textconverter.h matchrule.h
	touch inputreaderfo.h

matchrule.o : matchrule.cpp matchrule.h environment.h substitution.h typesubstitution.h
	g++ -c matchrule.cpp

matchrule.h : rule.h
	touch matchrule.h

nonterminator.o : nonterminator.cpp nonterminator.h typesubstitution.h substitution.h beta.h environment.h outputmodule.h
	g++ -c nonterminator.cpp

nonterminator.h : matchrule.h beta.h alphabet.h
	touch nonterminator.h

orderingproblem.o : orderingproblem.cpp orderingproblem.h formula.h outputmodule.h rulesmanipulator.h substitution.h
	g++ -c orderingproblem.cpp

orderingproblem.h : alphabet.h dependencypair.h formula.h matchrule.h requirement.h
	touch orderingproblem.h

outputmodule.o : outputmodule.cpp outputmodule.h afs.h
	g++ -c outputmodule.cpp

outputmodule.h : alphabet.h dependencypair.h matchrule.h polynomial.h requirement.h
	touch outputmodule.h

polconstraintlist.o : polconstraintlist.cpp polconstraintlist.h outputmodule.h polymodule.h
	g++ -c polconstraintlist.cpp

polconstraintlist.h : formula.h polynomial.h
	touch polconstraintlist.h

polymodule.o : polymodule.cpp polymodule.h outputmodule.h smt.h substitution.h
	g++ -c polymodule.cpp

polymodule.h : polynomial.h alphabet.h polconstraintlist.h orderingproblem.h
	touch polymodule.h

polynomial.o : polynomial.cpp polynomial.h
	g++ -c polynomial.cpp

polynomial.h : type.h
	touch polynomial.h

requirement.h : environment.h formula.h term.h
	touch requirement.h

rule.o : rule.cpp rule.h
	g++ -c rule.cpp

rule.h : term.h
	touch rule.h

ruleremover.o : ruleremover.cpp ruleremover.h requirement.h outputmodule.h polymodule.h horpo.h rulesmanipulator.h
	g++ -c ruleremover.cpp

ruleremover.h : alphabet.h matchrule.h orderingproblem.h
	touch ruleremover.h

rulesmanipulator.o : rulesmanipulator.cpp rulesmanipulator.h beta.h sat.h substitution.h typer.h typesubstitution.h
	g++ -c rulesmanipulator.cpp

rulesmanipulator.h : alphabet.h dependencypair.h formula.h matchrule.h
	touch rulesmanipulator.h

sat.o : sat.cpp sat.h
	g++ -c sat.cpp

sat.h : formula.h
	touch sat.h

smt.o : smt.cpp smt.h bitblaster.h
	g++ -c smt.cpp

smt.h : formula.h polynomial.h polconstraintlist.h
	touch smt.h

subcritchecker.o : subcritchecker.cpp subcritchecker.h environment.h outputmodule.h rulesmanipulator.h sat.h
	g++ -c subcritchecker.cpp

subcritchecker.h : dependencypair.h formula.h
	touch subcritchecker.h

substitution.o : substitution.cpp substitution.h environment.h
	g++ -c substitution.cpp

substitution.h : term.h
	touch substitution.h

textconverter.o : textconverter.cpp textconverter.h environment.h typesubstitution.h
	g++ -c textconverter.cpp

textconverter.h : alphabet.h term.h type.h typer.h
	touch textconverter.h

term.o : term.cpp term.h environment.h substitution.h
	g++ -c term.cpp

term.h : type.h varset.h
	touch term.h

type.o : type.cpp type.h typesubstitution.h
	g++ -c type.cpp

type.h : varset.h
	touch type.h

typer.o : typer.cpp typer.h typesubstitution.h
	g++ -c typer.cpp

typer.h : term.h
	touch typer.h

typesubstitution.o : typesubstitution.cpp typesubstitution.h
	g++ -c typesubstitution.cpp

typesubstitution.h : type.h
	touch typesubstitution.h

varset.o : varset.cpp varset.h term.h type.h
	g++ -c varset.cpp

wanda.o : wanda.cpp wanda.h beta.h dpframework.h inputreaderafsm.h inputreaderatrs.h inputreaderafs.h inputreaderfo.h nonterminator.h outputmodule.h xmlreader.h ruleremover.h
	g++ -c wanda.cpp

wanda.h : alphabet.h matchrule.h
	touch wanda.h

xmlreader.o : xmlreader.cpp xmlreader.h
	g++ -c xmlreader.cpp

.PHONY : clean
clean :
	rm $(objects)

