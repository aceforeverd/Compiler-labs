#ifndef __SEMANT_H_
#define __SEMANT_H_

#include "absyn.h"
#include "symbol.h"

struct expty;
typedef struct expty expty_t;

struct expty transVar(S_table venv, S_table tenv, A_var v);
struct expty transExp(S_table venv, S_table tenv, A_exp aExp);
void		 transDec(S_table venv, S_table tenv, A_dec dec);
Ty_ty		 transTy (              S_table tenv, A_ty a);

void SEM_transProg(A_exp exp);

int check_function_params(S_table venv, S_table tenv, A_exp exp, Ty_tyList standards);

Ty_tyList makeFormalTyList(S_table tenv, A_fieldList params);

void transSingleFunDec(S_table venv, S_table tenv, A_fundec fun);

void check_fields_match(S_table venv, S_table tenv, A_efieldList args, Ty_fieldList standards);
void check_field_match(S_table venv, S_table tenv, A_efield ef, Ty_field tf);

void transTypeDecHeader(S_table venv, S_table tenv, A_nametyList types);
void transTypeDecBody(S_table venv, S_table tenv, A_nametyList types);

Ty_fieldList transFieldList(S_table tenv, A_fieldList list);

Ty_ty real_ty(S_table tenv, Ty_ty ty) ;

void transFunDecHeader(S_table venv, S_table tenv, A_fundecList fundecList);
void transFunDecBody(S_table venv, S_table tenv, A_fundecList fundecList);

Ty_tyList transFuncParams(S_table tenv, A_fieldList params);


int checkTypeDecCircles(S_table tenv, A_nametyList types);
int checkTypeDecCircle(S_table tenv, A_namety type, int max_num);
#endif

