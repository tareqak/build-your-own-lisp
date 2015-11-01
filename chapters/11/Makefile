CC := cc
CFLAGS := -std=c99 -Wall -Werror -g
LIBS = -ledit -lm
EXE = variables
SOURCES = ${EXE}.c mpc/mpc.c
HEADERS = mpc/mpc.h

${EXE}: ${SOURCES} ${HEADERS}
	${CC} ${CFLAGS} ${SOURCES} ${LIBS} -o $@

all: ${EXE}

clean:
	rm -fr ${EXE} ${EXE}.dSYM
