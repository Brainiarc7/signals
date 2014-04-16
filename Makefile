CFLAGS := -std=c++11 -Wall -Wl,-z,relro -Wl,-z,now -fvisibility=hidden -W -Wno-unused-parameter -Wno-unused-function -Wno-unused-label -Wpointer-arith -Wformat -Wreturn-type -Wsign-compare -Wmultichar -Wformat-nonliteral -Winit-self -Wuninitialized -Wno-deprecated -Wformat-security
LDFLAGS := -lpthread -lswscale -lswresample -lgpac -lavcodec -lavformat -lavutil -lz -L/usr/local/lib -lSDL2 -lx264 -ldl

CFLAGS+=-D__STDC_CONSTANT_MACROS

BIN=bin/make
SRC=.

# default to debug mode
DEBUG?=1

ifeq ($(DEBUG), 1)
  CFLAGS += -Werror -Wno-deprecated-declarations
  CFLAGS += -g3
  LDFLAGS += -g
else
  CFLAGS += -s -O3 -DNDEBUG -Wno-unused-variable -Wno-deprecated-declarations
  LDFLAGS += -s
endif

CFLAGS += -I$(SRC)/signals
CFLAGS += -I$(SRC)/modules/src
CFLAGS += -I$(SRC)/gpacpp
CFLAGS += -I$(SRC)/ffpp

CFLAGS += -I$(SRC)/extra/include
LDFLAGS += -L$(SRC)/extra/lib

ifeq ($(CXX),clang++)
  CFLAGS += -stdlib=libc++
  LDFLAGS += -stdlib=libc++
endif

all: targets

# include sub-projects here
#

TARGETS:=
DEPS:=

ProjectName:=utils
include $(ProjectName)/project.mk
CFLAGS+=-I$(ProjectName)

ProjectName:=modules
include $(ProjectName)/project.mk
CFLAGS+=-I$(ProjectName)

ProjectName:=mm
include $(ProjectName)/project.mk
CFLAGS+=-I$(ProjectName)

ProjectName:=tests
include $(ProjectName)/project.mk
CFLAGS+=-I$(ProjectName)

ProjectName:=apps/player
include $(ProjectName)/project.mk
CFLAGS+=-I$(ProjectName)

#------------------------------------------------------------------------------

targets: $(TARGETS)

unit: $(TARGETS)

include extra.mak

#------------------------------------------------------------------------------

$(BIN)/%.exe:
	@mkdir -p $(dir $@)
	$(CXX) -o "$@" $^ $(LDFLAGS)
	
$(BIN)/%.o: $(SRC)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) -c -o "$@" $< $(CFLAGS)
	
clean:
	rm -rf $(BIN)
	mkdir $(BIN)

#-------------------------------------------------------------------------------

$(BIN)/alldeps: $(DEPS)
	@mkdir -p "$(dir $@)"
	cat $^ > "$@"

$(BIN)/%.deps: %.cpp
	@mkdir -p "$(dir $@)"
	$(CXX) $(CFLAGS) -c -MM "$^" -MT "$(BIN)/$*.o" > "$@"

-include $(BIN)/alldeps
