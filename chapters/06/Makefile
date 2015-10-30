CC := cc
CFLAGS := -std=c99 -Wall -Werror
LIBS = -ledit -lm
SOURCES = parsing.c mpc/mpc.c
HEADERS = mpc/mpc.h
EXE = parsing

${EXE}: ${SOURCES} ${HEADERS}
	${CC} ${CFLAGS} ${SOURCES} ${LIBS} -o $@

all: ${EXE}

clean:
	rm -f ${EXE}
