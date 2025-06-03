#ifndef FOO_H
#define FOO_H

/**********************************
 *   Foo
 *   Copyright (C) 2025 CoccusQ
 *   MIT License
 **********************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>

#define F_MAX_STACK 65536
#define F_MAX_LOOP 64
#define F_MAX_WORD 64
#define F_MAX_EXPR 512
#define F_MAX_DICT 512
#define F_MAX_VARS 512

#define F_MSG "Foo, Copyright (C) 2025 CoccusQ.\nInteractive Mode.\nType `bye` to exit"

typedef enum F_Type {
    F_PRIMITIVE,
    F_CONTROL,
    F_FUNCTION,
    F_VARIABLE,
    F_MODULE
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
    double *fvars;
    int fvar_size;
    int size;
} F_Dict;

typedef struct F_Stack {
    int *stack;
    int capacity;
    int size;
} F_Stack;

typedef struct F_FStack {
    double *stack;
    int capacity;
    int size;
} F_FStack;

struct F_State {
    F_Dict *dict;
    F_Stack *data;
    F_FStack *fdata;
    F_Stack *loop;
    FILE *input;
    char line_buf[F_MAX_EXPR * 2];
    char module_buf[F_MAX_EXPR * 2];
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
    dict->fvars = (double *) calloc(F_MAX_VARS, sizeof(double));
    dict->fvar_size = 0;
    dict->size = 0;
    return dict;
}

void F_destroyDict(F_Dict *dict) {
    free(dict->entry);
    free(dict->vars);
    free(dict->fvars);
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

void F_printDict(F_State *state) {
    F_Dict *dict = state->dict;
    for (int i = 0; i < dict->size; i++) {
        switch (dict->entry[i].type) {
            case F_PRIMITIVE:
                printf("<PRIMITIVE>: %s\n", dict->entry[i].word);
                break;
            case F_CONTROL:
                printf("<PRIMITIVE>: %s\n", dict->entry[i].word);
                break;
            case F_FUNCTION:
                printf("<FUNCTION>: %s\n\t%s\n;\n", dict->entry[i].word, dict->entry[i].expr);
                break;
            case F_VARIABLE:
                printf("<VARIABLE>: %s Address[%d]\n", dict->entry[i].word, dict->entry[i].var_index);
                break;
            case F_MODULE:
                printf("<MODULE>: %s\n", dict->entry[i].word);
                break;
        }
    }
} 

void F_printPrim(F_State *state) {
    F_Dict *dict = state->dict;
    int cnt = 0;
    for (int i = 0; i < dict->size; i++) {
        switch (dict->entry[i].type) {
            case F_PRIMITIVE:
                printf("%s\t\t", dict->entry[i].word);
                cnt++;
                break;
            case F_CONTROL:
                printf("%s\t\t", dict->entry[i].word);
                cnt++;
                break;
            default:
                break;
        }
        if (cnt % 5 == 0) putchar('\n');
    }
    putchar('\n');
} 

void F_printFunc(F_State *state) {
    F_Dict *dict = state->dict;
    for (int i = 0; i < dict->size; i++) {
        switch (dict->entry[i].type) {
            case F_FUNCTION:
                printf(": %s\n\t%s\n;\n", dict->entry[i].word, dict->entry[i].expr);
                break;
            default:
                break;
        }
    }
} 

void F_printMod(F_State *state) {
    F_Dict *dict = state->dict;
    int cnt = 0;
    for (int i = 0; i < dict->size; i++) {
        switch (dict->entry[i].type) {
            case F_MODULE:
                printf("#%d\t%s\n", cnt, dict->entry[i].word);
                cnt++;
                break;
            default:
                break;
        }
    }
} 

void F_printVar(F_State *state) {
    F_Dict *dict = state->dict;
    for (int i = 0; i < dict->size; i++) {
        switch (dict->entry[i].type) {
            case F_VARIABLE:
                printf("[%d]\t%s\n", dict->entry[i].var_index, dict->entry[i].word);
                break;
            default:
                break;
        }
    }
} 

void F_show(F_State *state, const char *s, int *pos) {
    int i = *pos, word_idx = 0;
    while (s[i] == ' ') i++;
    while (s[i] != ' ' && s[i] != '\0') state->word_buf[word_idx++] = s[i++];
    state->word_buf[word_idx] = '\0';
    *pos = i;
    if (!strcmp(state->word_buf, "*")) F_printDict(state);
    else if (!strcmp(state->word_buf, "*p")) F_printPrim(state);
    else if (!strcmp(state->word_buf, "*f")) F_printFunc(state);
    else if (!strcmp(state->word_buf, "*m")) F_printMod(state);
    else if (!strcmp(state->word_buf, "*v")) F_printVar(state);
    else {
        F_DictEntry *cur = F_find(state, state->word_buf);
        if (cur && cur->type == F_FUNCTION) {
            printf(": %s\n\t%s\n;\n", state->word_buf, cur->expr);
        }
    }
}

void F_addExpr(F_State *state, const char *word, const char *expr) {
    F_DictEntry *cur = F_find(state, word);
    F_Dict *dict = state->dict;
    if (!cur) {
        cur = &dict->entry[dict->size++];
        strcpy(cur->word, word);
    } else if (state->interactive)
        printf("[INFO] Redefined function `%s` at line %d\n", word, state->line_count);
    strcpy(cur->expr, expr);
    cur->type = F_FUNCTION;
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
    F_DictEntry *cur = &dict->entry[dict->size];
    strcpy(cur->word, word);
    cur->control = control;
    cur->type = F_CONTROL;
    dict->size++;
}

void F_addVar(F_State *state, const char *word, int val) {
    F_Dict *dict = state->dict;
    F_DictEntry *cur = F_find(state, word);
    if (!cur) {
        cur = &dict->entry[dict->size++];
        strcpy(cur->word, word);
        cur->var_index = dict->var_size++;
        cur->type = F_VARIABLE;
    } else if (cur->type != F_VARIABLE) {
        cur->var_index = dict->var_size++;
        cur->type = F_VARIABLE;
    }
    dict->vars[cur->var_index] = val;
}

void F_faddVar(F_State *state, const char *word, double val) {
    F_Dict *dict = state->dict;
    F_DictEntry *cur = F_find(state, word);
    if (!cur) {
        cur = &dict->entry[dict->size++];
        strcpy(cur->word, word);
        cur->var_index = dict->fvar_size++;
        cur->type = F_VARIABLE;
    } else if (cur->type != F_VARIABLE) {
        cur->var_index = dict->fvar_size++;
        cur->type = F_VARIABLE;
    }
    dict->fvars[cur->var_index] = val;
}

void F_addMod(F_State *state, const char *word, int flag) {
    F_Dict *dict = state->dict;
    F_DictEntry *cur = &dict->entry[dict->size++];
    strcpy(cur->word, word);
    cur->var_index = dict->var_size++;
    cur->type = F_MODULE;
    dict->vars[cur->var_index] = flag;
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
        //else state->data->size = 0;
        return;
    }
    F_pushValue(state->data, value);
}

int F_pop(F_State *state) {
    if (state->data->size <= 0) {
        fprintf(stderr, "[ERROR] Stack underflow at line %d\n", state->line_count);
        if (!state->interactive) state->running = 0;
        //else state->data->size = 0;
        return 0;
    }
    return F_popValue(state->data);
}

int F_top(F_State *state) {
    if (state->data->size <= 0) {
        fprintf(stderr, "[ERROR] Stack underflow at line %d\n", state->line_count);
        if (!state->interactive) state->running = 0;
        //else state->data->size = 0;
        return 0;
    }
    return F_topValue(state->data);
}

int F_get(F_State *state, int idx) {
    return state->data->stack[(state->data->size - idx - 1) % state->data->size];
}

void F_set(F_State *state, int idx, int value) {
    state->data->stack[(state->data->size - idx - 1) % state->data->size] = value;
}

F_FStack *F_createFStack(int capacity) {
    F_FStack *stk = (F_FStack *) malloc(sizeof(F_FStack));
    stk->stack = (double *) calloc(capacity, sizeof(double));
    stk->capacity = capacity;
    stk->size = 0;
    return stk;
}

void F_destroyFStack(F_FStack *stk) {
    free(stk->stack);
    free(stk);
}

void F_fpushValue(F_FStack *stk, double val) {
    stk->stack[stk->size++] = val;
}

double F_fpopValue(F_FStack *stk) {
    return stk->stack[--stk->size];
}

double F_ftopValue(F_FStack *stk) {
    return stk->stack[stk->size - 1];
}

void F_fpush(F_State *state, double value) {
    if (state->fdata->size >= state->fdata->capacity) {
        fprintf(stderr, "[ERROR] Stack overflow at line %d\n", state->line_count);
        if (!state->interactive) state->running = 0;
        //else state->fdata->size = 0;
        return;
    }
    F_fpushValue(state->fdata, value);
}

double F_fpop(F_State *state) {
    if (state->fdata->size <= 0) {
        fprintf(stderr, "[ERROR] Stack underflow at line %d\n", state->line_count);
        if (!state->interactive) state->running = 0;
        //else state->fdata->size = 0;
        return 0;
    }
    return F_fpopValue(state->fdata);
}

double F_ftop(F_State *state) {
    if (state->fdata->size <= 0) {
        fprintf(stderr, "[ERROR] Stack underflow at line %d\n", state->line_count);
        if (!state->interactive) state->running = 0;
        //else state->fdata->size = 0;
        return 0;
    }
    return F_ftopValue(state->fdata);
}

double F_fget(F_State *state, int idx) {
    return state->fdata->stack[(state->fdata->size - idx - 1) % state->fdata->size];
}

void F_fset(F_State *state, int idx, double value) {
    state->fdata->stack[(state->fdata->size - idx - 1) % state->fdata->size] = value;
}

F_State *F_createState() {
    F_State *state = (F_State *) malloc(sizeof(F_State));
    state->dict = F_createDict();
    state->data = F_createStack(F_MAX_STACK);
    state->fdata = F_createFStack(F_MAX_STACK);
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
    F_destroyFStack(state->fdata);
    F_destroyStack(state->loop);
    if (state->input != stdin) fclose(state->input);
    free(state);
}

void F_eval(F_State *state, char *s);

void F_parseNum(F_State *state, const char *str, int *pos) {
    if (!state->running) return;
    int x = 0, f = 1;
    double fractional = 0.0, divisor = 10.0;
    char c = str[*pos];
    while (c != '\0' && !isdigit((unsigned char)c) && c != '.') {
        if (c == '-') f = -1;
        c = str[++(*pos)];
    }
    while (c != '\0' && isdigit((unsigned char)c)) {
        x = x * 10 + (c - '0');
        c = str[++(*pos)];
    }
    if (c == '.') {
        c = str[++(*pos)];
        while (c != '\0' && isdigit((unsigned char)c)) {
            fractional += (c - '0') / divisor;
            divisor *= 10.0;
            c = str[++(*pos)];
        }
    }
    if (divisor == 10.0) {
        F_push(state, x * f);
    } else {
        F_fpush(state, (x + fractional) * f);
    }
}
void F_parseChar(F_State *state, const char *str, int *pos) {
    if (!state->running) return;
    (*pos)++;
    if (str[*pos] == '\0') {
        fprintf(stderr, "[ERROR] Unterminated character literal at line %d\n", state->line_count);
        if (!state->interactive) state->running = 0;
        return;
    }
    int c = (int)str[(*pos)++];
    if (str[*pos] != '\'') {
        fprintf(stderr, "[ERROR] Expected closing quote at line %d\n", state->line_count);
        if (!state->interactive) state->running = 0;
        return;
    }
    (*pos)++;
    F_push(state, c);
    while (str[*pos] == ' ') (*pos)++;
}

void F_parseString(F_State *state, const char *str, int *pos) {
    if (!state->running) return;
    (*pos)++;
    while (str[*pos] != '"' && str[*pos] != '\0') {
        F_push(state, str[(*pos)++]);
    }
    F_push(state, '\0');
    (*pos)++;
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
                case F_MODULE:
                    F_push(state, state->dict->entry[i].var_index);
                    break;
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
                default:
                    break;
            }
            return;
        }
    }
    fprintf(stderr, "[ERROR] Undefined word `%s` at line %d\n", state->word_buf, state->line_count);
    if (!state->interactive) state->running = 0;
    //else state->data->size = 0;
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
        else if (s[i] == '"') F_parseString(state, s, &i);
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

int F_mread(F_State *state, FILE *fm) {
    int c = fgetc(fm);
    int len = 0, comment = 0, f = 0;
    while (c != EOF) {
        f = 0;
        if (c == '\\') {
            comment = 1;
            state->line_count++;
        } else if (c == '\n' || c == '\r') {
            f = 1;
            if (comment) comment = 0;
            else break;
        }
        if (!comment && !f)
            state->module_buf[len++] = c;
        c = fgetc(fm);
    }
    state->module_buf[len] = '\0';
    return c;
}

void F_import(F_State *state, char *s) {
    if (!state->running) return;
    int saved_line_count = state->line_count;
    int saved_interactive = state->interactive;
    state->line_count = 0;
    state->interactive = 0;
    int i = 1, word_idx = 0;
    char filename[F_MAX_WORD];
    while (s[i] == ' ') i++;
    while (s[i] != ' ' && s[i] != '\0') 
        filename[word_idx++] = s[i++];
    filename[word_idx] = '\0';
    strcat(filename, ".foo");
    if (F_find(state, filename)) {
        state->line_count = saved_line_count;
        state->interactive = saved_interactive;
        if (state->interactive)
            fprintf(stdout, "[INFO] Already load module `%s` before\n", filename);
        return;
    }
    F_addMod(state, filename, 1);
    FILE *fm = fopen(filename, "r");
    if (!fm) {
        fprintf(stderr, "[ERROR] Failed to load module `%s`: %s\n", filename, strerror(errno));
        if (!saved_interactive) state->running = 0;
        state->line_count = saved_line_count;
        state->interactive = saved_interactive;
        return;
    }
    while (state->running && F_mread(state, fm) != EOF) {
        if (state->module_buf[0] == ':') {
            F_compile(state, state->module_buf);
        }
    }
    fclose(fm);
    state->line_count = saved_line_count;
    state->interactive = saved_interactive;
}

int F_read(F_State *state) {
    int c = fgetc(state->input);
    int len = 0, comment = 0, f = 0;
    while (c != EOF) {
        f = 0;
        if (c == '\\') {
            comment = 1;
            state->line_count++;
        } else if (c == '\n' || c == '\r') {
            f = 1;
            if (comment) comment = 0;
            else break;
        }
        if (!comment && !f)
            state->line_buf[len++] = c;
        c = fgetc(state->input);
    }
    state->line_buf[len] = '\0';
    //puts(state->line_buf);
    return c;
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

void F_fadd(F_State *state) {
    double b = F_fpop(state);
    double a = F_fpop(state);
    F_fpush(state, a + b);
}

void F_fsub(F_State *state) {
    double b = F_fpop(state);
    double a = F_fpop(state);
    F_fpush(state, a - b);
}

void F_fmul(F_State *state) {
    double b = F_fpop(state);
    double a = F_fpop(state);
    F_fpush(state, a * b);
}

void F_fdiv(F_State *state) {
    double b = F_fpop(state);
    if (b == 0) {
        fprintf(stderr, "[ERROR] Division by zero at line %d\n", state->line_count);
        if (!state->interactive) state->running = 0;
        else {
            F_fpush(state, b);
            fprintf(stderr, "Traceback...\n");
        }
        return;
    }
    double a = F_fpop(state);
    F_fpush(state, a / b);
}

void F_fmod(F_State *state) {
    double b = F_fpop(state);
    if (b == 0) {
        fprintf(stderr, "[ERROR] Division by zero at line %d\n", state->line_count);
        if (!state->interactive) state->running = 0;
        else {
            F_fpush(state, b);
            fprintf(stderr, "Traceback...\n");
        }
        return;
    }
    double a = F_fpop(state);
    F_fpush(state, fmod(a, b));
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
        //else state->data->size = 0;
        return;
    }
    int val = F_popValue(state->data);
    printf("%d\n", val);
}

void F_pop_silent(F_State *state) {
    if (state->data->size <= 0) {
        fprintf(stderr, "[ERROR] Stack underflow at line %d\n", state->line_count);
        if (!state->interactive) state->running = 0;
        //else state->data->size = 0;
        return;
    }
    F_popValue(state->data);
}

void F_print_stack(F_State *state) {
    printf("<%d> ", state->data->size);
    for (int i = 0; i < state->data->size; i++)
        printf("%d ", state->data->stack[i]);
    putchar('\n');
}

void F_dup(F_State *state) {
    F_push(state, F_top(state));
}

void F_swap(F_State *state) {
    int b = F_pop(state);
    int a = F_pop(state);
    F_push(state, b);
    F_push(state, a);
}

void F_pick(F_State *state) {
    int idx = F_pop(state);
    F_push(state, F_get(state, idx));
}

void F_pick_set(F_State *state) {
    int idx = F_pop(state);
    int value = F_pop(state);
    F_set(state, idx, value);
}

void F_depth(F_State *state) {
    F_push(state, state->data->size);
}

void F_fgreater(F_State *state) {
    double b = F_fpop(state);
    double a = F_fpop(state);
    F_push(state, a > b);
}

void F_fless(F_State *state) {
    double b = F_fpop(state);
    double a = F_fpop(state);
    F_push(state, a < b);
}

void F_fgreater_equal(F_State *state) {
    double b = F_fpop(state);
    double a = F_fpop(state);
    F_push(state, a >= b);
}

void F_fless_equal(F_State *state) {
    double b = F_fpop(state);
    double a = F_fpop(state);
    F_push(state, a <= b);
}

void F_fequal(F_State *state) {
    double b = F_fpop(state);
    double a = F_fpop(state);
    F_push(state, a == b);
}

void F_fnot_equal(F_State *state) {
    double b = F_fpop(state);
    double a = F_fpop(state);
    F_push(state, a != b);
}

void F_fpop_stack(F_State *state) {
    if (state->fdata->size <= 0) {
        fprintf(stderr, "[ERROR] Stack underflow at line %d\n", state->line_count);
        if (!state->interactive) state->running = 0;
        //else state->fdata->size = 0;
        return;
    }
    double val = F_fpopValue(state->fdata);
    printf("%f\n", val);
}

void F_fpop_silent(F_State *state) {
    if (state->fdata->size <= 0) {
        fprintf(stderr, "[ERROR] Stack underflow at line %d\n", state->line_count);
        if (!state->interactive) state->running = 0;
        //else state->fdata->size = 0;
        return;
    }
    F_fpopValue(state->fdata);
}

void F_fprint_stack(F_State *state) {
    printf("<%d> ", state->fdata->size);
    for (int i = 0; i < state->fdata->size; i++)
        printf("%f ", state->fdata->stack[i]);
    putchar('\n');
}

void F_fdup(F_State *state) {
    F_fpush(state, F_ftop(state));
}

void F_fswap(F_State *state) {
    double b = F_fpop(state);
    double a = F_fpop(state);
    F_fpush(state, b);
    F_fpush(state, a);
}

void F_fpick(F_State *state) {
    int idx = F_pop(state);
    F_fpush(state, F_fget(state, idx));
}

void F_fpick_set(F_State *state) {
    int idx = F_pop(state);
    double value = F_fpop(state);
    F_fset(state, idx, value);
}

void F_fdepth(F_State *state) {
    F_push(state, state->fdata->size);
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
        fprintf(stderr, "[ERROR] Unmatched `until` at line %d\n", state->line_count);
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
    F_addVar(state, state->word_buf, state->data->size > 0 ? F_pop(state) : 0);
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
    printf("%d\n", state->dict->vars[var_idx]);
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

void F_mul_store(F_State *state) {
    int var_idx = F_pop(state);
    int x = F_pop(state);
    state->dict->vars[var_idx] *= x;
}

void F_div_store(F_State *state) {
    int var_idx = F_pop(state);
    int x = F_pop(state);
    state->dict->vars[var_idx] /= x;
}

void F_fvar(F_State *state, const char *s, int *pos) {
    if (state->dict->fvar_size >= F_MAX_VARS) {
        fprintf(stderr, "[ERROR] Variable limit reached at line %d\n", state->line_count);
        state->running = 0;
        return;
    }
    int i = *pos, word_idx = 0;
    while (s[i] == ' ') i++;
    while (s[i] != ' ' && s[i] != '\0') state->word_buf[word_idx++] = s[i++];
    state->word_buf[word_idx] = '\0';
    *pos = i;
    F_faddVar(state, state->word_buf, state->fdata->size > 0 ? F_fpop(state) : 0.0);
}

void F_ffetch(F_State *state) {
    int var_idx = F_pop(state);
    F_fpush(state, state->dict->fvars[var_idx]);
}

void F_fstore(F_State *state) {
    int var_idx = F_pop(state);
    double value = F_fpop(state);
    state->dict->fvars[var_idx] = value;
}

void F_fquery(F_State *state) {
    int var_idx = F_pop(state);
    printf("%f\n", state->dict->fvars[var_idx]);
}

void F_fadd_store(F_State *state) {
    int var_idx = F_pop(state);
    double x = F_fpop(state);
    state->dict->fvars[var_idx] += x;
}

void F_fsub_store(F_State *state) {
    int var_idx = F_pop(state);
    double x = F_fpop(state);
    state->dict->fvars[var_idx] -= x;
}

void F_fmul_store(F_State *state) {
    int var_idx = F_pop(state);
    double x = F_fpop(state);
    state->dict->fvars[var_idx] *= x;
}

void F_fdiv_store(F_State *state) {
    int var_idx = F_pop(state);
    double x = F_fpop(state);
    state->dict->fvars[var_idx] /= x;
}

void F_ftoi(F_State *state) {
    double value = F_fpop(state);
    F_push(state, (int)value);
}

void F_itof(F_State *state) {
    int value = F_pop(state);
    F_fpush(state, (double)value);
}

void F_emit(F_State *state) {
    putchar(F_pop(state));
    if (state->interactive) putchar('\n');
}

void F_cr(F_State *state) {
    F_push(state, '\n');
}

void F_space(F_State *state) {
    F_push(state, ' ');
}

void F_tab(F_State *state) {
    F_push(state, '\t');
}

void F_geti(F_State *state) {
    int value;
    scanf("%d", &value);
    F_push(state, value);
}

void F_getf(F_State *state) {
    double value;
    scanf("%lf", &value);
    F_fpush(state, value);
}

void F_getc(F_State *state) {
    int c = getchar();
    F_push(state, c);
}

void F_bye(F_State *state) {
    state->running = 0;
}

void F_sqrt(F_State *state) {
    double x = F_fpop(state);
    F_fpush(state, sqrt(x));
}

void F_sin(F_State *state) {
    double x = F_fpop(state);
    F_fpush(state, sin(x));
}

void F_tan(F_State *state) {
    double x = F_fpop(state);
    F_fpush(state, tan(x));
}


void F_cos(F_State *state) {
    double x = F_fpop(state);
    F_fpush(state, cos(x));
}

void F_ceil(F_State *state) {
    double x = F_fpop(state);
    F_fpush(state, ceil(x));
}

void F_fabs(F_State *state) {
    double x = F_fpop(state);
    F_fpush(state, fabs(x));
}

void F_floor(F_State *state) {
    double x = F_fpop(state);
    F_fpush(state, floor(x));
}

void F_log(F_State *state) {
    double x = F_fpop(state);
    F_fpush(state, log(x));
}

void F_log10(F_State *state) {
    double x = F_fpop(state);
    F_fpush(state, log10(x));
}

void F_pow(F_State *state) {
    double y = F_fpop(state);
    double x = F_fpop(state);
    F_fpush(state, pow(x, y));
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
    F_addFunc(state, ".x", F_pop_silent);
    F_addFunc(state, ".s", F_print_stack);
    F_addFunc(state, "dup", F_dup);
    F_addFunc(state, "swp", F_swap);
    F_addFunc(state, "pick", F_pick);
    F_addFunc(state, "!pick", F_pick_set);
    F_addFunc(state, "depth", F_depth);

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
    F_addFunc(state, "*!", F_mul_store);
    F_addFunc(state, "/!", F_div_store);

    F_addFunc(state, "emit", F_emit);
    F_addFunc(state, "<cr>", F_cr);
    F_addFunc(state, "<space>", F_space);
    F_addFunc(state, "<tab>", F_tab);
    F_addFunc(state, "geti", F_geti);
    F_addFunc(state, "getf", F_getf);
    F_addFunc(state, "getc", F_getc);
    F_addControl(state, "show", F_show);
    F_addFunc(state, "bye", F_bye);
    
    F_addFunc(state, "f+", F_fadd);
    F_addFunc(state, "f-", F_fsub);
    F_addFunc(state, "f*", F_fmul);
    F_addFunc(state, "f/", F_fdiv);
    F_addFunc(state, "f%", F_fmod);

    F_addFunc(state, "f>", F_fgreater);
    F_addFunc(state, "f<", F_fless);
    F_addFunc(state, "f>=", F_fgreater_equal);
    F_addFunc(state, "f<=", F_fless_equal);
    F_addFunc(state, "f==", F_fequal);
    F_addFunc(state, "f~=", F_fnot_equal);

    F_addFunc(state, "f.", F_fpop_stack);
    F_addFunc(state, "f.x", F_fpop_silent);
    F_addFunc(state, "f.s", F_fprint_stack);
    F_addFunc(state, "fdup", F_fdup);
    F_addFunc(state, "fswp", F_fswap);
    F_addFunc(state, "fpick", F_fpick);
    F_addFunc(state, "f!pick", F_fpick_set);
    F_addFunc(state, "fdepth", F_fdepth);

    F_addControl(state, "fvar", F_fvar);
    F_addFunc(state, "f@", F_ffetch);
    F_addFunc(state, "f!", F_fstore);
    F_addFunc(state, "f?", F_fquery);
    F_addFunc(state, "f+!", F_fadd_store);
    F_addFunc(state, "f-!", F_fsub_store);
    F_addFunc(state, "f*!", F_fmul_store);
    F_addFunc(state, "f/!", F_fdiv_store);
    
    F_addFunc(state, "f2i", F_ftoi);
    F_addFunc(state, "i2f", F_itof);

    F_addFunc(state, "sqrt", F_sqrt);
    F_addFunc(state, "sin", F_sin);
    F_addFunc(state, "cos", F_cos);
    F_addFunc(state, "tan", F_tan);
    F_addFunc(state, "ceil", F_ceil);
    F_addFunc(state, "floor", F_floor);
    F_addFunc(state, "fabs", F_fabs);
    F_addFunc(state, "log", F_log);
    F_addFunc(state, "log10", F_log10);
    F_addFunc(state, "pow", F_pow);
}

void F_execScript(F_State *state, const char *filename) {
    if (filename) {
        state->input = fopen(filename, "r");
        if (state->input == NULL) {
            fprintf(stderr, "[ERROR] Failed to open file `%s`: %s\n", filename, strerror(errno));
            return;
        }
        state->interactive = 0;
    } else puts(F_MSG);
    while (state->running && F_read(state) != EOF) {
        if (state->line_buf[0] == ':') F_compile(state, state->line_buf);
        else if (state->line_buf[0] == '#') F_import(state, state->line_buf);
        else F_eval(state, state->line_buf);
    }
}
#endif //FOO_H
