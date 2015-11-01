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
    LISP_VALUE_QEXPRESSION,
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

lisp_value* lisp_value_qexpression() {
    lisp_value* value = malloc(sizeof(lisp_value));
    value->type = LISP_VALUE_QEXPRESSION;
    value->count = 0;
    value->cell = NULL;
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
        case LISP_VALUE_QEXPRESSION:
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
    } else if (strstr(t->tag, "qexpression")) {
        x = lisp_value_qexpression();
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
        case LISP_VALUE_QEXPRESSION:
            lisp_value_expression_print(value, '{', '}');
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

lisp_value* builtin_head(lisp_value* a) {
    if (a->count > 1) {
        lisp_value_delete(a);
        return lisp_value_error("Function 'head' passed too many arguments.");
    }
    if (a->cell[0]->type != LISP_VALUE_QEXPRESSION) {
        lisp_value_delete(a);
        return lisp_value_error("Function 'head' passed an incorrect type.");
    }
    if (a->cell[0]->count == 0) {
        lisp_value_delete(a);
        return lisp_value_error("Function 'head' passed {}.");
    }
    lisp_value* value = lisp_value_take(a, 0);
    while (value->count > 1) {
        lisp_value_delete(lisp_value_pop(value, 1));
    }
    return value;
}

lisp_value* builtin_tail(lisp_value* a) {
    if (a->count > 1) {
        lisp_value_delete(a);
        return lisp_value_error("Function 'tail' passed too many arguments.");
    }
    if (a->cell[0]->type != LISP_VALUE_QEXPRESSION) {
        lisp_value_delete(a);
        return lisp_value_error("Function 'tail' passed an incorrect type.");
    }
    if (a->cell[0]->count == 0) {
        lisp_value_delete(a);
        return lisp_value_error("Function 'tail' passed {}.");
    }
    lisp_value* value = lisp_value_take(a, 0);
    lisp_value_delete(lisp_value_pop(value, 0));
    return value;
}

lisp_value* builtin_list(lisp_value* a) {
    a->type = LISP_VALUE_QEXPRESSION;
    return a;
}

lisp_value* lisp_value_evaluate(lisp_value* value);

lisp_value* builtin_eval(lisp_value* a) {
    if (a->count > 1) {
        lisp_value_delete(a);
        return lisp_value_error("Function 'eval' passed too many arguments.");
    }
    if (a->cell[0]->type != LISP_VALUE_QEXPRESSION) {
        lisp_value_delete(a);
        return lisp_value_error("Function 'eval' passed incorrect type.");
    }
    lisp_value* x = lisp_value_take(a, 0);
    x->type = LISP_VALUE_SEXPRESSION;
    return lisp_value_evaluate(x);
}

lisp_value* builtin_join(lisp_value* a) {
    for (size_t i = 0; i < a->count; i += 1) {
        if (a->cell[i]->type != LISP_VALUE_QEXPRESSION) {
            lisp_value_delete(a);
            return lisp_value_error("Function 'join' passed incorrect type.");
        }
    }
    lisp_value* x = lisp_value_pop(a, 0);
    while (a->count > 1) {
        lisp_value* y = lisp_value_pop(a, 0);
        while (y->count > 1) {
            x = lisp_value_add(x, lisp_value_pop(y, 0));
        }
        lisp_value_delete(y);
    }
    lisp_value_delete(a);
    return x;
}

lisp_value* builtin_op(lisp_value* a, const char* op) {
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

lisp_value* builtin(lisp_value* a, const char* function) {
    if (strcmp("list", function) == 0) {
        return builtin_list(a);
    }
    if (strcmp("head", function) == 0) {
        return builtin_head(a);
    }
    if (strcmp("tail", function) == 0) {
        return builtin_tail(a);
    }
    if (strcmp("join", function) == 0) {
        return builtin_join(a);
    }
    if (strcmp("eval", function) == 0) {
        return builtin_eval(a);
    }
    if (strstr("+-/*%", function)) {
        return builtin_op(a, function);
    }
    lisp_value_delete(a);
    return lisp_value_error("Unknown function.");
}

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

    lisp_value* result = builtin(value, first->symbol);
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
    mpc_parser_t* Qexpression = mpc_new("qexpression");
    mpc_parser_t* Sexpression = mpc_new("sexpression");
    mpc_parser_t* Expression = mpc_new("expression");
    mpc_parser_t* Tlisp = mpc_new("tlisp");

    mpca_lang(MPCA_LANG_DEFAULT,
              "\
            number: /-?[0-9]+/ ;\
            symbol: \"list\" | \"head\" | \"tail\" | \"join\" | \"eval\" \
            | '+' | '-' | '*' | '/' | '%';\
            qexpression: '{' <expression>* '}';\
            sexpression: '(' <expression>* ')';\
            expression: <number> | <symbol> | <sexpression> | <qexpression> ;\
            tlisp: /^/ <expression>* /$/;\
            ",
              Number, Symbol, Qexpression, Sexpression, Expression, Tlisp);

    puts("Tareq Lisp Version 00.00.06");
    puts("Press Ctrl+c to Exit\n");
    for (;;) {
        char* input = readline("tlisp> ");
        add_history(input);
        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Tlisp, &r)) {
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

    mpc_cleanup(6, Number, Symbol, Qexpression, Sexpression, Expression, Tlisp);
    return 0;
}
