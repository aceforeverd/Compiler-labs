#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"
#include "types.h"
#include "helper.h"
#include "env.h"
#include "semant.h"

/*Lab4: Your implementation of lab4*/

typedef void* Tr_exp;

struct expty
{
    Tr_exp exp;
    Ty_ty ty;
};

//In Lab4, the first argument exp should always be **NULL**.
struct expty expTy(Tr_exp exp, Ty_ty ty)
{
    struct expty e;

    e.exp = exp;
    e.ty = ty;

    return e;
}

struct expty transVar(S_table venv, S_table tenv, A_var v)
{
    switch (v->kind) {
        case A_simpleVar: {
            E_enventry entry = S_look(venv, v->u.simple);

            if (!entry || entry->kind == E_funEntry) {
                EM_error(v->pos, "undefined variable %s", S_name(v->u.simple));
                return expTy(NULL, Ty_Int());
            }
            return expTy(NULL, actual_ty(entry->u.var.ty));
        }

        case A_fieldVar: {
            expty_t t_var = transVar(venv, tenv, v->u.field.var);
            if (t_var.ty->kind != Ty_record) {
                EM_error(v->u.field.var->pos, "not a record type");
                return expTy(NULL, Ty_Void());
            }
            Ty_field t_field = FieldList_lookup(t_var.ty->u.record, v->u.field.sym);
            if (!t_field) {
                EM_error(v->u.field.var->pos, "field %s doesn't exist", S_name(v->u.field.sym));
                return expTy(NULL, Ty_Void());
            }
            return expTy(NULL, t_field->ty);
        }

        case A_subscriptVar: {
            expty_t t_var = transVar(venv, tenv, v->u.subscript.var);
            if (t_var.ty->kind != Ty_array) {
                EM_error(v->u.subscript.var->pos, "array type required");
            }
            expty_t t_exp = transExp(venv, tenv, v->u.subscript.exp);
            if (t_exp.ty->kind != Ty_int) {
                EM_error(v->u.subscript.exp->pos, "exp of subscript should be int");
            }
            return expTy(NULL, t_var.ty->u.array);
        }
    }
}

struct expty transExp(S_table venv, S_table tenv, A_exp aExp)
{
    if (!aExp) {
        return expTy(NULL, Ty_Void());
    }

    switch (aExp->kind) {
        case A_varExp: {
            return transVar(venv, tenv, aExp->u.var);
        }
        case A_intExp: {
            return expTy(NULL, Ty_Int());
        }
        case A_stringExp: {
            return expTy(NULL, Ty_String());
        }
        case A_callExp: {
            E_enventry f_entry = S_look(venv, aExp->u.call.func);
            if (!f_entry || f_entry->kind == E_varEntry) {
                EM_error(aExp->pos, "undefined function %s", S_name(aExp->u.call.func));
                return expTy(NULL, Ty_Int());
            }
            check_function_params(venv, tenv, aExp, f_entry->u.fun.formals);
            return expTy(NULL, f_entry->u.fun.result);
        }
        case A_opExp: {
            A_oper oper = aExp->u.op.oper;
            struct expty left = transExp(venv, tenv, aExp->u.op.left);
            struct expty right = transExp(venv, tenv, aExp->u.op.right);

            if (oper == A_plusOp || oper == A_minusOp
                    || oper == A_timesOp || oper == A_divideOp) {
                if (left.ty->kind != Ty_int) {
                    EM_error(aExp->u.op.left->pos, "integer required");
                }
                if (right.ty->kind != Ty_int) {
                    EM_error(aExp->u.op.right->pos, "integer required");
                }
                return expTy(NULL, Ty_Int());
            } else {
                if (left.ty->kind != right.ty->kind) {
                    EM_error(aExp->u.op.right->pos, "same type required");
                }

                if (oper != A_eqOp && oper != A_neqOp) {
                    if (left.ty->kind != Ty_int && left.ty->kind != Ty_string) {
                        EM_error(aExp->u.op.left->pos, "Compare unsupport type");
                    }
                }
                return expTy(NULL, Ty_Int());
            }
        }

        case A_recordExp: {
            Ty_ty record_ty = S_look(tenv, aExp->u.record.typ);
            if (!record_ty) {
                EM_error(aExp->pos, "undefined type %s", S_name(aExp->u.record.typ));
                return expTy(NULL, Ty_Record(NULL));
            }
            if (record_ty->kind != Ty_record) {
                EM_error(aExp->pos, "%s is not a record", S_name(aExp->u.record.typ));
                return expTy(NULL, Ty_Name(aExp->u.record.typ, Ty_Record(record_ty->u.record)));
            }

            if (!aExp->u.record.fields && record_ty->u.record) {
                EM_error(aExp->pos, "too less assign to record");
            } else {
                check_fields_match(venv, tenv, aExp->u.record.fields, record_ty->u.record);
            }
            return expTy(NULL, Ty_Name(aExp->u.record.typ, Ty_Record(record_ty->u.record)));
        }

        case A_seqExp: {
            A_expList list = aExp->u.seq;
            if (list == NULL || list->head == NULL) {
                return expTy(NULL, Ty_Void());
            }
            A_exp exp;
            expty_t exp_ty = expTy(NULL, Ty_Void());
            while (list) {
                exp = list->head;
                exp_ty = transExp(venv, tenv, exp);
                list = list->tail;
            }

            return exp_ty;
        }

        case A_assignExp: {
            expty_t var_ty = transVar(venv, tenv, aExp->u.assign.var);
            if (var_ty.ty->kind == Ty_void) {
                return expTy(NULL, Ty_Void());
            }
            expty_t exp_ty = transExp(venv, tenv, aExp->u.assign.exp);
            if (aExp->u.assign.var->kind == A_simpleVar) {
                E_enventry entry = S_look(venv, aExp->u.assign.var->u.simple);
                if (entry && entry->readonly && entry->kind == E_varEntry) {
                    EM_error(aExp->u.assign.var->pos, "loop variable can't be assigned");
                }
            }
            if (var_ty.ty->kind == Ty_record && exp_ty.ty->kind == Ty_nil) {
                return expTy(NULL, Ty_Void());
            }
            if (var_ty.ty->kind != exp_ty.ty->kind) {
                EM_error(aExp->u.assign.exp->pos, "unmatched assign exp");
            }

            return expTy(NULL, Ty_Void());
        }

        case A_ifExp: {
            expty_t test_ty = transExp(venv, tenv, aExp->u.iff.test);
            expty_t then_ty = transExp(venv, tenv, aExp->u.iff.then);
            expty_t else_ty = transExp(venv, tenv, aExp->u.iff.elsee);
            if (test_ty.ty->kind != Ty_int) {
                EM_error(aExp->u.iff.test->pos, "test statement is not integer");
            }
            if (aExp->u.iff.elsee == NULL) {
                if (then_ty.ty->kind != Ty_void) {
                    EM_error(aExp->u.iff.then->pos, "if-then exp's body must produce no value");
                }
                return expTy(NULL, Ty_Void());
            }

            if (then_ty.ty->kind != else_ty.ty->kind) {
                EM_error(aExp->u.iff.elsee->pos, "then exp and else exp type mismatch");
            }
            if (then_ty.ty->kind == Ty_nil) {
                EM_error(aExp->u.iff.then->pos, "then exp return type can't be nil");
            }
            if (else_ty.ty->kind == Ty_nil) {
                EM_error(aExp->u.iff.elsee->pos, "else exp return type can't be nil");
            }

            return expTy(NULL, then_ty.ty);
        }

        case A_whileExp: {
            expty_t test_ty = transExp(venv, tenv, aExp->u.whilee.test);
            expty_t body_ty = transExp(venv, tenv, aExp->u.whilee.body);
            if (test_ty.ty->kind != Ty_int) {
                EM_error(aExp->u.whilee.test->pos, "test exp must return integer");
            }
            if (body_ty.ty->kind != Ty_void) {
                EM_error(aExp->u.whilee.body->pos, "while body must produce no value");
            }
            return expTy(NULL, Ty_Void());
        }

        case A_forExp: {
            S_beginScope(venv);
            S_enter(venv, aExp->u.forr.var, E_ROVarEntry(Ty_Int()));


            expty_t exp1_ty = transExp(venv, tenv, aExp->u.forr.lo);
            if (exp1_ty.ty->kind != Ty_int) {
                EM_error(aExp->u.forr.lo->pos, "for exp's range type is not integer");
            }

            expty_t exp2_ty = transExp(venv, tenv, aExp->u.forr.hi);
            if (exp2_ty.ty->kind != Ty_int) {
                EM_error(aExp->u.forr.hi->pos, "for exp's range type is not integer");
            }

            expty_t exp3_ty = transExp(venv, tenv, aExp->u.forr.body);
            if (exp3_ty.ty->kind != Ty_void) {
                EM_error(aExp->u.forr.body->pos, "for loop body must return no value");
            }

            S_endScope(venv);
            return expTy(NULL, Ty_Void());
        }

        case A_breakExp: {
            return expTy(NULL, Ty_Void());
        }

        case A_letExp: {
            expty_t exp;
            A_decList d;
            S_beginScope(venv);
            S_beginScope(tenv);
            for (d = aExp->u.let.decs; d; d = d->tail) {
                transDec(venv, tenv, d->head);
            }
            exp = transExp(venv, tenv, aExp->u.let.body);
            S_endScope(tenv);
            S_endScope(venv);
            return exp;
        }

        case A_arrayExp: {
            Ty_ty base_ty = S_look(tenv, aExp->u.array.typ);
            if (!base_ty) {
                EM_error(aExp->pos, "array type %s not defined", S_name(aExp->u.array.typ));
                return expTy(NULL, Ty_Void());
            }
            expty_t array_ty = transExp(venv, tenv, aExp->u.array.init);
            expty_t size_ty = transExp(venv, tenv, aExp->u.array.size);
            if (array_ty.ty->kind != base_ty->u.array->kind) {
                EM_error(aExp->u.array.init->pos, "type mismatch");
            }
            if (size_ty.ty->kind != Ty_int) {
                EM_error(aExp->u.array.size->pos, "array size should be integer");
            }
            return expTy(NULL, Ty_Name(aExp->u.array.typ, Ty_Array(base_ty)));
        }

        case A_nilExp:
            return expTy(NULL, Ty_Nil());
        default:
            return expTy(NULL, Ty_Void());

    }
}

void transDec(S_table venv, S_table tenv, A_dec dec)
{
    switch (dec->kind) {
        case A_varDec: {
            expty_t e = transExp(venv, tenv, dec->u.var.init);
            if ( e.ty->kind == Ty_nil) {
                if (!dec->u.var.typ ||
                        strlen(S_name(dec->u.var.typ)) == 0)
                    EM_error(dec->u.var.init->pos, "init should not be nil without type specified");
            }
            if (dec->u.var.typ &&
                    strlen(S_name(dec->u.var.typ)) != 0) {
                Ty_ty var_ty = S_look(tenv, dec->u.var.typ);
                if (e.ty->kind != var_ty->kind) {
                    if (e.ty->kind == Ty_nil) {
                        if (var_ty->kind != Ty_record)
                            EM_error(dec->u.var.init->pos, "Nil can't not assign to var that is not record");
                    } else {
                        EM_error(dec->u.var.init->pos, "type mismatch");
                    }
                }

                S_enter(venv, dec->u.var.var, E_VarEntry(var_ty));
            } else {
                S_enter(venv, dec->u.var.var, E_VarEntry(e.ty));
            }
            break;
        }

        case A_typeDec: {
            transTypeDecHeader(venv, tenv, dec->u.type);
            transTypeDecBody(venv, tenv, dec->u.type);
            if (checkTypeDecCircles(tenv, dec->u.type) == 1) {
                EM_error(dec->pos, "illegal type cycle");
            }
            break;
        }
        case A_functionDec: {
            transFunDecHeader(venv, tenv, dec->u.function);
            transFunDecBody(venv, tenv, dec->u.function);
            break;
        }
    }
}

Ty_ty transTy(S_table tenv, A_ty a_ty)
{
    Ty_ty ty;
    switch (a_ty->kind) {
        case A_nameTy:
            ty = S_look(tenv, a_ty->u.name);
            return ty;
        case A_recordTy: {
            return Ty_Record(transFieldList(tenv, a_ty->u.record));
        }
        case A_arrayTy: {
            Ty_ty type = S_look(tenv, a_ty->u.array);
            return Ty_Array(type);
        }
    }
}

void SEM_transProg(A_exp exp)
{
    S_table tenv = E_base_tenv();
    S_table venv = E_base_venv();
    transExp(venv, tenv, exp);
}


int check_function_params(S_table venv, S_table tenv,
                          A_exp exp, Ty_tyList standards) {
    if (exp->kind != A_callExp) {
        return 1;
    }

    A_expList params = exp->u.call.args;
    A_exp a_exp ;
    Ty_ty a_ty ;
    A_expList a_expList = params;
    Ty_tyList a_tyList = standards;
    while (a_expList && a_tyList) {
        a_exp = a_expList->head;
        a_ty = a_tyList->head;

        expty_t exp_ty = transExp(venv, tenv, a_exp);
        if (exp_ty.ty->kind != a_ty->kind) {
            EM_error(a_exp->pos, "para type mismatch");
        }

        a_expList = a_expList->tail;
        a_tyList = a_tyList->tail;
    }

    if (a_expList ) {
        EM_error(a_expList->head->pos, "too many params in function %s", S_name(exp->u.call.func));
    }
    if (a_tyList) {
        EM_error(a_exp->pos, "too less parmas");
    }

    return 0;
}

Ty_tyList makeFormalTyList(S_table tenv, A_fieldList params)
{
    if (params == NULL) {
        return NULL;
    } else {
        Ty_ty ty = S_look(tenv, params->head->typ);
        if (!ty) {
            printf("unknow param type: %s\n", S_name(params->head->typ));
        }
        return Ty_TyList(ty, makeFormalTyList(tenv, params->tail));
    }
}


void check_fields_match(S_table venv, S_table tenv,
                        A_efieldList args, Ty_fieldList standards)
{
    if (!args) {
        return;
    }

    if ( !standards) {
        EM_error(args->head->exp->pos, "too match assign to record");
        return;
    }

    A_efieldList e_list = args;
    Ty_fieldList s_list = standards;
    A_efield efield;
    A_efield pos;
    Ty_field field;
    while (e_list && s_list) {
        efield = e_list->head;
        field = s_list->head;

        check_field_match(venv, tenv, efield, field);
        pos = efield;

        e_list = e_list->tail;
        s_list = s_list->tail;
    }

    if (e_list) {
        EM_error(e_list->head->exp->pos, "too match assign to record");
    }
    if (s_list) {
        EM_error(pos->exp->pos, "too less assign to record");
    }
}

void check_field_match(S_table venv, S_table tenv, A_efield ef, Ty_field tf)
{
    if (ef->name != tf->name) {
        EM_error(ef->exp->pos, "field name not match declared");
    }

    expty_t ef_ty = transExp(venv, tenv, ef->exp);
    if (ef_ty.ty->kind != tf->ty->kind) {
        if (ef_ty.ty->kind == Ty_nil &&
                real_ty(tenv, tf->ty)->kind == Ty_record) {
        } else {
            EM_error(ef->exp->pos, "assign exp value not match declared");
        }
    }
}

void transTypeDecHeader(S_table venv, S_table tenv, A_nametyList types)
{
    if (types == NULL) return;

    if (S_look(tenv, types->head->name) != NULL) {
        EM_error(0, "two types have the same name", S_name(types->head->name));
    } else {
        S_enter(tenv, types->head->name, Ty_Name(types->head->name, NULL));
    }

    transTypeDecHeader(venv, tenv, types->tail);
}

void transTypeDecBody(S_table venv, S_table tenv, A_nametyList types)
{
    if (types == NULL) return;

    Ty_ty ty = transTy(tenv, types->head->ty);
    S_enter(tenv, types->head->name, ty);

    transTypeDecBody(venv, tenv, types->tail);
}

int checkTypeDecCircles(S_table tenv, A_nametyList types)
{
    int n = 0;
    A_nametyList ls = types;
    while (ls) {
        n ++;
        ls = ls->tail;
    }

    ls = types;
    while (ls) {
        if  (checkTypeDecCircle(tenv, ls->head, n) == 1)
            return 1;

        ls = ls->tail;
    }

    return 0;
}

int checkTypeDecCircle(S_table tenv, A_namety type, int max_num)
{
    int i = 0;
    Ty_ty ty = S_look(tenv, type->name);
    while (ty && ty->kind == Ty_name) {
        i ++;

        ty = S_look(tenv, ty->u.name.sym);

        if (i >= max_num) {
            return 1;
        }
    }

    return 0;
}

Ty_fieldList transFieldList(S_table tenv, A_fieldList list)
{
    if (list == NULL || list->head == NULL) {
        return NULL;
    }

    A_field f = list->head;
    Ty_ty ty = S_look(tenv, f->typ);
    if (!ty) {
        EM_error(f->pos, "undefined type %s", S_name(f->typ));
    }

    return Ty_FieldList(Ty_Field(f->name, ty), transFieldList(tenv, list->tail));
}

Ty_ty real_ty(S_table tenv, Ty_ty ty) {
    /*
     * recursive find real type
     */
    if (!ty || ty->kind != Ty_name) {
        return ty;
    }

    Ty_ty t = ty;
    while (t && t->kind == Ty_name) {
        t = S_look(tenv, t->u.name.sym);

        if (!t) {
            EM_error(0, "undefined type");
            break;
        }

        if (ty == t) {
            EM_error(0, "illegal type cycle");
            break;
        }
    }

    return t;
}

void transSingleFunDec(S_table venv, S_table tenv, A_fundec fun)
{
    Ty_ty resultTy = S_look(tenv, fun->result);
    Ty_tyList formalTys = makeFormalTyList(tenv, fun->params);
    S_enter(venv, fun->name, E_FunEntry(formalTys, resultTy));

    S_beginScope(venv);

    A_fieldList l;
    Ty_tyList t;
    for (l = fun->params, t = formalTys; l; l = l->tail, t = t->tail) {
        S_enter(venv, l->head->name, E_VarEntry(t->head));
    }
    expty_t fun_ty = transExp(venv, tenv, fun->body);

    S_endScope(venv);

    if (resultTy) {
        if (resultTy->kind != fun_ty.ty->kind) {
            EM_error(fun->body->pos, "fun return type declared missmatch with body returned");
        }
    } else {
        S_enter(venv, fun->name, E_FunEntry(formalTys, fun_ty.ty));
    }

}

void transFunDecHeader(S_table venv, S_table tenv, A_fundecList fundecList)
{
    /* just parse function params and return type */
    if (fundecList == NULL) {
        return ;
    }

    Ty_ty result_ty = Ty_Void();
    if (fundecList->head->result &&
            strlen(S_name(fundecList->head->result)) != 0) {
        result_ty = S_look(tenv, fundecList->head->result);
        if (!result_ty) {
            EM_error(fundecList->head->pos, "undefined function return type");
            result_ty = Ty_Void();
        }
    }

    if (S_look(venv, fundecList->head->name) != NULL) {
        EM_error(fundecList->head->pos, "two functions have the same name", fundecList->head->pos);
    } else {
        S_enter(venv, fundecList->head->name, E_FunEntry(
                transFuncParams(tenv, fundecList->head->params),
                result_ty
        ));
    }


    transFunDecHeader(venv, tenv, fundecList->tail);
}

void transFunDecBody(S_table venv, S_table tenv, A_fundecList fundecList)
{
    /*
     * parse function body
     */
    if (fundecList == NULL) {
        return;
    }

    Ty_tyList formalTys = transFuncParams(tenv, fundecList->head->params);

    S_beginScope(venv);

    A_fieldList l;
    Ty_tyList t;
    for (l = fundecList->head->params, t = formalTys; l; l = l->tail, t = t->tail) {
        S_enter(venv, l->head->name, E_VarEntry(t->head));
    }

    expty_t body_ty = transExp(venv, tenv, fundecList->head->body);
    if (fundecList->head->result &&
            strlen(S_name(fundecList->head->result))  != 0) {
        Ty_ty result_ty = S_look(tenv, fundecList->head->result);
        if (body_ty.ty->kind != result_ty->kind) {
            EM_error(fundecList->head->pos, "function return type not match body");
        }
    } else {
        if (body_ty.ty->kind != Ty_void) {
            EM_error(fundecList->head->body->pos, "procedure returns value");
        }
    }

    S_endScope(venv);

    transFunDecBody(venv, tenv, fundecList->tail);
}

Ty_tyList transFuncParams(S_table tenv, A_fieldList params)
{
    /*
     * turn A_fieldList to Ty_tyList
     */
    if (params == NULL) {
        return NULL;
    }

    Ty_ty type = S_look(tenv, params->head->typ);
    return Ty_TyList(
            type,
            transFuncParams(tenv, params->tail)
    );
}
