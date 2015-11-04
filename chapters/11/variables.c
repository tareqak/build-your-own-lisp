#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <editline/readline.h>

#include "mpc/mpc.h"

struct lisp_value;
typedef struct lisp_value lisp_value;

struct lisp_environment;
typedef struct lisp_environment lisp_environment;

typedef lisp_value* (*lisp_builtin)(lisp_environment*, lisp_value*);

struct lisp_value {
    int type;
    long number;
    char* error;
    char* symbol;
    lisp_builtin function;
    size_t count;
    lisp_value** cell;
};

enum {
    LISP_VALUE_NUMBER,
    LISP_VALUE_ERROR,
    LISP_VALUE_SYMBOL,
    LISP_VALUE_QEXPRESSION,
    LISP_VALUE_SEXPRESSION,
    LISP_VALUE_FUNCTION
};

struct lisp_environment {
    size_t count;
    char** symbols;
    lisp_value** values;
};

lisp_value* lisp_value_function(lisp_builtin function) {
    lisp_value* value = malloc(sizeof(lisp_value));
    value->type = LISP_VALUE_FUNCTION;
    value->function = function;
    return value;
}

lisp_value* lisp_value_number(long x) {
    lisp_value* value = malloc(sizeof(lisp_value));
    value->type = LISP_VALUE_NUMBER;
    value->number = x;
    return value;
}

lisp_value* lisp_value_error(char* fmt, ...) {
    lisp_value* value = malloc(sizeof(lisp_value));
    value->type = LISP_VALUE_ERROR;

    /* Create a va list and initialize it */
    va_list va;
    va_start(va, fmt);

    value->error = malloc(512);

    /* printf the error string with a maximum of 511 characters */
    vsnprintf(value->error, 511, fmt, va);

    /* Reallocate to number of bytes actually used */
    value->error = realloc(value->error, strlen(value->error) + 1);

    /* Cleanup our va list */
    va_end(va);

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
        case LISP_VALUE_FUNCTION:
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

lisp_value* lisp_value_copy(lisp_value* value) {
    lisp_value* x = malloc(sizeof(lisp_value));
    x->type = value->type;
    switch (value->type) {
        case LISP_VALUE_FUNCTION:
            x->function = value->function;
            break;
        case LISP_VALUE_NUMBER:
            x->number = value->number;
            break;
        case LISP_VALUE_ERROR: {
            size_t size = strlen(value->error) + 1;
            x->error = malloc(size);
            strncpy(x->error, value->error, size);
            break;
        }
        case LISP_VALUE_SYMBOL: {
            size_t size = strlen(value->symbol) + 1;
            x->symbol = malloc(size);
            strncpy(x->symbol, value->symbol, size);
            break;
        }
        case LISP_VALUE_QEXPRESSION:
        case LISP_VALUE_SEXPRESSION:
            x->count = value->count;
            x->cell = malloc(sizeof(lisp_value*) * value->count);
            for (size_t i = 0; i < value->count; i += 1) {
                x->cell[i] = lisp_value_copy(value->cell[i]);
            }
            break;
    }
    return x;
}

lisp_environment* lisp_environment_new() {
    lisp_environment* environment = malloc(sizeof(lisp_environment));
    environment->count = 0;
    environment->symbols = NULL;
    environment->values = NULL;
    return environment;
}

void lisp_environment_delete(lisp_environment* environment) {
    for (size_t i = 0; i < environment->count; i += 1) {
        free(environment->symbols[i]);
        lisp_value_delete(environment->values[i]);
    }
    free(environment->symbols);
    free(environment->values);
    free(environment);
}

lisp_value* lisp_environment_get(lisp_environment* environment,
                                 lisp_value* key) {
    for (size_t i = 0; i < environment->count; i += 1) {
        if (strcmp(environment->symbols[i], key->symbol) == 0) {
            return lisp_value_copy(environment->values[i]);
        }
    }
    return lisp_value_error("Unbound symbol '%s'.", key->symbol);
}

const char* lisp_type_name(int type) {
    switch (type) {
        case LISP_VALUE_FUNCTION:
            return "Function";
        case LISP_VALUE_NUMBER:
            return "Number";
        case LISP_VALUE_ERROR:
            return "Error";
        case LISP_VALUE_SYMBOL:
            return "Symbol";
        case LISP_VALUE_SEXPRESSION:
            return "S-Expression";
        case LISP_VALUE_QEXPRESSION:
            return "Q-Expression";
        default:
            return "Unknown";
    }
}

void lisp_environment_put(lisp_environment* environment, lisp_value* key,
                          lisp_value* value) {
    for (size_t i = 0; i < environment->count; i += 1) {
        if (strcmp(environment->symbols[i], key->symbol) == 0) {
            lisp_value_delete(environment->values[i]);
            environment->values[i] = lisp_value_copy(value);
            return;
        }
    }
    size_t last = environment->count;
    environment->count += 1;
    environment->values =
        realloc(environment->values, sizeof(lisp_value*) * environment->count);
    environment->symbols =
        realloc(environment->symbols, sizeof(char*) * environment->count);

    environment->values[last] = lisp_value_copy(value);
    size_t symbol_length = strlen(key->symbol) + 1;
    environment->symbols[last] = malloc(symbol_length);
    strncpy(environment->symbols[last], key->symbol, symbol_length);
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
        case LISP_VALUE_FUNCTION:
            printf("<function>");
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

lisp_value* builtin_head(lisp_environment* environment, lisp_value* a) {
    if (a->count > 1) {
        lisp_value* error = lisp_value_error(
            "Function 'head' passed too many arguments. Got %li. Expected "
            "1.",
            a->count);
        lisp_value_delete(a);
        return error;
    }
    if (a->cell[0]->type != LISP_VALUE_QEXPRESSION) {
        lisp_value* error = lisp_value_error(
            "Function 'head' passed an incorrect type '%s'. Expected "
            "Q-Expression.",
            lisp_type_name(a->cell[0]->type));
        lisp_value_delete(a);
        return error;
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

lisp_value* builtin_tail(lisp_environment* environment, lisp_value* a) {
    if (a->count > 1) {
        lisp_value* error = lisp_value_error(
            "Function 'tail' passed too many arguments. Got %li. Expected "
            "1.",
            a->count);
        lisp_value_delete(a);
        return error;
    }
    if (a->cell[0]->type != LISP_VALUE_QEXPRESSION) {
        lisp_value* error = lisp_value_error(
            "Function 'tail' passed an incorrect type '%s'. Expected "
            "Q-Expression.",
            lisp_type_name(a->cell[0]->type));
        lisp_value_delete(a);
        return error;
    }
    if (a->cell[0]->count == 0) {
        lisp_value_delete(a);
        return lisp_value_error("Function 'tail' passed {}.");
    }
    lisp_value* value = lisp_value_take(a, 0);
    lisp_value_delete(lisp_value_pop(value, 0));
    return value;
}

lisp_value* builtin_list(lisp_environment* environment, lisp_value* a) {
    a->type = LISP_VALUE_QEXPRESSION;
    return a;
}

lisp_value* lisp_value_evaluate(lisp_environment* environment,
                                lisp_value* value);

lisp_value* builtin_eval(lisp_environment* environment, lisp_value* a) {
    if (a->count > 1) {
        lisp_value* error = lisp_value_error(
            "Function 'eval' passed too many arguments. Got %li. Expected 1.",
            a->count);
        lisp_value_delete(a);
        return error;
    }
    if (a->cell[0]->type != LISP_VALUE_QEXPRESSION) {
        lisp_value* error = lisp_value_error(
            "Function 'eval' passed incorrect type '%s'. Expected "
            "Q-Expression.",
            lisp_type_name(a->cell[0]->type));
        lisp_value_delete(a);
        return error;
    }
    lisp_value* x = lisp_value_take(a, 0);
    x->type = LISP_VALUE_SEXPRESSION;
    return lisp_value_evaluate(environment, x);
}

lisp_value* builtin_join(lisp_environment* environment, lisp_value* a) {
    for (size_t i = 0; i < a->count; i += 1) {
        if (a->cell[i]->type != LISP_VALUE_QEXPRESSION) {
            lisp_value* error = lisp_value_error(
                "Function 'join' passed incorrect type '%s'. Expected "
                "Q-Expression.",
                lisp_type_name(a->cell[i]->type));
            lisp_value_delete(a);
            return error;
        }
    }
    lisp_value* x = lisp_value_pop(a, 0);
    while (a->count > 0) {
        lisp_value* y = lisp_value_pop(a, 0);
        while (y->count > 0) {
            x = lisp_value_add(x, lisp_value_pop(y, 0));
        }
        lisp_value_delete(y);
    }
    lisp_value_delete(a);
    return x;
}

lisp_value* builtin_op(lisp_environment* environment, lisp_value* a,
                       const char* op) {
    for (size_t i = 0; i < a->count; i += 1) {
        if (a->cell[i]->type != LISP_VALUE_NUMBER) {
            lisp_value* error =
                lisp_value_error("Cannot operate on '%s'. Expected Number.",
                                 lisp_type_name(a->cell[i]->type));
            lisp_value_delete(a);
            return error;
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

lisp_value* builtin_add(lisp_environment* environment, lisp_value* a) {
    return builtin_op(environment, a, "+");
}

lisp_value* builtin_sub(lisp_environment* environment, lisp_value* a) {
    return builtin_op(environment, a, "-");
}

lisp_value* builtin_mul(lisp_environment* environment, lisp_value* a) {
    return builtin_op(environment, a, "*");
}

lisp_value* builtin_div(lisp_environment* environment, lisp_value* a) {
    return builtin_op(environment, a, "/");
}

lisp_value* builtin_mod(lisp_environment* environment, lisp_value* a) {
    return builtin_op(environment, a, "%");
}

lisp_value* lisp_value_evaluate_sexpression(lisp_environment* environment,
                                            lisp_value* value) {
    if (value->count == 0) {
        return value;
    }

    for (size_t i = 0; i < value->count; i += 1) {
        value->cell[i] = lisp_value_evaluate(environment, value->cell[i]);
        if (value->cell[i]->type == LISP_VALUE_ERROR) {
            return lisp_value_take(value, i);
        }
    }

    if (value->count == 1) {
        return lisp_value_take(value, 0);
    }

    lisp_value* first = lisp_value_pop(value, 0);
    if (first->type != LISP_VALUE_FUNCTION) {
        lisp_value_delete(value);
        lisp_value* error = lisp_value_error(
            "S-expression must start with a function. Found '%s'",
            lisp_type_name(first->type));
        lisp_value_delete(first);
        return error;
    }

    lisp_value* result = first->function(environment, value);
    lisp_value_delete(first);
    return result;
}

lisp_value* lisp_value_evaluate(lisp_environment* environment,
                                lisp_value* value) {
    if (value->type == LISP_VALUE_SYMBOL) {
        lisp_value* x = lisp_environment_get(environment, value);
        lisp_value_delete(value);
        return x;
    }
    if (value->type == LISP_VALUE_SEXPRESSION) {
        return lisp_value_evaluate_sexpression(environment, value);
    }
    return value;
}

lisp_value* builtin_def(lisp_environment* environment, lisp_value* a) {
    if (a->cell[0]->type != LISP_VALUE_QEXPRESSION) {
        lisp_value* error = lisp_value_error(
            "Function 'def' passed incorrect type '%s'. Expected "
            "Q-Expression.",
            lisp_type_name(a->cell[0]->type));
        lisp_value_delete(a);
        return error;
    }
    lisp_value* symbols = a->cell[0];
    for (size_t i = 0; i < symbols->count; i += 1) {
        if (symbols->cell[i]->type != LISP_VALUE_SYMBOL) {
            lisp_value* error = lisp_value_error(
                "Function 'def' cannot define non-symbol. Found '%s'.",
                lisp_type_name(symbols->cell[i]->type));
            lisp_value_delete(a);
            return error;
        }
    }
    if (symbols->count != a->count - 1) {
        lisp_value* error = lisp_value_error(
            "Function 'def' cannot define incorrect number of values to "
            "symbols. %li and %li.",
            symbols->count, a->count - 1);
        lisp_value_delete(a);
        return error;
    }
    for (size_t i = 0; i < symbols->count; i += 1) {
        lisp_environment_put(environment, symbols->cell[i], a->cell[i + 1]);
    }
    lisp_value_delete(a);
    return lisp_value_sexpression();
}

void lisp_environment_add_builtin(lisp_environment* environment, char* name,
                                  lisp_builtin function) {
    lisp_value* key = lisp_value_symbol(name);
    lisp_value* value = lisp_value_function(function);
    lisp_environment_put(environment, key, value);
    lisp_value_delete(key);
    lisp_value_delete(value);
}

void lisp_environment_add_builtins(lisp_environment* environment) {
    lisp_environment_add_builtin(environment, "list", builtin_list);
    lisp_environment_add_builtin(environment, "head", builtin_head);
    lisp_environment_add_builtin(environment, "tail", builtin_tail);
    lisp_environment_add_builtin(environment, "join", builtin_join);
    lisp_environment_add_builtin(environment, "eval", builtin_eval);
    lisp_environment_add_builtin(environment, "def", builtin_def);

    lisp_environment_add_builtin(environment, "add", builtin_add);
    lisp_environment_add_builtin(environment, "+", builtin_add);
    lisp_environment_add_builtin(environment, "sub", builtin_sub);
    lisp_environment_add_builtin(environment, "-", builtin_sub);
    lisp_environment_add_builtin(environment, "mul", builtin_mul);
    lisp_environment_add_builtin(environment, "*", builtin_mul);
    lisp_environment_add_builtin(environment, "div", builtin_div);
    lisp_environment_add_builtin(environment, "/", builtin_div);
    lisp_environment_add_builtin(environment, "mod", builtin_mod);
    lisp_environment_add_builtin(environment, "%", builtin_mod);
}

int main() {
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Symbol = mpc_new("symbol");
    mpc_parser_t* Qexpression = mpc_new("qexpression");
    mpc_parser_t* Sexpression = mpc_new("sexpression");
    mpc_parser_t* Expression = mpc_new("expression");
    mpc_parser_t* Lispy = mpc_new("lispy");

    mpca_lang(MPCA_LANG_DEFAULT,
              "\
            number: /-?[0-9]+/ ;\
            symbol: /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/;\
            qexpression: '{' <expression>* '}';\
            sexpression: '(' <expression>* ')';\
            expression: <number> | <symbol> | <sexpression> | <qexpression> ;\
            lispy: /^/ <expression>* /$/;\
            ",
              Number, Symbol, Qexpression, Sexpression, Expression, Lispy);

    lisp_environment* environment = lisp_environment_new();
    lisp_environment_add_builtins(environment);
    puts("Lispy Version 00.00.07");
    puts("Press Ctrl+c to Exit\n");
    for (;;) {
        char* input = readline("lispy> ");
        add_history(input);
        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lispy, &r)) {
            lisp_value* x =
                lisp_value_evaluate(environment, lisp_value_read(r.output));
            lisp_value_println(x);
            lisp_value_delete(x);
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }
        free(input);
    }
    lisp_environment_delete(environment);

    mpc_cleanup(6, Number, Symbol, Qexpression, Sexpression, Expression, Lispy);
    return 0;
}
