#ifndef TRANSLATE_H
#define TRANSLATE_H

#include "absyn.h"
#include "frame.h"
#include "temp.h"
#include "util.h"

typedef struct Tr_exp_ *Tr_exp;
typedef struct Tr_expList_ *Tr_expList;

Tr_expList  Tr_ExpList(Tr_exp head, Tr_expList tail);

typedef struct Tr_access_ *Tr_access;

typedef struct Tr_accessList_ *Tr_accessList;

typedef struct Tr_level_ *Tr_level;

Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail);

Tr_level Tr_outermost(void);

Tr_level Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals);

Tr_accessList Tr_formals(Tr_level level);

Tr_access Tr_allocLocal(Tr_level level, bool escape);

void Tr_procEntryExit(Tr_level level, Tr_exp body, Tr_accessList formals);

F_fragList Tr_getResult();

void Tr_init();

Tr_exp Tr_simpleVar(Tr_access access, Tr_level level);

static T_exp follow_static_link(Tr_level call_level, Tr_level dec_level);

Tr_exp Tr_subscriptVar(Tr_exp head, Tr_exp sub);

Tr_exp Tr_fieldVar(Tr_exp var, int order) ;

Tr_exp Tr_intExp(int var);

Tr_exp Tr_stringExp(string str);

Tr_exp Tr_opExp(A_oper op, Tr_exp left, Tr_exp right);

Tr_exp Tr_ifExp(Tr_exp test, Tr_exp then, Tr_exp els, Tr_level level);

Tr_exp Tr_whileExp(Tr_exp test, Tr_exp body);

Tr_exp Tr_callExp(S_symbol func, Temp_label label, Tr_level call_level, Tr_level dec_level, Tr_expList args);

Tr_exp Tr_letExp(Tr_exp body, Tr_level level);

Tr_exp Tr_varDec();

Tr_exp Tr_funDec(Tr_exp body, Tr_level level);

Tr_exp Tr_nilExp();

int *initArray(int size, int init_val);
#endif
