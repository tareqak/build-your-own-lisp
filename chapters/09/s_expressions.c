#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <editline/readline.h>

#include "mpc/mpc.h"

typedef struct lisp_value {
    int type;
    long number;
    char* error;
    char* symbol;
    size_t count;
    struct lisp_value** cell;
} lisp_value;

enum {
    LISP_VALUE_NUMBER,
    LISP_VALUE_ERROR,
    LISP_VALUE_SYMBOL,
    LISP_VALUE_SEXPRESSION
};

enum {
    LISP_ERROR_DIVISION_BY_ZERO,
    LISP_ERROR_BAD_OPERATOR,
    LISP_ERROR_BAD_NUMBER
};

lisp_value* lisp_value_number(long x) {
    lisp_value* value = malloc(sizeof(lisp_value));
    value->type = LISP_VALUE_NUMBER;
    value->number = x;
    return value;
}

lisp_value* lisp_value_error(char* m) {
    lisp_value* value = malloc(sizeof(lisp_value));
    value->type = LISP_VALUE_ERROR;
    size_t size = strlen(m) + 1;
    value->error = malloc(size);
    strncpy(value->error, m, size);
    return value;
}

lisp_value* lisp_value_symbol(char* s) {
    lisp_value* value = malloc(sizeof(lisp_value));
    value->type = LISP_VALUE_SYMBOL;
    size_t size = strlen(s) + 1;
    value->symbol = malloc(size);
    strncpy(value->symbol, s, size);
    return value;
}

lisp_value* lisp_value_sexpression() {
    lisp_value* value = malloc(sizeof(lisp_value));
    value->type = LISP_VALUE_SEXPRESSION;
    value->count = 0;
    value->cell = NULL;
    return value;
}

void lisp_value_delete(lisp_value* value) {
    switch (value->type) {
        case LISP_VALUE_NUMBER:
            break;
        case LISP_VALUE_SYMBOL:
            free(value->symbol);
            break;
        case LISP_VALUE_ERROR:
            free(value->error);
            break;
        case LISP_VALUE_SEXPRESSION:
            for (size_t i = 0; i < value->count; i += 1) {
                lisp_value_delete(value->cell[i]);
            }
            free(value->cell);
            break;
    }
    free(value);
}

lisp_value* lisp_value_read_number(mpc_ast_t* t) {
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lisp_value_number(x)
                           : lisp_value_error("Invalid number.");
}

lisp_value* lisp_value_add(lisp_value* value, lisp_value* x) {
    value->count += 1;
    value->cell = realloc(value->cell, sizeof(lisp_value*) * value->count);
    value->cell[value->count - 1] = x;
    return value;
}

lisp_value* lisp_value_read(mpc_ast_t* t) {
    if (strstr(t->tag, "number")) {
        return lisp_value_read_number(t);
    }
    if (strstr(t->tag, "symbol")) {
        return lisp_value_symbol(t->contents);
    }

    lisp_value* x = NULL;
    if (strcmp(t->tag, ">") == 0) {
        x = lisp_value_sexpression();
    } else if (strstr(t->tag, "sexpression")) {
        x = lisp_value_sexpression();
    }

    for (size_t i = 0; i < t->children_num; i += 1) {
        if (strcmp(t->children[i]->contents, "(") == 0) {
            continue;
        }
        if (strcmp(t->children[i]->contents, ")") == 0) {
            continue;
        }
        if (strcmp(t->children[i]->contents, "}") == 0) {
            continue;
        }
        if (strcmp(t->children[i]->contents, "{") == 0) {
            continue;
        }
        if (strcmp(t->children[i]->tag, "regex") == 0) {
            continue;
        }
        x = lisp_value_add(x, lisp_value_read(t->children[i]));
    }
    return x;
}

void lisp_value_print(lisp_value* value);

void lisp_value_expression_print(lisp_value* value, char open, char close) {
    putchar(open);
    for (size_t i = 0; i < value->count; i += 1) {
        lisp_value_print(value->cell[i]);
        if (i != (value->count - 1)) {
            putchar(' ');
        }
    }
    putchar(close);
}

void lisp_value_print(lisp_value* value) {
    switch (value->type) {
        case LISP_VALUE_NUMBER:
            printf("%li", value->number);
            break;
        case LISP_VALUE_ERROR:
            printf("Error: %s", value->error);
            break;
        case LISP_VALUE_SYMBOL:
            printf("%s", value->symbol);
            break;
        case LISP_VALUE_SEXPRESSION:
            lisp_value_expression_print(value, '(', ')');
            break;
    }
}

void lisp_value_println(lisp_value* value) {
    lisp_value_print(value);
    putchar('\n');
}

lisp_value* lisp_value_pop(lisp_value* value, size_t i) {
    lisp_value* x = value->cell[i];

    /* Shift memory after the item at "i" over the top */
    memmove(&value->cell[i], &value->cell[i + 1],
            sizeof(lisp_value*) * (value->count - i - 1));

    value->count -= 1;

    /* Reallocate the memory used */
    value->cell = realloc(value->cell, sizeof(lisp_value*) * value->count);
    return x;
}

lisp_value* lisp_value_take(lisp_value* value, size_t i) {
    lisp_value* x = lisp_value_pop(value, i);
    lisp_value_delete(value);
    return x;
}

lisp_value* builtin_op(lisp_value* a, char* op) {
    for (size_t i = 0; i < a->count; i += 1) {
        if (a->cell[i]->type != LISP_VALUE_NUMBER) {
            lisp_value_delete(a);
            return lisp_value_error("Cannot operate on non-number.");
        }
    }

    lisp_value* x = lisp_value_pop(a, 0);

    if ((strcmp(op, "-") == 0) && a->count == 0) {
        x->number = -x->number;
    }

    while (a->count > 0) {
        lisp_value* y = lisp_value_pop(a, 0);
        if (strcmp(op, "+") == 0) {
            x->number += y->number;
        }
        if (strcmp(op, "-") == 0) {
            x->number -= y->number;
        }
        if (strcmp(op, "*") == 0) {
            x->number *= y->number;
        }
        if (strcmp(op, "/") == 0) {
            if (y->number == 0) {
                lisp_value_delete(x);
                lisp_value_delete(y);
                x = lisp_value_error("Division by zero.");
                break;
            }
            x->number /= y->number;
        }
        if (strcmp(op, "%") == 0) {
            if (y->number == 0) {
                lisp_value_delete(x);
                lisp_value_delete(y);
                x = lisp_value_error("Division by zero.");
                break;
            }
            x->number %= y->number;
        }
        lisp_value_delete(y);
    }
    lisp_value_delete(a);
    return x;
}

lisp_value* lisp_value_evaluate(lisp_value* value);

lisp_value* lisp_value_evaluate_sexpression(lisp_value* value) {
    if (value->count == 0) {
        return value;
    }

    for (size_t i = 0; i < value->count; i += 1) {
        value->cell[i] = lisp_value_evaluate(value->cell[i]);
        if (value->cell[i]->type == LISP_VALUE_ERROR) {
            return lisp_value_take(value, i);
        }
    }

    if (value->count == 1) {
        return lisp_value_take(value, 0);
    }

    lisp_value* first = lisp_value_pop(value, 0);
    if (first->type != LISP_VALUE_SYMBOL) {
        lisp_value_delete(first);
        lisp_value_delete(value);
        return lisp_value_error("S-expression does not start with a symbol.");
    }

    lisp_value* result = builtin_op(value, first->symbol);
    lisp_value_delete(first);
    return result;
}

lisp_value* lisp_value_evaluate(lisp_value* value) {
    return (value->type == LISP_VALUE_SEXPRESSION)
               ? lisp_value_evaluate_sexpression(value)
               : value;
}

int main() {
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Symbol = mpc_new("symbol");
    mpc_parser_t* Sexpression = mpc_new("sexpression");
    mpc_parser_t* Expression = mpc_new("expression");
    mpc_parser_t* Lispy = mpc_new("lispy");

    mpca_lang(MPCA_LANG_DEFAULT,
              "\
            number: /-?[0-9]+/ ;\
            symbol: '+' | '-' | '*' | '/' | '%';\
            sexpression: '(' <expression>* ')';\
            expression: <number> | <symbol> | <sexpression> ;\
            lispy: /^/ <expression>* /$/;\
            ",
              Number, Symbol, Sexpression, Expression, Lispy);

    puts("Lispy Version 00.00.05");
    puts("Press Ctrl+c to Exit\n");
    for (;;) {
        char* input = readline("lispy> ");
        add_history(input);
        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lispy, &r)) {
            lisp_value* x = lisp_value_evaluate(lisp_value_read(r.output));
            lisp_value_println(x);
            lisp_value_delete(x);
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }
        free(input);
    }

    mpc_cleanup(5, Number, Symbol, Sexpression, Expression, Lispy);
    return 0;
}
