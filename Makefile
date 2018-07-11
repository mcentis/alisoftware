# directories
INCLUDEDIR = include
SRCDIR = src
OBJDIR = obj
EXECUTABLEDIR = bin

# compiler
CC = g++
CFLAGS = -c -g -Wall `root-config --cflags` -I$(INCLUDEDIR)

#linker
LINKER = g++
LDFLAGS = `root-config --libs`

# the := expands the meaning of the expression in the variable assignment 
SOURCES := $(wildcard $(SRCDIR)/*.cc) # take all the .cc files in the src folder
OBJECTS := $(SOURCES:$(SRCDIR)/%.cc=$(OBJDIR)/%.o) # in the SOURCES (variable content) what matches $(SRCDIR)/%.cc becomes $(OBJDIR)/%.o where the % is used to create an "entry to entry" correspondance
TARGETS_SOURCES := $(wildcard $(SRCDIR)/*.cpp)
TARGETS_OBJECTS := $(TARGETS_SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
TARGETS := $(TARGETS_SOURCES:$(SRCDIR)/%.cpp=$(EXECUTABLEDIR)/%)
#TARGETS += $(EXECUTABLEDIR)/readTrees

ALLINCLUDE = $(INCLUDEDIR)/clusterDef.h
#ALLINCLUDE := $(wildcard $(INCLUDEDIR)/*.h)
#ALLINCLUDE += $(wildcard $(INCLUDEDIR)/*.hh)
ROOTDICTFILES := $(filter-out $(INCLUDEDIR)/LinkDef.h, $(ALLINCLUDE)) # LinkDef must be the last file given to rootcint

all: $(TARGETS)


$(TARGETS): $(EXECUTABLEDIR)/%: $(OBJDIR)/%.o $(OBJECTS) $(OBJDIR)/Dict.so # the dict is necessary to let root use the cluster structure in the root ttree
	@$(LINKER) $< $(OBJECTS) $(OBJDIR)/Dict.so $(LDFLAGS) -o $@
	@echo "\tLinking "$@" complete"

$(TARGETS_OBJECTS): $(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@$(CC) $(CFLAGS) $< -o $@
	@echo "\tCompiled "$<" succesfully"

# $< is the current input file and $@ is the target of this action the @ at the beginning of the line is to not print out the line
$(OBJECTS): $(OBJDIR)/%.o: $(SRCDIR)/%.cc # create a object $(OBJDIR)/%.o from the file $(SRCDIR)/%.cc for the name matching $(OBJDIR)/%.o in the OBJECT variable
	@$(CC) $(CFLAGS) $< -o $@
	@echo "\tCompiled "$<" succesfully"

$(OBJDIR)/Dict.cxx: $(ROOTDICTFILES) $(INCLUDEDIR)/LinkDef.h
	@rootcint -f $@ $(CFLAGS) -p $^
	@echo "\tGenerated "$@

$(OBJDIR)/Dict.so: $(OBJDIR)/Dict.cxx
	@$(CC) -shared -o $@ `root-config --ldflags` $(CFLAGS) -I. $^
	@echo "\tGenerated "$@

clean:
	rm -f $(TARGETS) $(wildcard $(OBJDIR)/*)

clear:
	rm -f $(wildcard $(INCLUDEDIR)/*~) $(wildcard $(SRCDIR)/*~) $(wildcard tests/*~)
