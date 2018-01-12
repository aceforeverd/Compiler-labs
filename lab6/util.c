/*
 * util.c - commonly used utility functions.
 */

#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void *checked_malloc(int len) {
    void *p = malloc(len);
    if (!p) {
        fprintf(stderr, "\nRan out of memory!\n");
        exit(1);
    }
    return p;
}

string String(char *s) {
    if (!s) {
        return NULL;
    }
    unsigned int length = strlen(s) + 1;
    string p = checked_malloc(length);
    strncpy(p, s, length);
    return p;
}

U_boolList U_BoolList(bool head, U_boolList tail) {
    U_boolList list = checked_malloc(sizeof(*list));
    list->head = head;
    list->tail = tail;
    return list;
}
