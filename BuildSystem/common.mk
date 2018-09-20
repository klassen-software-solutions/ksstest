# This is the common portion of the make system. You should not have to make any
# modifications to this file.

PROJECTDIR := $(shell pwd)
PACKAGEBASENAME := $(shell basename `pwd` | tr '[:upper:]' '[:lower:]')
PACKAGEBASENAME := $(shell echo $(PACKAGEBASENAME) | sed 's/^$(PREFIX)//')
BUILDSYSTEMDIR := $(PROJECTDIR)/BuildSystem
VERSION := $(shell $(BUILDSYSTEMDIR)/revision.sh)
VERSIONMACRO := $(shell echo $(PREFIX)$(PACKAGEBASENAME) | tr '[:lower:]' '[:upper:]')_VERSION

OS := $(shell uname -s)
ARCH := $(OS)-$(shell uname -m)
BUILDDIR := .build/$(ARCH)
HEADERDIR := $(BUILDDIR)/include/$(PREFIX)/$(PACKAGEBASENAME)
LIBDIR := $(BUILDDIR)/lib
APPDIR := $(BUILDDIR)/app

ifeq ($(OS),Darwin)
	SOEXT := .$(VERSION).dylib
	SOEXT_SHORT := .dylib
	LDPATHEXPR := DYLD_LIBRARY_PATH="$(LIBDIR)"
	CC := clang
	C++ := clang++
else
	SOEXT := .so.$(VERSION)
	SOEXT_SHORT := .so
	LIBS := $(LIBS) -lpthread
	LDPATHEXPR := LD_LIBRARY_PATH="$(LIBDIR)"
	CC := gcc
	C++ := g++
	CFLAGS := $(CFLAGS) -fPIC 
endif

.phony: build library application check clean cleanall directory-checks hello prep

LIBNAME := $(PREFIX)$(PACKAGEBASENAME)
LIBFILE := lib$(LIBNAME)$(SOEXT)
LIBFILELINK := lib$(LIBNAME)$(SOEXT_SHORT)
LIBPATH := $(LIBDIR)/$(LIBFILE)

EXENAME := $(PACKAGEBASENAME)
EXEPATH := $(APPDIR)/$(EXENAME)

TESTDIR := $(BUILDDIR)/tests
TESTPATH := $(TESTDIR)/unittest


# Turn off the building of the library, application, and tests, based on the existance
# of Sources/main.cpp and Tests directories.

ifeq ($(wildcard Sources/main.cpp),)
	EXEPATH :=
else
	LIBPATH :=
endif

ifeq ($(wildcard Tests/.*),)
	TESTPATH :=
endif

build: library application

library: $(LIBPATH)

application: $(EXEPATH)


# Use "make hello" to test that the names are what you would expect.

hello:
	@echo "Hello from $(PACKAGEBASENAME)"
	@echo "  PROJECTDIR=$(PROJECTDIR)"
	@echo "  VERSION=$(VERSION)"
	@echo "  HEADERDIR=$(HEADERDIR)"
	@echo "  LIBNAME=$(LIBNAME)"
	@echo "  LIBPATH=$(LIBPATH)"
	@echo "  EXEPATH=$(EXEPATH)"
ifneq ($(wildcard Tests/.*),)
	@echo "  TESTPATH=$(TESTPATH)"
endif

CFLAGS := $(CFLAGS) -Wall -I$(BUILDDIR)/include
OPTIMIZED_FLAGS := -O2 -DNDEBUG
DEBUGGING_FLAGS := -g
ifeq ($(DEBUG),true)
	CFLAGS := $(CFLAGS) $(DEBUGGING_FLAGS)
else
	CFLAGS := $(CFLAGS) $(OPTIMIZED_FLAGS)
endif

CPPFLAGS := $(CPPFLAGS) $(CFLAGS) -std=c++14 -Wno-unknown-pragmas
LDFLAGS := $(LDFLAGS) -L$(LIBDIR)

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
	$(C++) $(LDFLAGS) -shared $(OBJS) $(LIBS) -o $@
	(cd $(LIBDIR) ; rm -f $(LIBFILELINK) ; ln -s $(LIBFILE) $(LIBFILELINK))

$(BUILDDIR)/%.o: Sources/%.cpp $(HDRS)
	$(C++) -c $< $(CPPFLAGS) -o $@

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


# Build the executable.

$(EXEPATH): $(PREPS) $(EXEDIR) $(OBJS) $(BUILDDIR)/main.o
	$(C++) $(LDFLAGS) $(OBJS) $(BUILDDIR)/main.o $(LIBS) -o $@

$(EXEDIR):
	-mkdir -p $@

# Build and run the unit tests.

TESTSRCS := $(wildcard Tests/*.cpp)
ifneq ($(wildcard Sources/main.cpp),)
	TESTSRCS := $(TESTSRCS) $(SRCS)
endif
TESTSRCS := $(filter-out Sources/main.cpp, $(TESTSRCS))
TESTOBJS := $(patsubst Tests/%.cpp,$(TESTDIR)/%.o,$(TESTSRCS))
TESTHDRS := $(wildcard Tests/*.h)

check: library application $(TESTPATH)
ifneq ($(wildcard $(EXEPATH)),)
	$(LDPATHEXPR) $(EXEPATH) --version
endif
	$(LDPATHEXPR) $(TESTPATH)

$(TESTPATH): $(LIBPATH) $(TESTDIR) $(TESTOBJS)
	$(C++) $(LDFLAGS) -L$(BUILDDIR) $(TESTOBJS) -l $(LIBNAME) $(LIBS) -o $@

$(TESTDIR):
	-mkdir -p $@

$(TESTDIR)/%.o: Tests/%.cpp $(TESTHDRS)
	$(C++) -c $< $(CPPFLAGS) -I. -o $@



# Clean the build.
clean:
	rm -rf $(BUILDDIR) REVISION Sources/_version_internal.h Sources/all.h Tests/unit_test_main.cpp

cleanall: clean
	rm -rf .build
