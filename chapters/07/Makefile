CC := cc
CFLAGS := -std=c99 -Wall -Werror
LIBS = -ledit -lm
SOURCES = evaluation.c mpc/mpc.c
HEADERS = mpc/mpc.h
EXE = evaluation

${EXE}: ${SOURCES} ${HEADERS}
	${CC} ${CFLAGS} ${SOURCES} ${LIBS} -o $@

all: ${EXE}

clean:
	rm -f ${EXE}
