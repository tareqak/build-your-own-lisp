#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>

int main() {
    puts("Lispy Version 00.00.01");
    puts("Press Ctrl+c to Exit\n");
    for (;;) {
        char* input = readline("lispy> ");
        add_history(input);
        printf("You said: %s\n", input);
        free(input);
    }
    return 0;
}
