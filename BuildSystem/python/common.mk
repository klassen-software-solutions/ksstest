.PHONY: prep build check analyze

# Only include the license file dependancy if there are prerequisites to examine.
PREREQS_LICENSE_FILE := Dependancies/prereqs-licenses.json
ifeq ($(wildcard Dependancies/prereqs.json),)
    PREREQS_LICENSE_FILE :=
endif

build: $(PREREQS_LICENSE_FILE)
	echo TODO: build the package

$(PREREQS_LICENSE_FILE): Dependancies/prereqs.json
	BuildSystem/common/license_scanner.py

prep:
	echo TODO: auto preparation

prereqs:
	BuildSystem/common/update_prereqs.py

check: build
	echo TODO: run the tests

analyze:
	BuildSystem/python/python_analyzer.py $(PREFIX)
