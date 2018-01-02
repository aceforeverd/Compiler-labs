#ifndef PRINT_TREE_H
#define PRINT_TREE_H

#include "frame.h"
#include "tree.h"
#include <stdio.h>

/* function prototype from printtree.c */
void printStmList (FILE *out, T_stmList stmList) ;

void F_echoFrag(F_frag frag);

void pr_tree_exp(FILE *out, T_exp exp, int d);

void pr_stm(FILE *out, T_stm stm, int d);

void F_echoFragList(F_fragList list);

#endif

