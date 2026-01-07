MAKEFLAGS := -j $(shell nproc)

.PHONY: run clean clean_all format disasm raw trace

CC := gcc
CFLAGS := -std=c23 -g -O2 \
			-Wall -Wextra -Wpedantic \
			-Wdeprecated -Wunreachable-code -Wduplicated-branches \
			-Wduplicated-cond -Wswitch-enum \
			-Wundef -Wunused -Wshadow -Wredundant-decls \
			-Wdouble-promotion -Wfloat-equal -Wpointer-arith \
			-Wcast-qual -Wcast-align -Wconversion -Wold-style-definition \
			-Wvla -Wformat=2 -Wlogical-op -Wnull-dereference \
			-Werror=return-local-addr -Werror=return-type \
			-isystem ./include/deps

LDFLAGS :=

TARGET := build/cchat
BUILDDIR := build

# Header only
DEPS := include/deps/clay.h

SRCS := src/main.c
OBJS := ${SRCS:%.c=${BUILDDIR}/%.o}


${TARGET}: ${OBJS} ${BUILDDIR}/deps.o
	@ echo "Linking..."
	@ mkdir -p $(dir $@)
	@ ${CC} ${LDFLAGS} $^ -o $@

${BUILDDIR}/%.o: %.c
	@ echo "Compiling ${<}..."
	@ mkdir -p $(dir $@)
	@ ${CC} ${CFLAGS} -MD $< -c -o $@

${BUILDDIR}/deps.o: ${DEPS}
	@ echo "Compiling Header-Only Dependencies..."
	@ mkdir -p $(dir $@)
	@ ${CC} -c -x c ${CFLAGS} ${LDFLAGS} $^ -o $@

run: ${TARGET}
	./${TARGET}

clean:
	rm -rf ${TARGET} ${OBJS} ${OBJS:.o=.d} ${BUILDDIR}/deps.o

clean_all: clean
	rm -rf calls.strace compile_flags.txt

format:
	clang-format -i ${SRCS}

disasm: ${TARGET}
	@ objdump -dC -M intel $< | bat --style=plain --wrap=never -l asm

raw: ${TARGET}
	@ hexyl $< | bat --style=plain

trace: calls.strace
	@ # This 30 is for the first 30 lines in a tipical C glibc program
	@ tail --lines=+30 $^ | bat --style=plain --wrap=never -l strace

calls.strace: ${TARGET}
	@ echo "Tracing..."
	@- strace -o $@ ./${TARGET}

compile_flags.txt: Makefile
	echo ${CFLAGS} | tr ' ' '\n' > $@


-include $(OBJS:.o=.d)
