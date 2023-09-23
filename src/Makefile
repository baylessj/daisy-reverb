# Project Name
TARGET = cloudseed

# Sources
CPP_SOURCES = allocator.cpp cloudseed.cpp  cloudseed/audiolib/biquad.cpp cloudseed/audiolib/sharandom.cpp cloudseed/audiolib/valuetables.cpp
# Library Locations
LIBDAISY_DIR = ../libdaisy
DAISYSP_DIR = ../DaisySP

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

# Include terrarium.h
C_INCLUDES += -I../Terrarium -I../DaisySP/Source
