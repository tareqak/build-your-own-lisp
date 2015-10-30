CC := cc
CFLAGS := -std=c99 -Wall -Werror
LIBS = -ledit
SOURCES = prompt.c
EXE = prompt

${EXE}: ${SOURCES}
	${CC} ${CFLAGS} ${SOURCES} ${LIBS} -o $@

all: ${EXE}

clean:
	rm -f ${EXE}
