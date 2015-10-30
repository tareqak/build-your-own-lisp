CC := cc
CFLAGS := -std=c99 -Wall -Werror
SOURCES = hello_world.c
EXE = hello_world

${EXE}: ${SOURCES}
	${CC} ${CFLAGS} ${SOURCES} -o $@

all: ${EXE}

clean:
	rm -f ${EXE}
