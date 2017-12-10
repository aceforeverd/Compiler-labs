#ifndef __SEMANT_H_
#define __SEMANT_H_

#include "absyn.h"
#include "symbol.h"
#include "temp.h"
#include "frame.h"
#include "translate.h"
#include "types.h"

struct expty;
typedef struct expty expty_t;

struct expty transVar(S_table venv, S_table tenv, A_var v, Tr_level level, Temp_label  label);
struct expty transExp(S_table venv, S_table tenv, A_exp aExp, Tr_level level, Temp_label label);
Tr_exp		 transDec(S_table venv, S_table tenv, A_dec dec, Tr_level level, Temp_label label);
Ty_ty		 transTy (S_table tenv, A_ty a);

F_fragList SEM_transProg(A_exp exp);

Tr_expList check_function_params(S_table venv, S_table tenv, int pos, A_expList args, Ty_tyList standards, S_symbol func,
                          Tr_level level, Temp_label label);

Ty_tyList makeFormalTyList(S_table tenv, A_fieldList params);

void check_fields_match(S_table venv, S_table tenv, A_efieldList args, Ty_fieldList standards, Tr_level level);
void check_field_match(S_table venv, S_table tenv, A_efield ef, Ty_field tf, Tr_level level);

void transTypeDecHeader(S_table venv, S_table tenv, A_nametyList types);
void transTypeDecBody(S_table venv, S_table tenv, A_nametyList types);

Ty_fieldList transFieldList(S_table tenv, A_fieldList list);

Ty_ty real_ty(S_table tenv, Ty_ty ty) ;

void transFunDecHeader(S_table venv, S_table tenv, A_fundecList fundecList, Tr_level level);
void transFunDecBody(S_table venv, S_table tenv, A_fundecList fundecList, Tr_level level);

U_boolList paramsToEscapes(A_fieldList params);

Ty_tyList transFuncParams(S_table tenv, A_fieldList params);


int checkTypeDecCircles(S_table tenv, A_nametyList types);
int checkTypeDecCircle(S_table tenv, A_namety type, int max_num);
#endif

