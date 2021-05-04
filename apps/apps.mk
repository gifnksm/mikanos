ifndef REPOROOT
    $(error $$(REPOROOT) is not defined)
endif
ifndef APPNAME
    $(error $$(APPNAME) is not defined)
endif

WORKDIR=$(REPOROOT)/target/work/apps/$(APPNAME)
TARGET=$(REPOROOT)/target/apps/$(APPNAME)

CPP_SRCS=$(shell find . -type f -name '*.cpp' -printf '%P\n')
C_SRCS=$(shell find . -type f -name '*.c' -printf '%P\n')
ASM_SRCS=$(shell find . -type f -name '*.asm' -printf '%P\n')

OBJS=$(addprefix $(WORKDIR)/,$(CPP_SRCS:.cpp=.o) $(C_SRCS:.c=.o) $(ASM_SRCS:.asm=.o))
DEPS=$(addprefix $(WORKDIR)/,$(CPP_SRCS:.cpp=.d) $(C_SRCS:.c=.d))

all: 

include $(REPOROOT)/apps/common.mk

all: $(TARGET)
.PHONY: all

clean:
	$(RM) -r $(TARGET) $(WORKDIR)
.PHONY: clean

$(TARGET): $(OBJS) $(COMMON_OBJS) Makefile
	mkdir -p $(@D)
	ld.lld $(LDFLAGS) -o $@ $(OBJS) $(COMMON_OBJS) -lc -lc++ -lc++abi -lm

