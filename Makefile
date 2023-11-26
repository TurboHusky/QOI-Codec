INCLUDE_PATH := include/
SRC_PATH := src/

BUILD_PATH := build/
OBJ_PATH := $(BUILD_PATH)obj/
LIB_PATH := $(BUILD_PATH)lib/
BIN_PATH := $(BUILD_PATH)bin/

CC := cc
AR := ar
CCFLAGS := -O3 -I $(INCLUDE_PATH) -Wall -Wextra -Werror -Wpedantic -Winline -std=c17

LIB_NAMES := qoi
LDFLAGS := -L$(LIB_PATH) $(addprefix -l, $(LIB_NAMES))

STATIC_LIBS := $(LIB_PATH)$(addprefix lib, $(addsuffix .a, $(LIB_NAMES)))
ifeq ($(CC), x86_64-w64-mingw32-cc)
BIN_SUFFIX := .exe
DYNAMIC_LIBS := $(LIB_PATH)$(addprefix lib, $(addsuffix .dll, $(LIB_NAMES)))
else
BIN_SUFFIX := 
DYNAMIC_LIBS := $(LIB_PATH)$(addprefix lib, $(addsuffix .so, $(LIB_NAMES)))
endif

SRC := $(OBJ_PATH)main.o
OBJS := qoi
LIB_SRC := $(addprefix $(OBJ_PATH), $(OBJS))

debug: $(SRC) $(STATIC_LIBS)
	@mkdir -p $(BIN_PATH)
	$(info "GDB debug build")
	# @$(CC) -ggdb $(SRC) -static $(LDFLAGS) -I$(INCLUDE_PATH) -o $(BIN_PATH)qoi$(BIN_SUFFIX)
	cc -ggdb src/main.c src/qoi.c -static -Iinclude/ -o build/bin/qoi


static: $(SRC) $(STATIC_LIBS)
	@mkdir -p $(BIN_PATH)
	$(info "Static build")
	@$(CC) $(SRC) -static $(LDFLAGS) -I$(INCLUDE_PATH) -o $(BIN_PATH)qoi$(BIN_SUFFIX)

shared: $(SRC) $(DYNAMIC_LIBS)
	@mkdir -p $(BIN_PATH)
	$(info "Shared build")
	@$(CC) $(SRC) $(LDFLAGS) -o $(BIN_PATH)qoi$(BIN_SUFFIX)

$(STATIC_LIBS): $(addsuffix .o, $(LIB_SRC))
	@mkdir -p $(@D)
	$(info "Building static lib")
	@$(AR) -rcs $@ $(addsuffix .o, $(LIB_SRC))

$(DYNAMIC_LIBS): $(addsuffix .s.o, $(LIB_SRC))
	@mkdir -p $(@D)
	$(info "Building shared lib")
	@$(CC) -shared $(addsuffix .s.o, $(LIB_SRC)) -o $@

$(OBJ_PATH)%.o : $(SRC_PATH)%.c
	@mkdir -p $(@D)
	@$(CC) -c $< $(CCFLAGS) -o $@

$(OBJ_PATH)%.s.o : $(SRC_PATH)%.c
	@mkdir -p $(@D)
	@$(CC) -fPIC -c $< $(CCFLAGS) -o $@

.PHONY : clean
clean :
	@echo Cleaning up...
	@rm -r build