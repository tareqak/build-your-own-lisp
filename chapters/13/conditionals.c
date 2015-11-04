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

    lisp_builtin builtin;
    lisp_environment* environment;
    lisp_value* formals;
    lisp_value* body;

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
    lisp_environment* parent; /* Do not delete parent */
    size_t count;
    char** symbols;
    lisp_value** values;
};

lisp_value* lisp_value_builtin(lisp_builtin const builtin) {
    lisp_value* value = malloc(sizeof(lisp_value));
    value->type = LISP_VALUE_FUNCTION;
    value->builtin = builtin;
    return value;
}

lisp_value* lisp_value_number(const long x) {
    lisp_value* value = malloc(sizeof(lisp_value));
    value->type = LISP_VALUE_NUMBER;
    value->number = x;
    return value;
}

lisp_value* lisp_value_error(const char* const fmt, ...) {
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

lisp_value* lisp_value_symbol(const char* const s) {
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

void lisp_environment_delete(lisp_environment* const environment);

void lisp_value_delete(lisp_value* const value) {
    switch (value->type) {
        case LISP_VALUE_NUMBER:
            break;
        case LISP_VALUE_FUNCTION:
            if (value->builtin == NULL) {
                lisp_environment_delete(value->environment);
                lisp_value_delete(value->formals);
                lisp_value_delete(value->body);
            }
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

lisp_environment* lisp_environment_copy(
    const lisp_environment* const environment);

lisp_value* lisp_value_copy(const lisp_value* const value) {
    lisp_value* x = malloc(sizeof(lisp_value));
    x->type = value->type;
    switch (value->type) {
        case LISP_VALUE_FUNCTION:
            if (value->builtin != NULL) {
                x->builtin = value->builtin;
            } else {
                x->builtin = NULL;
                x->environment = lisp_environment_copy(value->environment);
                x->formals = lisp_value_copy(value->formals);
                x->body = lisp_value_copy(value->body);
            }
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

lisp_environment* const lisp_environment_new() {
    lisp_environment* environment = malloc(sizeof(lisp_environment));
    environment->parent = NULL;
    environment->count = 0;
    environment->symbols = NULL;
    environment->values = NULL;
    return environment;
}

void lisp_environment_delete(lisp_environment* const environment) {
    for (size_t i = 0; i < environment->count; i += 1) {
        free(environment->symbols[i]);
        lisp_value_delete(environment->values[i]);
    }
    /* Do not delete environment->parent */
    free(environment->symbols);
    free(environment->values);
    free(environment);
}

lisp_environment* lisp_environment_copy(
    const lisp_environment* const environment) {
    lisp_environment* new_environment = malloc(sizeof(lisp_environment));
    new_environment->parent = environment->parent;
    new_environment->count = environment->count;
    new_environment->symbols = malloc(sizeof(char*) * environment->count);
    new_environment->values = malloc(sizeof(lisp_value*) * environment->count);
    for (size_t i = 0; i < environment->count; i += 1) {
        size_t symbol_length = strlen(environment->symbols[i]) + 1;
        new_environment->symbols[i] = malloc(symbol_length);
        strncpy(new_environment->symbols[i], environment->symbols[i],
                symbol_length);
        new_environment->values[i] = lisp_value_copy(environment->values[i]);
    }
    return new_environment;
}

lisp_value* lisp_environment_get(const lisp_environment* const environment,
                                 const lisp_value* const key) {
    for (size_t i = 0; i < environment->count; i += 1) {
        if (strcmp(environment->symbols[i], key->symbol) == 0) {
            return lisp_value_copy(environment->values[i]);
        }
    }
    if (environment->parent != NULL) {
        return lisp_environment_get(environment->parent, key);
    }
    return lisp_value_error("Unbound symbol '%s'.", key->symbol);
}

void lisp_environment_put(lisp_environment* environment,
                          const lisp_value* const key,
                          const lisp_value* const value) {
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

void lisp_environment_def(lisp_environment* environment,
                          const lisp_value* const key,
                          const lisp_value* const value) {
    /* Define in global environment */
    while (environment->parent != NULL) {
        environment = environment->parent;
    }
    lisp_environment_put(environment, key, value);
}

const char* lisp_type_name(const int type) {
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

lisp_value* lisp_value_lambda(lisp_value* const formals,
                              lisp_value* const body) {
    lisp_value* value = malloc(sizeof(lisp_value));
    value->type = LISP_VALUE_FUNCTION;
    value->builtin = NULL;
    value->environment = lisp_environment_new();
    value->formals = formals;
    value->body = body;
    return value;
}

lisp_value* lisp_value_read_number(const mpc_ast_t* const t) {
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lisp_value_number(x)
                           : lisp_value_error("Invalid number.");
}

lisp_value* lisp_value_add(lisp_value* value, lisp_value* const x) {
    value->count += 1;
    value->cell = realloc(value->cell, sizeof(lisp_value*) * value->count);
    value->cell[value->count - 1] = x;
    return value;
}

lisp_value* lisp_value_read(const mpc_ast_t* const t) {
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

void lisp_value_print(const lisp_value* const value);

void lisp_value_expression_print(const lisp_value* const value, char open,
                                 char close) {
    putchar(open);
    for (size_t i = 0; i < value->count; i += 1) {
        lisp_value_print(value->cell[i]);
        if (i != (value->count - 1)) {
            putchar(' ');
        }
    }
    putchar(close);
}

void lisp_value_print(const lisp_value* const value) {
    switch (value->type) {
        case LISP_VALUE_NUMBER:
            printf("%li", value->number);
            break;
        case LISP_VALUE_FUNCTION:
            if (value->builtin != NULL) {
                printf("<builtin>");
            } else {
                printf("(\\ ");
                lisp_value_print(value->formals);
                putchar(' ');
                lisp_value_print(value->body);
                putchar(')');
            }
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

void lisp_value_println(const lisp_value* const value) {
    lisp_value_print(value);
    putchar('\n');
}

lisp_value* lisp_value_pop(lisp_value* const value, size_t i) {
    lisp_value* x = value->cell[i];

    /* Shift memory after the item at "i" over the top */
    memmove(&value->cell[i], &value->cell[i + 1],
            sizeof(lisp_value*) * (value->count - i - 1));

    value->count -= 1;

    /* Reallocate the memory used */
    value->cell = realloc(value->cell, sizeof(lisp_value*) * value->count);
    return x;
}

lisp_value* lisp_value_take(lisp_value* const value, size_t i) {
    lisp_value* x = lisp_value_pop(value, i);
    lisp_value_delete(value);
    return x;
}

lisp_value* builtin_head(lisp_environment* const environment,
                         lisp_value* const arguments) {
    if (arguments->count > 1) {
        lisp_value* error = lisp_value_error(
            "Function 'head' passed too many arguments. Expected 1. Got %li.",
            arguments->count);
        lisp_value_delete(arguments);
        return error;
    }
    if (arguments->cell[0]->type != LISP_VALUE_QEXPRESSION) {
        lisp_value* error = lisp_value_error(
            "Function 'head' passed an incorrect type '%s'. Expected "
            "Q-Expression.",
            lisp_type_name(arguments->cell[0]->type));
        lisp_value_delete(arguments);
        return error;
    }
    if (arguments->cell[0]->count == 0) {
        lisp_value_delete(arguments);
        return lisp_value_error("Function 'head' passed {}.");
    }
    lisp_value* value = lisp_value_take(arguments, 0);
    while (value->count > 1) {
        lisp_value_delete(lisp_value_pop(value, 1));
    }
    return value;
}

lisp_value* builtin_tail(lisp_environment* const environment,
                         lisp_value* const arguments) {
    if (arguments->count > 1) {
        lisp_value* error = lisp_value_error(
            "Function 'tail' passed too many arguments. Expected 1. Got %li.",
            arguments->count);
        lisp_value_delete(arguments);
        return error;
    }
    if (arguments->cell[0]->type != LISP_VALUE_QEXPRESSION) {
        lisp_value* error = lisp_value_error(
            "Function 'tail' passed an incorrect type '%s'. Expected "
            "Q-Expression.",
            lisp_type_name(arguments->cell[0]->type));
        lisp_value_delete(arguments);
        return error;
    }
    if (arguments->cell[0]->count == 0) {
        lisp_value_delete(arguments);
        return lisp_value_error("Function 'tail' passed {}.");
    }
    lisp_value* value = lisp_value_take(arguments, 0);
    lisp_value_delete(lisp_value_pop(value, 0));
    return value;
}

lisp_value* builtin_list(lisp_environment* const environment,
                         lisp_value* const arguments) {
    arguments->type = LISP_VALUE_QEXPRESSION;
    return arguments;
}

lisp_value* lisp_value_evaluate(lisp_environment* const environment,
                                lisp_value* const value);

lisp_value* builtin_eval(lisp_environment* const environment,
                         lisp_value* const arguments) {
    if (arguments->count > 1) {
        lisp_value* error = lisp_value_error(
            "Function 'eval' passed too many arguments. Expected 1. Got %li.",
            arguments->count);
        lisp_value_delete(arguments);
        return error;
    }
    if (arguments->cell[0]->type != LISP_VALUE_QEXPRESSION) {
        lisp_value* error = lisp_value_error(
            "Function 'eval' passed incorrect type '%s'. Expected "
            "Q-Expression.",
            lisp_type_name(arguments->cell[0]->type));
        lisp_value_delete(arguments);
        return error;
    }
    lisp_value* x = lisp_value_take(arguments, 0);
    x->type = LISP_VALUE_SEXPRESSION;
    return lisp_value_evaluate(environment, x);
}

lisp_value* builtin_join(lisp_environment* const environment,
                         lisp_value* const arguments) {
    for (size_t i = 0; i < arguments->count; i += 1) {
        if (arguments->cell[i]->type != LISP_VALUE_QEXPRESSION) {
            lisp_value* error = lisp_value_error(
                "Function 'join' passed incorrect type '%s'. Expected "
                "Q-Expression.",
                lisp_type_name(arguments->cell[i]->type));
            lisp_value_delete(arguments);
            return error;
        }
    }
    lisp_value* x = lisp_value_pop(arguments, 0);
    while (arguments->count > 0) {
        lisp_value* y = lisp_value_pop(arguments, 0);
        for (size_t i = 0; i < y->count; i += 1) {
            x = lisp_value_add(x, y->cell[i]);
        }
        /* We don't want to get rid off the y->cell[i]'s, so we cannot call
         * lisp_value_delete(y)
         */
        free(y->cell);
        free(y);
    }
    lisp_value_delete(arguments);
    return x;
}

lisp_value* builtin_op(lisp_environment* const environment,
                       lisp_value* const arguments, const char* const op) {
    for (size_t i = 0; i < arguments->count; i += 1) {
        if (arguments->cell[i]->type != LISP_VALUE_NUMBER) {
            lisp_value* error =
                lisp_value_error("Cannot operate on '%s'. Expected Number.",
                                 lisp_type_name(arguments->cell[i]->type));
            lisp_value_delete(arguments);
            return error;
        }
    }

    lisp_value* x = lisp_value_pop(arguments, 0);

    if ((strcmp(op, "-") == 0) && arguments->count == 0) {
        x->number = -x->number;
    }

    while (arguments->count > 0) {
        lisp_value* y = lisp_value_pop(arguments, 0);
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
    lisp_value_delete(arguments);
    return x;
}

lisp_value* builtin_add(lisp_environment* const environment,
                        lisp_value* const arguments) {
    return builtin_op(environment, arguments, "+");
}

lisp_value* builtin_sub(lisp_environment* const environment,
                        lisp_value* const arguments) {
    return builtin_op(environment, arguments, "-");
}

lisp_value* builtin_mul(lisp_environment* const environment,
                        lisp_value* const arguments) {
    return builtin_op(environment, arguments, "*");
}

lisp_value* builtin_div(lisp_environment* const environment,
                        lisp_value* const arguments) {
    return builtin_op(environment, arguments, "/");
}

lisp_value* builtin_mod(lisp_environment* const environment,
                        lisp_value* const arguments) {
    return builtin_op(environment, arguments, "%");
}

lisp_value* builtin_lambda(lisp_environment* const environment,
                           lisp_value* const arguments) {
    if (arguments->count != 2) {
        lisp_value* error = lisp_value_error(
            "Function '\\' expects 2 arguments. Got %li.", arguments->count);
        lisp_value_delete(arguments);
        return error;
    }
    if (arguments->cell[0]->type != LISP_VALUE_QEXPRESSION) {
        lisp_value* error = lisp_value_error(
            "Function '\\' expects first argument to be Q-Expression. Got "
            "'%s'.",
            lisp_type_name(arguments->cell[0]->type));
        lisp_value_delete(arguments);
        return error;
    }
    if (arguments->cell[1]->type != LISP_VALUE_QEXPRESSION) {
        lisp_value* error = lisp_value_error(
            "Function '\\' expects second argument to be Q-Expression. Got "
            "'%s'.",
            lisp_type_name(arguments->cell[1]->type));
        lisp_value_delete(arguments);
        return error;
    }
    for (size_t i = 0; i < arguments->cell[0]->count; i += 1) {
        if (arguments->cell[0]->cell[i]->type != LISP_VALUE_SYMBOL) {
            lisp_value* error = lisp_value_error(
                "Cannot define non-symbol. Expected Symbol. Got '%s'.",
                lisp_type_name(arguments->cell[0]->cell[i]->type));
            lisp_value_delete(arguments);
            return error;
        }
    }
    lisp_value* formals = lisp_value_pop(arguments, 0);
    lisp_value* body = lisp_value_pop(arguments, 0);
    lisp_value_delete(arguments);
    return lisp_value_lambda(formals, body);
}

lisp_value* builtin_var(lisp_environment* const environment,
                        lisp_value* const arguments,
                        const char* const function) {
    if (arguments->cell[0]->type != LISP_VALUE_QEXPRESSION) {
        lisp_value* error = lisp_value_error(
            "Function '%s' expects a Q-Expression for its first argument. "
            "Got '%s'.",
            function, lisp_type_name(arguments->cell[0]->type));
        lisp_value_delete(arguments);
        return error;
    }
    lisp_value* symbols = arguments->cell[0];
    if (symbols->count != arguments->count - 1) {
        lisp_value* error = lisp_value_error(
            "Function '%s' passed incorrect number of argument. Expected "
            "%li. Got %li.",
            function, arguments->count - 1, symbols->count);
        lisp_value_delete(arguments);
        return error;
    }
    for (size_t i = 0; i < symbols->count; i += 1) {
        if (symbols->cell[i]->type != LISP_VALUE_SYMBOL) {
            lisp_value* error = lisp_value_error(
                "Function '%s' cannot define non-symbol. Expected 'Symbol'. "
                "Got '%s'.",
                function, symbols->cell[i]->type);
            lisp_value_delete(arguments);
            return error;
        }
    }
    if (strcmp(function, "def") == 0) {
        for (size_t i = 0; i < symbols->count; i += 1) {
            lisp_environment_def(environment, symbols->cell[i],
                                 arguments->cell[i + 1]);
        }
    } else if (strcmp(function, "=") == 0) {
        for (size_t i = 0; i < symbols->count; i += 1) {
            lisp_environment_put(environment, symbols->cell[i],
                                 arguments->cell[i + 1]);
        }
    }
    lisp_value_delete(arguments);
    return lisp_value_sexpression();
}

lisp_value* builtin_def(lisp_environment* const environment,
                        lisp_value* const arguments) {
    return builtin_var(environment, arguments, "def");
}

lisp_value* builtin_put(lisp_environment* const environment,
                        lisp_value* const arguments) {
    return builtin_var(environment, arguments, "=");
}

lisp_value* builtin_order(lisp_environment* const environment,
                          lisp_value* const arguments, const char* const op) {
    if (arguments->count != 2) {
        lisp_value* error =
            lisp_value_error("Function '%s' expects 2 arguments. Got %li.", op,
                             arguments->count);
        lisp_value_delete(arguments);
        return error;
    }
    if (arguments->cell[0]->type != LISP_VALUE_NUMBER) {
        lisp_value* error = lisp_value_error(
            "Function '%s' expects a Number for its first argument. Got '%s'.",
            op, lisp_type_name(arguments->cell[0]->type));
        lisp_value_delete(arguments);
        return error;
    }
    if (arguments->cell[1]->type != LISP_VALUE_NUMBER) {
        lisp_value* error = lisp_value_error(
            "Function '%s' expects a Number for its second argument. Got '%s'.",
            op, lisp_type_name(arguments->cell[1]->type));
        lisp_value_delete(arguments);
        return error;
    }
    int r;
    if (strcmp(op, ">") == 0) {
        r = arguments->cell[0]->number > arguments->cell[1]->number;
    } else if (strcmp(op, "<") == 0) {
        r = arguments->cell[0]->number < arguments->cell[1]->number;
    } else if (strcmp(op, ">=") == 0) {
        r = arguments->cell[0]->number >= arguments->cell[1]->number;
    } else if (strcmp(op, "<=") == 0) {
        r = arguments->cell[0]->number <= arguments->cell[1]->number;
    } else {
        lisp_value_delete(arguments);
        return lisp_value_error("Unknown function order function '%s'.", op);
    }
    lisp_value_delete(arguments);
    return lisp_value_number(r);
}

lisp_value* builtin_greater_than(lisp_environment* const environment,
                                 lisp_value* const arguments) {
    return builtin_order(environment, arguments, ">");
}

lisp_value* builtin_lesser_than(lisp_environment* const environment,
                                lisp_value* const arguments) {
    return builtin_order(environment, arguments, "<");
}

lisp_value* builtin_greater_than_or_equal_to(
    lisp_environment* const environment, lisp_value* const arguments) {
    return builtin_order(environment, arguments, ">=");
}

lisp_value* builtin_lesser_than_or_equal_to(lisp_environment* const environment,
                                            lisp_value* const arguments) {
    return builtin_order(environment, arguments, "<=");
}

int lisp_value_equal(lisp_value* x, lisp_value* y) {
    if (x->type != y->type) {
        return 0;
    }

    switch (x->type) {
        case LISP_VALUE_NUMBER:
            return x->number == y->number;
        case LISP_VALUE_ERROR:
            return strcmp(x->error, y->error) == 0;
        case LISP_VALUE_SYMBOL:
            return strcmp(x->symbol, y->symbol) == 0;
        case LISP_VALUE_FUNCTION:
            if (x->builtin || y->builtin) {
                return x->builtin == y->builtin;
            } else {
                return lisp_value_equal(x->formals, y->formals) &&
                       lisp_value_equal(x->body, y->body);
            }
        case LISP_VALUE_SEXPRESSION:
        case LISP_VALUE_QEXPRESSION:
            if (x->count != y->count) {
                return 0;
            }
            for (size_t i = 0; i < x->count; i += 1) {
                if (!lisp_value_equal(x->cell[i], y->cell[i])) {
                    return 0;
                }
            }
            return 1;
    }
    return 0;
}

lisp_value* builtin_compare(lisp_environment* const environment,
                            lisp_value* const arguments, const char* const op) {
    if (arguments->count != 2) {
        lisp_value* error =
            lisp_value_error("Function '%s' expects 2 arguments. Got %li.", op,
                             arguments->count);
        lisp_value_delete(arguments);
        return error;
    }
    int r;
    if (strcmp(op, "==") == 0) {
        r = lisp_value_equal(arguments->cell[0], arguments->cell[1]);
    } else if (strcmp(op, "!=") == 0) {
        r = !lisp_value_equal(arguments->cell[0], arguments->cell[1]);
    } else {
        lisp_value_delete(arguments);
        return lisp_value_error("Function '%s' not found.");
    }
    lisp_value_delete(arguments);
    return lisp_value_number(r);
}

lisp_value* builtin_equal(lisp_environment* const environment,
                          lisp_value* const arguments) {
    return builtin_compare(environment, arguments, "==");
}

lisp_value* builtin_not_equal(lisp_environment* const environment,
                              lisp_value* const arguments) {
    return builtin_compare(environment, arguments, "!=");
}

lisp_value* builtin_if(lisp_environment* const environment,
                       lisp_value* const arguments) {
    if (arguments->count != 3) {
        lisp_value* error = lisp_value_error(
            "Function 'if' expects 3 arguments. Got %li.", arguments->count);
        lisp_value_delete(arguments);
        return error;
    }
    if (arguments->cell[0]->type != LISP_VALUE_NUMBER) {
        lisp_value* error = lisp_value_error(
            "Function 'if' expects a Number for its first argument. Got '%s'.",
            lisp_type_name(arguments->cell[0]->type));
        lisp_value_delete(arguments);
        return error;
    }
    if (arguments->cell[1]->type != LISP_VALUE_QEXPRESSION) {
        lisp_value* error = lisp_value_error(
            "Function 'if' expects a Q-Expression for its second argument. Got "
            "'%s'.",
            lisp_type_name(arguments->cell[1]->type));
        lisp_value_delete(arguments);
        return error;
    }
    if (arguments->cell[2]->type != LISP_VALUE_QEXPRESSION) {
        lisp_value* error = lisp_value_error(
            "Function 'if' expects a Q-Expression for its third argument. Got "
            "'%s'.",
            lisp_type_name(arguments->cell[2]->type));
        lisp_value_delete(arguments);
        return error;
    }

    lisp_value* x;
    if (arguments->cell[0]->number) {
        arguments->cell[1]->type = LISP_VALUE_SEXPRESSION;
        x = lisp_value_evaluate(environment, lisp_value_pop(arguments, 1));
    } else {
        arguments->cell[2]->type = LISP_VALUE_SEXPRESSION;
        x = lisp_value_evaluate(environment, lisp_value_pop(arguments, 2));
    }
    lisp_value_delete(arguments);
    return x;
}

lisp_value* lisp_value_call(lisp_environment* const environment,
                            lisp_value* const function,
                            lisp_value* const arguments) {
    if (function->builtin != NULL) {
        return function->builtin(environment, arguments);
    }
    size_t given = arguments->count;
    size_t total = function->formals->count;
    while (arguments->count > 0) {
        if (function->formals->count == 0) {
            lisp_value* error = lisp_value_error(
                "Function passed too many arguments. Expected %li. Got %li.",
                total, given);
            lisp_value_delete(arguments);
            return error;
        }
        lisp_value* symbol = lisp_value_pop(function->formals, 0);

        /* Special case to deal with '&' */
        if (strcmp(symbol->symbol, "&") == 0) {
            if (function->formals->count != 1) {
                lisp_value_delete(arguments);
                return lisp_value_error(
                    "Function format invalid. Symbol '&' not followed by "
                    "single symbol.");
            }
            lisp_value* next_symbol = lisp_value_pop(function->formals, 0);
            lisp_environment_put(function->environment, next_symbol,
                                 builtin_list(environment, arguments));
            lisp_value_delete(symbol);
            lisp_value_delete(next_symbol);
            break;
        }

        lisp_value* value = lisp_value_pop(arguments, 0);
        lisp_environment_put(function->environment, symbol, value);
        lisp_value_delete(symbol);
        lisp_value_delete(value);
    }
    lisp_value_delete(arguments);

    /* If '&' remains in formal list, bind to empty list */
    if (function->formals->count > 0 &&
        strcmp(function->formals->cell[0]->symbol, "&") == 0) {
        if (function->formals->count != 2) {
            return lisp_value_error(
                "Function format invalid. Symbol '&' not followed by single "
                "symbol.");
        }
        lisp_value_delete(lisp_value_pop(function->formals, 0));
        lisp_value* symbol = lisp_value_pop(function->formals, 0);
        lisp_value* value = lisp_value_qexpression();
        lisp_environment_put(function->environment, symbol, value);
        lisp_value_delete(symbol);
        lisp_value_delete(value);
    }

    if (function->formals->count == 0) {
        function->environment->parent = environment;
        return builtin_eval(function->environment,
                            lisp_value_add(lisp_value_sexpression(),
                                           lisp_value_copy(function->body)));
    } else {
        /* Return partially evaluated function */
        return lisp_value_copy(function);
    }
}

lisp_value* lisp_value_evaluate_sexpression(lisp_environment* const environment,
                                            lisp_value* const value) {
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
            "S-expression must start with a function. Got '%s'",
            lisp_type_name(first->type));
        lisp_value_delete(first);
        return error;
    }

    lisp_value* result = lisp_value_call(environment, first, value);
    lisp_value_delete(first);
    return result;
}

lisp_value* lisp_value_evaluate(lisp_environment* const environment,
                                lisp_value* const value) {
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

void lisp_environment_add_builtin(lisp_environment* const environment,
                                  const char* const name,
                                  lisp_builtin const builtin) {
    lisp_value* key = lisp_value_symbol(name);
    lisp_value* value = lisp_value_builtin(builtin);
    lisp_environment_put(environment, key, value);
    lisp_value_delete(key);
    lisp_value_delete(value);
}

void lisp_environment_add_builtins(lisp_environment* const environment) {
    lisp_environment_add_builtin(environment, "\\", builtin_lambda);
    lisp_environment_add_builtin(environment, "lambda", builtin_lambda);
    lisp_environment_add_builtin(environment, "def", builtin_def);
    lisp_environment_add_builtin(environment, "put", builtin_put);
    lisp_environment_add_builtin(environment, "=", builtin_put);

    lisp_environment_add_builtin(environment, "list", builtin_list);
    lisp_environment_add_builtin(environment, "head", builtin_head);
    lisp_environment_add_builtin(environment, "tail", builtin_tail);
    lisp_environment_add_builtin(environment, "join", builtin_join);
    lisp_environment_add_builtin(environment, "eval", builtin_eval);

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

    lisp_environment_add_builtin(environment, "if", builtin_if);
    lisp_environment_add_builtin(environment, "==", builtin_equal);
    lisp_environment_add_builtin(environment, "eq", builtin_equal);
    lisp_environment_add_builtin(environment, "!=", builtin_not_equal);
    lisp_environment_add_builtin(environment, "ne", builtin_not_equal);
    lisp_environment_add_builtin(environment, ">", builtin_greater_than);
    lisp_environment_add_builtin(environment, "gt", builtin_greater_than);
    lisp_environment_add_builtin(environment, "<", builtin_lesser_than);
    lisp_environment_add_builtin(environment, "lt", builtin_lesser_than);
    lisp_environment_add_builtin(environment, ">=",
                                 builtin_greater_than_or_equal_to);
    lisp_environment_add_builtin(environment, "ge",
                                 builtin_greater_than_or_equal_to);
    lisp_environment_add_builtin(environment, "<=",
                                 builtin_lesser_than_or_equal_to);
    lisp_environment_add_builtin(environment, "le",
                                 builtin_lesser_than_or_equal_to);
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
    puts("Lispy Version 00.00.09");
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
