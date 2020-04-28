# (C)2004-2008 SourceMod Development Team
# Makefile written by David "BAILOPAN" Anderson

SMSDK = ../sourcemod
HL2SDK_ORIG = ../hl2sdk
HL2SDK_OB = ../hl2sdk-ob
HL2SDK_CSS = ../hl2sdk-css
HL2SDK_OB_VALVE = ../hl2sdk-tf2
HL2SDK_L4D = ../hl2sdk-l4d
HL2SDK_L4D2 = ../hl2sdk-l4d2
HL2SDK_CSGO = ../hl2sdk-csgo
MMSOURCE19 = ../mmsource

#####################################
### EDIT BELOW FOR OTHER PROJECTS ###
#####################################

PROJECT = cleaner
OBJECTS = smsdk_ext.cpp extension.cpp CDetour/detours.cpp

##############################################
### CONFIGURE ANY OTHER FLAGS/OPTIONS HERE ###
##############################################

C_OPT_FLAGS = -DNDEBUG -O3 -funroll-loops -pipe -fno-strict-aliasing
C_DEBUG_FLAGS = -D_DEBUG -DDEBUG -g -ggdb3
C_GCC4_FLAGS = -fvisibility=hidden
CPP_GCC4_FLAGS = -fvisibility-inlines-hidden
CPP = gcc

override ENGSET = false
ifneq (,$(filter original orangebox orangeboxvalve css left4dead left4dead2 csgo,$(ENGINE)))
	override ENGSET = true
endif

ifeq "$(ENGINE)" "original"
	HL2SDK = $(HL2SDK_ORIG)
	CFLAGS += -DSOURCE_ENGINE=1
	GAMEFIX = 1.ep1
endif
ifeq "$(ENGINE)" "orangebox"
	HL2SDK = $(HL2SDK_OB)
	CFLAGS += -DSOURCE_ENGINE=3
	GAMEFIX = 2.ep2
endif
ifeq "$(ENGINE)" "css"
	HL2SDK = $(HL2SDK_CSS)
	CFLAGS += -DSOURCE_ENGINE=6
	GAMEFIX = 2.css
endif
ifeq "$(ENGINE)" "orangeboxvalve"
	HL2SDK = $(HL2SDK_OB_VALVE)
	CFLAGS += -DSOURCE_ENGINE=7
	GAMEFIX = 2.ep2v
endif
ifeq "$(ENGINE)" "left4dead"
	HL2SDK = $(HL2SDK_L4D)
	CFLAGS += -DSOURCE_ENGINE=8
	GAMEFIX = 2.l4d
endif
ifeq "$(ENGINE)" "left4dead2"
	HL2SDK = $(HL2SDK_L4D2)
	CFLAGS += -DSOURCE_ENGINE=9
	GAMEFIX = 2.l4d2
endif
ifeq "$(ENGINE)" "csgo"
	HL2SDK = $(HL2SDK_CSGO)
	CFLAGS += -DSOURCE_ENGINE=12
	GAMEFIX = 2.csgo
endif

HL2PUB = $(HL2SDK)/public

ifeq "$(ENGINE)" "original"
	INCLUDE += -I$(HL2SDK)/public/dlls
	METAMOD = $(MMSOURCE19)/core-legacy
else
	INCLUDE += -I$(HL2SDK)/public/game/server
	METAMOD = $(MMSOURCE19)/core
endif

OS := $(shell uname -s)

ifeq "$(OS)" "Darwin"
	LIB_EXT = dylib
	HL2LIB = $(HL2SDK)/lib/mac
else
	LIB_EXT = so
	ifeq "$(ENGINE)" "original"
		HL2LIB = $(HL2SDK)/linux_sdk
	else
		HL2LIB = $(HL2SDK)/lib/linux
	endif
endif

# if ENGINE is original or OB
ifneq (,$(filter original orangebox,$(ENGINE)))
	LIB_SUFFIX = _i486.$(LIB_EXT)
else
	LIB_PREFIX = lib
	ifneq (,$(filter orangeboxvalve css left4dead2,$(ENGINE)))
		ifneq "$(OS)" "Darwin"
			LIB_SUFFIX = _srv.$(LIB_EXT)
		else
			LIB_SUFFIX = .$(LIB_EXT)
		endif
	else
		LIB_SUFFIX = .$(LIB_EXT)
	endif
endif

INCLUDE += -I. -I.. -I$(HL2PUB) -I$(HL2PUB)/engine -I$(HL2PUB)/tier0 \
	-I$(HL2PUB)/tier1 -I$(METAMOD) -I$(METAMOD)/sourcehook -I$(SMSDK)/public -I$(SMSDK)/public/extensions \
	-I$(SMSDK)/public/amtl -I$(SMSDK)/sourcepawn/include

LINK += $(HL2LIB)/tier1_i486.a $(LIB_PREFIX)vstdlib$(LIB_SUFFIX) $(LIB_PREFIX)tier0$(LIB_SUFFIX)

ifeq "$(ENGINE)" "csgo"
	LINK += $(HL2LIB)/interfaces_i486.a
endif

CFLAGS += -DSE_EPISODEONE=1 -DSE_DARKMESSIAH=2 -DSE_ORANGEBOX=3 -DSE_BLOODYGOODTIME=4 -DSE_EYE=5 \
	-DSE_CSS=6 -DSE_ORANGEBOXVALVE=7 -DSE_LEFT4DEAD=8 -DSE_LEFT4DEAD2=9 -DSE_ALIENSWARM=10 \
	-DSE_PORTAL2=11 -DSE_CSGO=12

LINK += -m32 -ldl -lm

CFLAGS += -DPOSIX -Dstricmp=strcasecmp -D_stricmp=strcasecmp -D_strnicmp=strncasecmp -Dstrnicmp=strncasecmp \
	-D_snprintf=snprintf -D_vsnprintf=vsnprintf -D_alloca=alloca -Dstrcmpi=strcasecmp -DCOMPILER_GCC \
	-Wno-switch -Wall -Werror -mfpmath=sse -msse -DSOURCEMOD_BUILD -DHAVE_STDINT_H -m32
CPPFLAGS += -Wno-non-virtual-dtor -fno-exceptions -fno-rtti -fno-threadsafe-statics -std=c++11 

################################################
### DO NOT EDIT BELOW HERE FOR MOST PROJECTS ###
################################################

BINARY = $(PROJECT).ext.$(GAMEFIX).$(LIB_EXT)

ifeq "$(DEBUG)" "true"
	BIN_DIR = Debug
	CFLAGS += $(C_DEBUG_FLAGS)
else
	BIN_DIR = Release
	CFLAGS += $(C_OPT_FLAGS)
endif

ifeq "$(OS)" "Darwin"
	CPP = $(CPP_OSX)
	LIB_EXT = dylib
	CFLAGS += -DOSX -D_OSX
	LINK += -dynamiclib -lstdc++ -mmacosx-version-min=10.5
else
	LIB_EXT = so
	CFLAGS += -D_LINUX
	LINK += -static-libgcc -shared
endif

GCC_VERSION := $(shell $(CPP) -dumpversion >&1 | cut -b1)
ifeq "$(GCC_VERSION)" "4"
	CFLAGS += $(C_GCC4_FLAGS)
	CPPFLAGS += $(CPP_GCC4_FLAGS)
endif

OBJ_LINUX := $(OBJECTS:%.cpp=$(BIN_DIR)/%.o)

$(BIN_DIR)/%.o: %.cpp
	$(CPP) $(INCLUDE) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

$(BIN_DIR)/CDetour/%.o: CDetour/%.cpp
	$(CPP) $(INCLUDE) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

all: check
	ln -sf $(SMSDK)/public/smsdk_ext.cpp
	mkdir -p $(BIN_DIR)/CDetour
	ln -sf $(HL2LIB)/$(LIB_PREFIX)vstdlib$(LIB_SUFFIX); \
	ln -sf $(HL2LIB)/$(LIB_PREFIX)tier0$(LIB_SUFFIX); \
	$(MAKE) -f Makefile extension

check:
	if [ "$(ENGSET)" = "false" ]; then \
		echo "You must supply one of the following values for ENGINE:"; \
		echo "csgo, left4dead2, left4dead, css, orangeboxvalve, orangebox, or original"; \
		exit 1; \
	fi

extension: check $(OBJ_LINUX)
	$(CPP) $(INCLUDE) $(OBJ_LINUX) asm/asm.c $(LINK) -o $(BIN_DIR)/$(BINARY)

debug:
	$(MAKE) -f Makefile all DEBUG=true

default: all

clean: check
	rm -rf $(BIN_DIR)/*.o
	rm -rf $(BIN_DIR)/CDetour/*.o
	rm -rf $(BIN_DIR)/asm/*.o
	rm -rf $(BIN_DIR)/$(BINARY)
