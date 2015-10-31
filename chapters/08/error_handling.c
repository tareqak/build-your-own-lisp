#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <editline/readline.h>

#include "mpc/mpc.h"

typedef struct {
    int type;
    long number;
    int error;
} lisp_value;

enum { LISP_VALUE_NUMBER, LISP_VALUE_ERROR };
enum {
    LISP_ERROR_DIVISION_BY_ZERO,
    LISP_ERROR_BAD_OPERATOR,
    LISP_ERROR_BAD_NUMBER
};

lisp_value lisp_value_number(long x) {
    lisp_value value;
    value.type = LISP_VALUE_NUMBER;
    value.number = x;
    return value;
}

lisp_value lisp_value_error(int x) {
    lisp_value value;
    value.type = LISP_VALUE_ERROR;
    value.error = x;
    return value;
}

void lisp_value_print(lisp_value value) {
    switch (value.type) {
        case LISP_VALUE_NUMBER:
            printf("%li", value.number);
            break;
        case LISP_VALUE_ERROR:
            switch (value.error) {
                case LISP_ERROR_DIVISION_BY_ZERO:
                    printf("Error: Division by zero.");
                    break;
                case LISP_ERROR_BAD_OPERATOR:
                    printf("Error: Invalid operator.");
                    break;
                case LISP_ERROR_BAD_NUMBER:
                    printf("Error: Invalid number.");
                    break;
            }
            break;
    }
}

void lisp_value_println(lisp_value value) {
    lisp_value_print(value);
    putchar('\n');
}

lisp_value eval_op(lisp_value x, char* op, lisp_value y) {
    if (x.type == LISP_VALUE_ERROR) {
        return x;
    }
    if (y.type == LISP_VALUE_ERROR) {
        return y;
    }
    if (strcmp(op, "+") == 0) {
        return lisp_value_number(x.number + y.number);
    }
    if (strcmp(op, "-") == 0) {
        return lisp_value_number(x.number - y.number);
    }
    if (strcmp(op, "*") == 0) {
        return lisp_value_number(x.number * y.number);
    }
    if (strcmp(op, "/") == 0) {
        return y.number != 0 ? lisp_value_number(x.number / y.number)
                             : lisp_value_error(LISP_ERROR_DIVISION_BY_ZERO);
    }
    if (strcmp(op, "%") == 0) {
        return y.number != 0 ? lisp_value_number(x.number % y.number)
                             : lisp_value_error(LISP_ERROR_DIVISION_BY_ZERO);
    }
    return lisp_value_error(LISP_ERROR_BAD_OPERATOR);
}

lisp_value eval(mpc_ast_t* t) {
    if (strstr(t->tag, "number")) {
        errno = 0;
        long x = strtol(t->contents, NULL, 10);
        return errno != ERANGE ? lisp_value_number(x)
                               : lisp_value_error(LISP_ERROR_BAD_NUMBER);
    }

    char* op = t->children[1]->contents;
    lisp_value x = eval(t->children[2]);

    for (size_t i = 3; strstr(t->children[i]->tag, "expr"); ++i) {
        x = eval_op(x, op, eval(t->children[i]));
    }

    return x;
}

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
            ",
              Number, Operator, Expr, Tlisp);

    puts("Tareq Lisp Version 00.00.04");
    puts("Press Ctrl+c to Exit\n");
    for (;;) {
        char* input = readline("tlisp> ");
        add_history(input);
        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Tlisp, &r)) {
            lisp_value result = eval(r.output);
            lisp_value_println(result);
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
