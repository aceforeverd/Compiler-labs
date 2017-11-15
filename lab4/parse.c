/*
 * parse.c - Parse source file.
 */

#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "symbol.h"
#include "absyn.h"
#include "types.h"
#include "errormsg.h"
#include "parse.h"
#include "semant.h"

extern int yyparse(void);
extern A_exp absyn_root;

/* parse source file fname; 
   return abstract syntax data structure */
A_exp parse(string fname)
{
    EM_reset(fname);
    if (yyparse() == 0) /* parsing worked */
        return absyn_root;
    else return NULL;
}

int main(int argc, char **argv){
    if(argc != 2){
        fprintf(stderr,"usage: a.out filename\n");
        exit(1);
    }
    //pr_exp(stderr, parse(argv[1]), 0);
    absyn_root = parse(argv[1]);
    //pr_exp(stderr,absyn_root,0);
    if (absyn_root){
        SEM_transProg(absyn_root);
    }
    fprintf(stderr,"\n");
    return 0;
}
