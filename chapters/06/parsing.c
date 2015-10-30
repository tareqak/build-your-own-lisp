#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>

#include "mpc/mpc.h"

int main() {
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Operator = mpc_new("operator");
    mpc_parser_t* Expr = mpc_new("expr");
    mpc_parser_t* Tlisp = mpc_new("tlisp");

    mpca_lang(MPCA_LANG_DEFAULT,
            "\
            number: /-?[0-9]+/ ;\
            operator: '+' | '-' | '*' | '/' | '%';\
            expr: <number> | '(' <operator> <expr>+ ')';\
            tlisp: /^/ <operator> <expr>+ /$/;\
            ", Number, Operator, Expr, Tlisp);

    puts("Tareq Lisp Version 00.00.01");
    puts("Press Ctrl+c to Exit\n");
    for (;;) {
        char* input = readline("tlisp> ");
        add_history(input);
        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Tlisp, &r)) {
            mpc_ast_print(r.output);
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }
        free(input);
    }

    mpc_cleanup(4, Number, Operator, Expr, Tlisp);
    return 0;
}
