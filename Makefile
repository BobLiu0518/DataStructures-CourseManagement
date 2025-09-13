CC = gcc
BASE_CFLAGS = -Wall -Wextra -Wshadow -Wno-comment -Wno-invalid-source-encoding -std=c23

DEBUG ?= 0
ORDER ?= 5
SRCDIR = src
BINDIR = bin
INCDIR = include 

ifeq ($(DEBUG), 0)
    TARGET = $(BINDIR)/release.exe
	CFLAGS = $(BASE_CFLAGS) -O3
else
    TARGET = $(BINDIR)/debug.exe
	CFLAGS = $(BASE_CFLAGS) -g -O0
endif

SOURCES = $(wildcard $(SRCDIR)/*.c)
HEADERS = $(wildcard $(SRCDIR)/*.h) $(wildcard $(INCDIR)/*.h)

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS) Makefile | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $(SOURCES) -DDEBUG=$(DEBUG) -DB_PLUS_TREE_ORDER=$(ORDER) -I$(SRCDIR) -I$(INCDIR)

$(BINDIR):
	@mkdir $(BINDIR)

run: $(TARGET)
	@wt.exe -f --size 40,16 -d "$(CURDIR)" "$(abspath $(TARGET))"

clean:
	@rm -rf $(BINDIR)

rebuild: clean all

.PHONY: all run clean rebuild