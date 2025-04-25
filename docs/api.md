# Foo API

## 1. 简介

Foo 解释器提供了一系列的 API，用于在 C 语言中创建和操作虚拟机实例，以及扩展解释器的功能。本文档介绍了这些 API 的使用方法。

## 2. 核心结构

### 2.1 `F_State`

`F_State` 是解释器的核心结构体，表示虚拟机的状态。它包含字典、堆栈、输入流等信息。

```c
typedef struct F_State F_State;
```

### 2.2 `F_DictEntry`

`F_DictEntry` 表示字典中的一个条目，可以是函数、变量、控制结构等。

```c
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
```

其中，控制结构的函数有三个参数，分别代表当前虚拟机、当前执行的代码字符串，以及当前读入字符的光标位置。

通过这几个参数，控制结构类函数可以获取当前虚拟机环境参数，获取当前代码，并通过改变读入字符的光标位置，改变代码的执行顺序。

## 3. API 列表

### 3.1 虚拟机管理

#### 3.1.1 `F_createState`

创建一个新的虚拟机实例。

```c
F_State *F_createState();
```

#### 3.1.2 `F_destroyState`

销毁虚拟机实例，并释放相关资源。

```c
void F_destroyState(F_State *state);
```

### 3.2 字典操作

#### 3.2.1 `F_addFunc`

向字典中添加一个新的原始函数。

```c
void F_addFunc(F_State *state, const char *word, void (*func)(F_State *));
```

#### 3.2.2 `F_addExpr`

向字典中添加一个新的表达式。

```c
void F_addExpr(F_State *state, const char *word, const char *expr);
```

#### 3.2.3 `F_addControl`

向字典中添加一个新的控制结构。

```c
void F_addControl(F_State *state, const char *word, void (*control)(F_State *, const char *, int *));
```

#### 3.2.4 `F_addVar`

向字典中添加一个新的变量。

```c
void F_addVar(F_State *state, const char *word, int val);
```

#### 3.2.5 `F_find`

在字典中查找指定的单词。

```c
F_DictEntry *F_find(F_State *state, const char *word);
```

### 3.3 堆栈操作

#### 3.3.1 `F_push`

向数据堆栈中压入一个值。

```c
void F_push(F_State *state, int value);
```

#### 3.3.2 `F_pop`

从数据堆栈中弹出一个值。

```c
int F_pop(F_State *state);
```

#### 3.3.3 `F_top`

获取数据堆栈顶部的值。

```c
int F_top(F_State *state);
```

### 3.4 解释器控制

#### 3.4.1 `F_execScript`

执行脚本文件或进入交互模式。

```c
void F_execScript(F_State *state, const char *filename);
```

#### 3.4.2 `F_eval`

评估并执行输入的 Foo 代码。

```c
void F_eval(F_State *state, char *s);
```

#### 3.4.3 `F_compile`

编译用户定义的函数。

```c
void F_compile(F_State *state, char *s);
```

#### 3.4.4 `F_initState`

初始化虚拟机状态，添加内置的函数、控制结构和变量。

```c
void F_initState(F_State *state);
```

### 3.5 输入输出

#### 3.5.1 `F_read`

从输入流中读取一行代码。

```c
int F_read(F_State *state);
```

#### 3.5.2 `F_write`

将值输出到控制台。

```c
void F_write(int x);
```

#### 3.5.3 `F_pop_stack`

从数据堆栈中弹出一个值并输出。

```c
void F_pop_stack(F_State *state);
```

#### 3.5.4 `F_print_stack`

打印数据堆栈的内容。

```c
void F_print_stack(F_State *state);
```

### 3.6 控制结构

#### 3.6.1 `F_if`

处理 `if` 控制结构。

```c
void F_if(F_State *state, const char *s, int *pos);
```

#### 3.6.2 `F_else`

处理 `else` 控制结构。

```c
void F_else(F_State *state, const char *s, int *pos);
```

#### 3.6.3 `F_begin`

处理 `begin` 控制结构。

```c
void F_begin(F_State *state, const char *s, int *pos);
```

#### 3.6.4 `F_until`

处理 `until` 控制结构。

```c
void F_until(F_State *state, const char *s, int *pos);
```

### 3.7 变量操作

#### 3.7.1 `F_fetch`

获取变量的值。

```c
void F_fetch(F_State *state);
```

#### 3.7.2 `F_store`

设置变量的值。

```c
void F_store(F_State *state);
```

#### 3.7.3 `F_query`

查询并输出变量的值。

```c
void F_query(F_State *state);
```

#### 3.7.4 `F_increase`

增加变量的值。

```c
void F_increase(F_State *state);
```

#### 3.7.5 `F_decrease`

减少变量的值。

```c
void F_decrease(F_State *state);
```

#### 3.7.6 `F_add_store`

将值加到变量中。

```c
void F_add_store(F_State *state);
```

#### 3.7.7 `F_sub_store`

将值从变量中减去。

```c
void F_sub_store(F_State *state);
```

## 4. 示例

### 4.1 添加自定义函数

在 C 语言中定义一个新的函数并添加到字典中：

```c
#include "foo.h"

void custom_function(F_State *state) {
    int a = F_pop(state);
    int b = F_pop(state);
    F_push(state, a + b);
    F_pop_stack(state);
}

int main(int argc, char *argv[]) {
    F_State *fState = F_createState();
    F_initState(fState);
    F_addFunc(fState, "addAndPrint", custom_function);
    F_execScript(fState, argc > 1 ? argv[1] : NULL);
    F_destroyState(fState);
    return 0;
}
```

### 4.2 定义和使用变量

在 Foo 脚本中定义和使用变量：

```foo
var myVar
10 myVar !
myVar @ .
myVar @ 5 + myVar !
myVar @ .
```

### 4.3 使用控制结构

在 Foo 脚本中使用控制结构：

```foo
1 2 > if 1 . else 2 . then 
var i begin i @ . i @ 5 < if i @ 1 + i ! then until
```