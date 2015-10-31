CC := cc
CFLAGS := -std=c99 -Wall -Werror
LIBS = -ledit -lm
SOURCES = error_handling.c mpc/mpc.c
HEADERS = mpc/mpc.h
EXE = error_handling

${EXE}: ${SOURCES} ${HEADERS}
	${CC} ${CFLAGS} ${SOURCES} ${LIBS} -o $@

all: ${EXE}

clean:
	rm -f ${EXE}
