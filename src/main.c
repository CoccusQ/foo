/**********************************
 *   Foo
 *   Copyright (C) 2025 CoccusQ
 *   MIT License
 **********************************/

#include "foo.h"

int main(int argc, char *argv[]) {
    F_State *fState = F_createState();
    F_initState(fState);
    F_execScript(fState, argc > 1 ? argv[1] : NULL);
    F_destroyState(fState);
    return 0;
}