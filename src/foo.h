#ifndef FOO_H
#define FOO_H

/**********************************
 *   Foo Interpreter
 *   Copyright (C) 2025 CoccusQ
 *   MIT License
 **********************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define F_MAX_STACK 1024
#define F_MAX_LOOP 64
#define F_MAX_WORD 64
#define F_MAX_EXPR 256
#define F_MAX_DICT 256
#define F_MAX_VARS 512

#define F_MSG "Foo, Copyright (C) 2025 CoccusQ.\nInteractive Mode.\nType `bye` to exit"

typedef enum F_Type {
    F_PRIMITIVE, F_CONTROL,
    F_FUNCTION, F_VARIABLE
} F_Type;

typedef struct F_State F_State;

typedef struct F_DictEntry {
    char word[F_MAX_WORD];
    union {
        char expr[F_MAX_EXPR];
        void (*func)(F_State *);
        void (*control)(F_State *, const char *, int *);
        int var_index;
    };
    F_Type type;
} F_DictEntry;

typedef struct F_Dict {
    F_DictEntry *entry;
    int *vars;
    int var_size;
    int size;
} F_Dict;

typedef struct {
    int *stack;
    int capacity;
    int size;
} F_Stack;

struct F_State {
    F_Dict *dict;
    F_Stack *data;
    F_Stack *loop;
    FILE *input;
    char line_buf[F_MAX_EXPR * 2];
    char word_buf[F_MAX_WORD];
    char expr_buf[F_MAX_EXPR];
    int line_count;
    int running;
    int interactive;
};

F_Dict *F_createDict() {
    F_Dict *dict = (F_Dict *) malloc(sizeof(F_Dict));
    dict->entry = (F_DictEntry *) calloc(F_MAX_DICT, sizeof(F_DictEntry));
    dict->vars = (int *) calloc(F_MAX_VARS, sizeof(int));
    dict->var_size = 0;
    dict->size = 0;
    return dict;
}

void F_destroyDict(F_Dict *dict) {
    free(dict->entry);
    free(dict->vars);
    free(dict);
}

F_DictEntry *F_find(F_State *state, const char *word) {
    F_Dict *dict = state->dict;
    for (int i = 0; i < dict->size; i++) {
        if (!strcmp(word, dict->entry[i].word))
            return &dict->entry[i];
    }
    return NULL;
}

void F_addExpr(F_State *state, const char *word, const char *expr) {
    F_DictEntry *cur = F_find(state, word);
    F_Dict *dict = state->dict;
    if (!cur) cur = &dict->entry[dict->size];
    else if (state->interactive)
        printf("[INFO] Redefined word `%s` at line %d\n", word, state->line_count);
    strcpy(cur->word, word);
    strcpy(cur->expr, expr);
    cur->type = F_FUNCTION;
    dict->size++;
}

void F_addFunc(F_State *state, const char *word, void (*func)(F_State *)) {
    F_Dict *dict = state->dict;
    F_DictEntry *cur = &dict->entry[dict->size];
    strcpy(cur->word, word);
    cur->func = func;
    cur->type = F_PRIMITIVE;
    dict->size++;
}

void F_addControl(F_State *state, const char *word, void (*control)(F_State *, const char *, int *)) {
    F_Dict *dict = state->dict;
    F_DictEntry *cur = F_find(state, word);
    if (!cur) cur = &dict->entry[dict->size];
    else if (state->interactive)
        printf("[INFO] Redefined word `%s` at line %d\n", word, state->line_count);
    strcpy(cur->word, word);
    cur->control = control;
    cur->type = F_CONTROL;
    dict->size++;
}

void F_addVar(F_State *state, const char *word, int val) {
    F_Dict *dict = state->dict;
    F_DictEntry *cur = &dict->entry[dict->size];
    strcpy(cur->word, word);
    cur->var_index = dict->var_size++;
    dict->vars[cur->var_index] = val;
    cur->type = F_VARIABLE;
    dict->size++;
}

F_Stack *F_createStack(int capacity) {
    F_Stack *stk = (F_Stack *) malloc(sizeof(F_Stack));
    stk->stack = (int *) calloc(capacity, sizeof(int));
    stk->capacity = capacity;
    stk->size = 0;
    return stk;
}

void F_destroyStack(F_Stack *stk) {
    free(stk->stack);
    free(stk);
}

void F_pushValue(F_Stack *stk, int val) {
    stk->stack[stk->size++] = val;
}

int F_popValue(F_Stack *stk) {
    return stk->stack[--stk->size];
}

int F_topValue(F_Stack *stk) {
    return stk->stack[stk->size - 1];
}

void F_push(F_State *state, int value) {
    if (state->data->size >= state->data->capacity) {
        fprintf(stderr, "[ERROR] Stack overflow at line %d\n", state->line_count);
        if (!state->interactive) state->running = 0;
        else state->data->size = 0;
        return;
    }
    F_pushValue(state->data, value);
}

int F_pop(F_State *state) {
    if (state->data->size <= 0) {
        fprintf(stderr, "[ERROR] Stack underflow at line %d\n", state->line_count);
        if (!state->interactive) state->running = 0;
        else state->data->size = 0;
        return 0;
    }
    return F_popValue(state->data);
}

int F_top(F_State *state) {
    if (state->data->size <= 0) {
        fprintf(stderr, "[ERROR] Stack underflow at line %d\n", state->line_count);
        if (!state->interactive) state->running = 0;
        else state->data->size = 0;
        return 0;
    }
    return F_topValue(state->data);
}

F_State *F_createState() {
    F_State *state = (F_State *) malloc(sizeof(F_State));
    state->dict = F_createDict();
    state->data = F_createStack(F_MAX_STACK);
    state->loop = F_createStack(F_MAX_LOOP);
    state->input = stdin;
    state->line_count = 0;
    state->running = 1;
    state->interactive = 1;
    return state;
}

void F_destroyState(F_State *state) {
    F_destroyDict(state->dict);
    F_destroyStack(state->data);
    F_destroyStack(state->loop);
    free(state);
}

void F_eval(F_State *state, char *s);

void F_parseNum(F_State *state, const char *str, int *pos) {
    if (!state->running) return;
    int x = 0, f = 1;
    char c = str[*pos];
    while (!isdigit(c)) {
        if (c == '-') f = -1;
        c = str[++(*pos)];
    }
    while (isdigit(c)) {
        x = x * 10 + c - '0';
        c = str[++(*pos)];
    }
    F_push(state, x * f);
}

void F_parseChar(F_State *state, const char *str, int *pos) {
    if (!state->running) return;
    (*pos)++;
    int c = (int) str[(*pos)++];
    if (str[*pos] != '\'') {
        fprintf(stderr, "[ERROR] Invalid input at line %d\n", state->line_count);
        return;
    } else F_push(state, c);
    while (str[*pos] != ' ' && str[*pos] != '\n' && str[*pos] != '\r') (*pos)++;
}

void F_parseWord(F_State *state, const char *str, int *pos) {
    if (!state->running) return;
    int word_idx = 0;
    while (str[*pos] != ' ' && str[*pos] != '\0')
        state->word_buf[word_idx++] = str[(*pos)++];
    state->word_buf[word_idx] = '\0';
    for (int i = 0; i < state->dict->size; i++) {
        if (!strcmp(state->word_buf, state->dict->entry[i].word)) {
            switch (state->dict->entry[i].type) {
                case F_VARIABLE:
                    F_push(state, state->dict->entry[i].var_index);
                    break;
                case F_FUNCTION:
                    F_eval(state, state->dict->entry[i].expr);
                    break;
                case F_PRIMITIVE:
                    if (state->dict->entry[i].func)
                        state->dict->entry[i].func(state);
                    break;
                case F_CONTROL:
                    if (state->dict->entry[i].control)
                        state->dict->entry[i].control(state, str, pos);
                    break;
            }
            return;
        }
    }
    fprintf(stderr, "[ERROR] Undefined word `%s` at line %d\n", state->word_buf, state->line_count);
    if (!state->interactive) state->running = 0;
    else state->data->size = 0;
}

void F_eval(F_State *state, char *s) {
    if (!state->running) return;
    for (int i = 0; s[i] != '\0';) {
        if (s[i] == ' ') {
            i++;
            continue;
        }
        if (isdigit(s[i]) || (s[i] == '-' && isdigit(s[i + 1]))) F_parseNum(state, s, &i);
        else if (s[i] == '\'' && isprint(s[i + 1])) F_parseChar(state, s, &i);
        else F_parseWord(state, s, &i);
    }
}

void F_compile(F_State *state, char *s) {
    if (!state->running) return;
    int i = 1, word_idx = 0, expr_idx = 0;
    while (s[i] == ' ') i++;
    while (s[i] != ' ') state->word_buf[word_idx++] = s[i++];
    state->word_buf[word_idx] = '\0';
    while (s[i] == ' ') i++;
    while (s[i] != ';') state->expr_buf[expr_idx++] = s[i++];
    state->expr_buf[expr_idx] = '\0';
    F_addExpr(state, state->word_buf, state->expr_buf);
}

int F_read(F_State *state) {
    int c = fgetc(state->input), len = 0;
    while (c != '\n' && c != '\r' && c != EOF)
        state->line_buf[len++] = c, c = fgetc(state->input);
    state->line_buf[len] = '\0';
    char *comment = strstr(state->line_buf, "//");
    if (comment != NULL) *comment = '\0';
    return c;
}

void F_write(int x) {
    if (x < 0) putchar('-'), x = -x;
    if (x > 9) F_write(x / 10);
    putchar(x % 10 + '0');
}

void F_add(F_State *state) {
    int b = F_pop(state);
    int a = F_pop(state);
    F_push(state, a + b);
}

void F_sub(F_State *state) {
    int b = F_pop(state);
    int a = F_pop(state);
    F_push(state, a - b);
}

void F_mul(F_State *state) {
    int b = F_pop(state);
    int a = F_pop(state);
    F_push(state, a * b);
}

void F_div(F_State *state) {
    int b = F_pop(state);
    if (b == 0) {
        fprintf(stderr, "[ERROR] Division by zero at line %d\n", state->line_count);
        if (!state->interactive) state->running = 0;
        else {
            F_push(state, b);
            fprintf(stderr, "Traceback...\n");
        }
        return;
    }
    int a = F_pop(state);
    F_push(state, a / b);
}

void F_mod(F_State *state) {
    int b = F_pop(state);
    if (b == 0) {
        fprintf(stderr, "[ERROR] Division by zero at line %d\n", state->line_count);
        if (!state->interactive) state->running = 0;
        else {
            F_push(state, b);
            fprintf(stderr, "Traceback...\n");
        }
        return;
    }
    int a = F_pop(state);
    F_push(state, a % b);
}

void F_greater(F_State *state) {
    int b = F_pop(state);
    int a = F_pop(state);
    F_push(state, a > b);
}

void F_less(F_State *state) {
    int b = F_pop(state);
    int a = F_pop(state);
    F_push(state, a < b);
}

void F_greater_equal(F_State *state) {
    int b = F_pop(state);
    int a = F_pop(state);
    F_push(state, a >= b);
}

void F_less_equal(F_State *state) {
    int b = F_pop(state);
    int a = F_pop(state);
    F_push(state, a <= b);
}

void F_equal(F_State *state) {
    int b = F_pop(state);
    int a = F_pop(state);
    F_push(state, a == b);
}

void F_not_equal(F_State *state) {
    int b = F_pop(state);
    int a = F_pop(state);
    F_push(state, a != b);
}

void F_pop_stack(F_State *state) {
    if (state->data->size <= 0) {
        fprintf(stderr, "[ERROR] Stack underflow at line %d\n", state->line_count);
        if (!state->interactive) state->running = 0;
        else state->data->size = 0;
        return;
    }
    int val = F_popValue(state->data);
    F_write(val), putchar('\n');
}

void F_print_stack(F_State *state) {
    printf("<%d> ", state->data->size);
    for (int i = 0; i < state->data->size; i++)
        F_write(state->data->stack[i]), putchar(' ');
    putchar('\n');
}

void F_dup(F_State *state) {
    F_push(state, F_top(state));
}

void F_if(F_State *state, const char *s, int *pos) {
    int condition = F_pop(state);
    if (condition) return;
    int depth = 1, i = *pos;
    while (depth > 0 && s[i] != '\0') {
        while (s[i] == ' ') i++;
        int start = i;
        while (s[i] != ' ' && s[i] != '\0') i++;
        char next_word[F_MAX_WORD];
        strncpy(next_word, s + start, i - start);
        next_word[i - start] = '\0';
        if (!strcmp(next_word, "if")) depth++;
        else if (!strcmp(next_word, "then")) depth--;
        else if (!strcmp(next_word, "else") && depth == 1) depth--;
    }
    *pos = i;
}

void F_else(F_State *state, const char *s, int *pos) {
    int depth = 1, i = *pos;
    while (depth > 0 && s[i] != '\0') {
        while (s[i] == ' ') i++;
        int start = i;
        while (s[i] != ' ' && s[i] != '\0') i++;
        char next_word[F_MAX_WORD];
        strncpy(next_word, s + start, i - start);
        next_word[i - start] = '\0';
        if (!strcmp(next_word, "if")) depth++;
        else if (!strcmp(next_word, "then")) depth--;
    }
    *pos = i;
}

void F_begin(F_State *state, const char *s, int *pos) {
    if (state->loop->size >= state->loop->capacity) {
        fprintf(stderr, "[ERROR] Loop stack overflow at line %d\n", state->line_count);
        state->running = 0;
        return;
    }
    state->loop->stack[state->loop->size++] = *pos;
}

void F_until(F_State *state, const char *s, int *pos) {
    if (state->loop->size == 0) {
        fprintf(stderr, "Unmatched `until` at line %d\n", state->line_count);
        state->running = 0;
        return;
    }
    int condition = F_pop(state);
    if (!condition) *pos = state->loop->stack[state->loop->size - 1];
    else state->loop->size--;
}

void F_var(F_State *state, const char *s, int *pos) {
    if (state->dict->var_size >= F_MAX_VARS) {
        fprintf(stderr, "[ERROR] Variable limit reached at line %d\n", state->line_count);
        state->running = 0;
        return;
    }
    int i = *pos, word_idx = 0;
    while (s[i] == ' ') i++;
    while (s[i] != ' ' && s[i] != '\0') state->word_buf[word_idx++] = s[i++];
    state->word_buf[word_idx] = '\0';
    *pos = i;
    F_addVar(state, state->word_buf, 0);
}

void F_fetch(F_State *state) {
    int var_idx = F_pop(state);
    F_push(state, state->dict->vars[var_idx]);
}

void F_store(F_State *state) {
    int var_idx = F_pop(state);
    int value = F_pop(state);
    state->dict->vars[var_idx] = value;
}

void F_query(F_State *state) {
    int var_idx = F_pop(state);
    F_write(state->dict->vars[var_idx]), putchar('\n');
}

void F_increase(F_State *state) {
    int var_idx = F_pop(state);
    state->dict->vars[var_idx]++;
}

void F_decrease(F_State *state) {
    int var_idx = F_pop(state);
    state->dict->vars[var_idx]--;
}

void F_add_store(F_State *state) {
    int var_idx = F_pop(state);
    int x = F_pop(state);
    state->dict->vars[var_idx] += x;
}

void F_sub_store(F_State *state) {
    int var_idx = F_pop(state);
    int x = F_pop(state);
    state->dict->vars[var_idx] -= x;
}

void F_emit(F_State *state) {
    putchar(F_pop(state));
}

void F_bye(F_State *state) {
    state->running = 0;
}

void F_initState(F_State *state) {
    F_addFunc(state, "+", F_add);
    F_addFunc(state, "-", F_sub);
    F_addFunc(state, "*", F_mul);
    F_addFunc(state, "/", F_div);
    F_addFunc(state, "%", F_mod);

    F_addFunc(state, ">", F_greater);
    F_addFunc(state, "<", F_less);
    F_addFunc(state, ">=", F_greater_equal);
    F_addFunc(state, "<=", F_less_equal);
    F_addFunc(state, "==", F_equal);
    F_addFunc(state, "~=", F_not_equal);

    F_addFunc(state, ".", F_pop_stack);
    F_addFunc(state, ".s", F_print_stack);
    F_addFunc(state, "dup", F_dup);

    F_addControl(state, "if", F_if);
    F_addControl(state, "else", F_else);
    F_addControl(state, "then", NULL);
    F_addControl(state, "begin", F_begin);
    F_addControl(state, "until", F_until);

    F_addControl(state, "var", F_var);
    F_addFunc(state, "@", F_fetch);
    F_addFunc(state, "!", F_store);
    F_addFunc(state, "?", F_query);
    F_addFunc(state, "++", F_increase);
    F_addFunc(state, "--", F_decrease);
    F_addFunc(state, "+!", F_add_store);
    F_addFunc(state, "-!", F_sub_store);

    F_addFunc(state, "emit", F_emit);
    F_addFunc(state, "bye", F_bye);
}

void F_execScript(F_State *state, const char *filename) {
    if (filename) {
        state->input = fopen(filename, "r");
        state->interactive = 0;
    } else puts(F_MSG);
    while (state->running && F_read(state) != EOF) {
        state->line_count++;
        if (state->line_buf[0] == ':') F_compile(state, state->line_buf);
        else F_eval(state, state->line_buf);
    }
}
#endif //FOO_H
