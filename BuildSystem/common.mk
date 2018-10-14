# This is the common portion of the make system. You should not have to make any
# modifications to this file.

PROJECTDIR := $(shell pwd)
PACKAGEBASENAME := $(shell basename `pwd` | tr '[:upper:]' '[:lower:]')
PACKAGEBASENAME := $(shell echo $(PACKAGEBASENAME) | sed 's/^$(PREFIX)//')
BUILDSYSTEMDIR := $(PROJECTDIR)/BuildSystem
VERSION := $(shell $(BUILDSYSTEMDIR)/revision.sh)
VERSIONMACRO := $(shell echo $(PREFIX)$(PACKAGEBASENAME) | tr '[:lower:]' '[:upper:]')_VERSION
PREFIX := /usr/local

OS := $(shell uname -s)
ARCH := $(OS)-$(shell uname -m)
BUILDDIR := .build/$(ARCH)
HEADERDIR := $(BUILDDIR)/include/$(PREFIX)/$(PACKAGEBASENAME)
LIBDIR := $(BUILDDIR)/lib

ifeq ($(OS),Darwin)
	SOEXT := .$(VERSION).dylib
	SOEXT_SHORT := .dylib
	LDPATHEXPR := DYLD_LIBRARY_PATH="$(LIBDIR)"
	CC := clang
	CXX := clang++
else
	SOEXT := .so.$(VERSION)
	SOEXT_SHORT := .so
	LIBS := $(LIBS) -lpthread
	LDPATHEXPR := LD_LIBRARY_PATH="$(LIBDIR)"
	CC := gcc
	CXX := g++
	CFLAGS := $(CFLAGS) -fPIC 
endif

CFLAGS := $(CFLAGS) -Wall
OPTIMIZED_FLAGS := -O2 -DNDEBUG
DEBUGGING_FLAGS := -g
ifeq ($(DEBUG),true)
	CFLAGS := $(CFLAGS) $(DEBUGGING_FLAGS)
else
	CFLAGS := $(CFLAGS) $(OPTIMIZED_FLAGS)
endif

CXXFLAGS := $(CXXFLAGS) $(CFLAGS)
LDFLAGS := $(LDFLAGS) -L$(LIBDIR)

-include $(PROJECTDIR)/config.local
-include $(PROJECTDIR)/config.defs

CFLAGS := $(CFLAGS) -I$(BUILDDIR)/include
CXXFLAGS := $(CXXFLAGS) -I$(BUILDDIR)/include -std=c++14 -Wno-unknown-pragmas

.phony: build library install check clean cleanall directory-checks hello prep

LIBNAME := $(PREFIX)$(PACKAGEBASENAME)
LIBFILE := lib$(LIBNAME)$(SOEXT)
LIBFILELINK := lib$(LIBNAME)$(SOEXT_SHORT)
LIBPATH := $(LIBDIR)/$(LIBFILE)

TESTDIR := $(BUILDDIR)/tests
TESTPATH := $(TESTDIR)/unittest


# Turn off the building of the tests if there is no Tests directory.
ifeq ($(wildcard Tests/.*),)
	TESTPATH :=
endif

build: library

library: $(LIBPATH)


# Use "make hello" to test that the names are what you would expect.

hello:
	@echo "Hello from $(PACKAGEBASENAME)"
	@echo "  PROJECTDIR=$(PROJECTDIR)"
	@echo "  PREFIX=$(PREFIX)"
	@echo "  VERSION=$(VERSION)"
	@echo "  HEADERDIR=$(HEADERDIR)"
	@echo "  LIBNAME=$(LIBNAME)"
	@echo "  LIBPATH=$(LIBPATH)"
	@echo "  CC=$(CC)"
	@echo "  CFLAGS=$(CFLAGS)"
	@echo "  CXX=$(CXX)"
	@echo "  CXXFLAGS=$(CXXFLAGS)"
ifneq ($(wildcard Tests/.*),)
	@echo "  TESTPATH=$(TESTPATH)"
endif

# Build the library

C_SRCS := $(wildcard Sources/*.c)
CPP_SRCS := $(wildcard Sources/*.cpp)
SRCS := $(C_SRCS) $(CPP_SRCS)
SRCS := $(filter-out Sources/main.cpp, $(SRCS))
HDRS := $(wildcard Sources/*.h)
HDRS := $(filter-out Sources/_*.h, $(HDRS))
HDRS := $(filter-out Sources/all.h, $(HDRS))
OBJS := $(patsubst Sources/%.cpp,$(BUILDDIR)/%.o,$(CPP_SRCS)) $(patsubst Sources/%.c,$(BUILDDIR)/%.o,$(C_SRCS))

PREPS := Sources/_version_internal.h Sources/all.h $(HEADERDIR)

prep: $(PREPS)

$(LIBPATH): $(PREPS) $(LIBDIR) $(OBJS)
	$(CXX) $(LDFLAGS) -shared $(OBJS) $(LIBS) -o $@
	(cd $(LIBDIR) ; rm -f $(LIBFILELINK) ; ln -s $(LIBFILE) $(LIBFILELINK))

$(BUILDDIR)/%.o: Sources/%.cpp $(HDRS)
	$(CXX) -c $< $(CXXFLAGS) -o $@

$(BUILDDIR)/%.o: Sources/%.c $(HDRS)
	$(CC) -c $< $(CFLAGS) -o $@

$(BUILDDIR):
	-mkdir -p $@

$(LIBDIR):
	-mkdir -p $@

Sources/_version_internal.h: REVISION
	echo "Rebuild $@"
	@echo "#ifndef $(PREFIX)$(PACKAGEBASENAME)__version_internal_h" > $@
	@echo "#define $(PREFIX)$(PACKAGEBASENAME)__version_internal_h" >> $@
	@echo "" >> $@
	@echo "/* This file is auto-generated and should not be edited. */" >> $@
	@echo "" >> $@
	@echo "#define $(VERSIONMACRO) \"$(VERSION)"\" >> $@
	@echo "" >> $@
	@echo "#endif" >> $@
	@echo "" >> $@

Sources/all.h: $(HDRS)
	(cd Sources ; $(BUILDSYSTEMDIR)/generate_all_h.sh) > $@

$(HEADERDIR):
	echo build $(HEADERDIR)
	-mkdir -p $(BUILDDIR)/include/$(PREFIX)
	-rm $(BUILDDIR)/include/$(PREFIX)/$(PACKAGEBASENAME)
	-ln -s `pwd`/Sources $(BUILDDIR)/include/$(PREFIX)/$(PACKAGEBASENAME)


# Build and run the unit tests.

TESTSRCS := $(wildcard Tests/*.cpp)
ifneq ($(wildcard Sources/main.cpp),)
	TESTSRCS := $(TESTSRCS) $(SRCS)
endif
TESTSRCS := $(filter-out Sources/main.cpp, $(TESTSRCS))
TESTOBJS := $(patsubst Tests/%.cpp,$(TESTDIR)/%.o,$(TESTSRCS))
TESTHDRS := $(wildcard Tests/*.h)

check: library $(TESTPATH)
ifneq ($(wildcard $(EXEPATH)),)
	$(LDPATHEXPR) $(EXEPATH) --version
endif
	$(LDPATHEXPR) $(TESTPATH)

$(TESTPATH): $(LIBPATH) $(TESTDIR) $(TESTOBJS)
	$(CXX) $(LDFLAGS) -L$(BUILDDIR) $(TESTOBJS) -l $(LIBNAME) $(LIBS) -o $@

$(TESTDIR):
	-mkdir -p $@

$(TESTDIR)/%.o: Tests/%.cpp $(TESTHDRS)
	$(CXX) -c $< $(CXXFLAGS) -I. -o $@


# Perform the install.
install:

# Clean the build.
clean:
	rm -rf $(BUILDDIR) REVISION Sources/_version_internal.h Sources/all.h

cleanall: clean
	rm -rf .build config.defs config.target.defs