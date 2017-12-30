#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"
#include "types.h"
#include "env.h"
#include "semant.h"
#include "helper.h"
#include "translate.h"

/*Lab5: Your implementation of lab4*/

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

struct expty transVar(S_table venv, S_table tenv, A_var v, Tr_level level, Temp_label label)
{
    switch (v->kind) {
        case A_simpleVar: {
            E_enventry entry = S_look(venv, v->u.simple);

            if (!entry || entry->kind == E_funEntry) {
                EM_error(v->pos, "undefined variable %s", S_name(v->u.simple));
                return expTy(NULL, Ty_Int());
            }

            return expTy(Tr_simpleVar(entry->u.var.access, level), actual_ty(entry->u.var.ty));
        }

        case A_fieldVar: {
            expty_t t_var = transVar(venv, tenv, v->u.field.var, level, label);
            if (t_var.ty->kind != Ty_record) {
                EM_error(v->u.field.var->pos, "not a record type");
                return expTy(NULL, Ty_Void());
            }
            Ty_field_wrap field_wrap = FieldList_lookup(t_var.ty->u.record, v->u.field.sym);
            if (!field_wrap) {
                /*
                 * filed not found
                 */
                EM_error(v->u.field.var->pos, "field %s doesn't exist", S_name(v->u.field.sym));
                return expTy(NULL, Ty_Void());
            }
            return expTy(Tr_fieldVar(t_var.exp, field_wrap->offset * F_wordSize), field_wrap->field->ty);
        }

        case A_subscriptVar: {
            /*
             * subscript of a array
             */
            expty_t t_var = transVar(venv, tenv, v->u.subscript.var, level, label);
            if (t_var.ty->kind != Ty_array) {
                /* var is not a array */
                EM_error(v->u.subscript.var->pos, "array type required");
            }
            expty_t t_exp = transExp(venv, tenv, v->u.subscript.exp, level, label);
            if (t_exp.ty->kind != Ty_int) {
                /* exp is not integer */
                EM_error(v->u.subscript.exp->pos, "exp of subscript should be int");
            }

            return expTy(Tr_subscriptVar(t_var.exp, t_exp.exp), t_var.ty->u.array);
        }
    }
}

struct expty transExp(S_table venv, S_table tenv, A_exp aExp, Tr_level level, Temp_label label)
{
    if (!aExp) {
        return expTy(NULL, Ty_Void());
    }

    switch (aExp->kind) {
        case A_varExp: {
            return transVar(venv, tenv, aExp->u.var, level, label);
        }
        case A_intExp: {
            return expTy(Tr_intExp(aExp->u.intt), Ty_Int());
        }
        case A_stringExp: {
            return expTy(Tr_stringExp(aExp->u.stringg), Ty_String());
        }
        case A_callExp: {
            E_enventry f_entry = S_look(venv, aExp->u.call.func);
            if (!f_entry || f_entry->kind != E_funEntry) {
                EM_error(aExp->pos, "undefined function %s", S_name(aExp->u.call.func));
                return expTy(NULL, Ty_Int());
            }
            Tr_expList list = check_function_params(venv, tenv, aExp->pos, aExp->u.call.args, f_entry->u.fun.formals,
                                                   aExp->u.call.func, level, label);
             return expTy(Tr_callExp(f_entry->u.fun.label, level, f_entry->u.fun.level, list),
                        f_entry->u.fun.result);
        }
        case A_opExp: {
            A_oper oper = aExp->u.op.oper;
            struct expty left = transExp(venv, tenv, aExp->u.op.left, level, label);
            struct expty right = transExp(venv, tenv, aExp->u.op.right, level, label);

            if (oper == A_plusOp || oper == A_minusOp
                    || oper == A_timesOp || oper == A_divideOp) {
                if (left.ty->kind != Ty_int) {
                    EM_error(aExp->u.op.left->pos, "integer required");
                }
                if (right.ty->kind != Ty_int) {
                    EM_error(aExp->u.op.right->pos, "integer required");
                }
                return expTy(Tr_opExp(oper, left.exp, right.exp), Ty_Int());
            } else {
                if (left.ty->kind != right.ty->kind) {
                    if (right.ty->kind != Ty_nil) {
                        EM_error(aExp->u.op.right->pos, "same type required");
                    }
                }

                if (oper != A_eqOp && oper != A_neqOp) {
                    if (left.ty->kind != Ty_int && left.ty->kind != Ty_string) {
                        EM_error(aExp->u.op.left->pos, "Compare unsupport type");
                    }
                }
                return expTy(Tr_opExp(oper, left.exp, right.exp), Ty_Int());
            }
        }

        case A_seqExp: {
            A_expList list = aExp->u.seq;
            if (list == NULL || list->head == NULL) {
                return expTy(NULL, Ty_Void());
            }
            expty_t exp_ty = expTy(NULL, Ty_Void());
            while (list && list->head) {
                exp_ty = transExp(venv, tenv, list->head, level, label);
                list = list->tail;
            }

            return exp_ty;
        }

        case A_assignExp: {
            /*
             * assign a var
             */
            expty_t var_ty = transVar(venv, tenv, aExp->u.assign.var, level, label);
            if (var_ty.ty->kind == Ty_void) {
                return expTy(NULL, Ty_Void());
            }
            expty_t exp_ty = transExp(venv, tenv, aExp->u.assign.exp, level, label);
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
            expty_t test_ty = transExp(venv, tenv, aExp->u.iff.test, level, label);
            expty_t then_ty = transExp(venv, tenv, aExp->u.iff.then, level, label);
            expty_t else_ty = transExp(venv, tenv, aExp->u.iff.elsee, level, label);
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
                // EM_error(aExp->u.iff.elsee->pos, "then exp and else exp type mismatch");
            }
            if (then_ty.ty->kind == Ty_nil) {
                EM_error(aExp->u.iff.then->pos, "then exp return type can't be nil");
            }
            if (else_ty.ty->kind == Ty_nil) {
                // EM_error(aExp->u.iff.elsee->pos, "else exp return type can't be nil");
            }

            Tr_exp ee = Tr_ifExp(test_ty.exp, then_ty.exp, else_ty.exp, level);
            return expTy(ee, then_ty.ty);
        }

        case A_whileExp: {
            expty_t test_ty = transExp(venv, tenv, aExp->u.whilee.test, level, label);
            expty_t body_ty = transExp(venv, tenv, aExp->u.whilee.body, level, label);
            if (test_ty.ty->kind != Ty_int) {
                EM_error(aExp->u.whilee.test->pos, "test exp must return integer");
            }
            if (body_ty.ty->kind != Ty_void) {
                EM_error(aExp->u.whilee.body->pos, "while body must produce no value");
            }
            return expTy(Tr_whileExp(test_ty.exp, body_ty.exp), Ty_Void());
        }

        case A_forExp:
        {
            return transExp(venv, tenv, A_TryForToLetExp(aExp), level, label);
            /*
             * S_beginScope(venv);
             * S_enter(venv, aExp->u.forr.var, E_ROVarEntry(Tr_allocLocal(level, TRUE), Ty_Int()));

             * expty_t exp1_ty = transExp(venv, tenv, aExp->u.forr.lo, level, label);
             * if (exp1_ty.ty->kind != Ty_int) {
             *     EM_error(aExp->u.forr.lo->pos, "for exp's range type is not integer");
             * }

             * expty_t exp2_ty = transExp(venv, tenv, aExp->u.forr.hi, level, label);
             * if (exp2_ty.ty->kind != Ty_int) {
             *     EM_error(aExp->u.forr.hi->pos, "for exp's range type is not integer");
             * }

             * expty_t exp3_ty = transExp(venv, tenv, aExp->u.forr.body, level, label);
             * if (exp3_ty.ty->kind != Ty_void) {
             *     EM_error(aExp->u.forr.body->pos, "for loop body must return no value");
             * }

             * S_endScope(venv);


             * // return transExp(venv, tenv, A_TryForToLetExp(aExp), level, label);
             * return expTy(NULL, Ty_Void());
             */
        }

        case A_breakExp: {
            return expTy(Tr_breakExp(level), Ty_Void());
        }

        case A_letExp: {
            expty_t exp;
            A_decList d;
            S_beginScope(venv);
            S_beginScope(tenv);
            for (d = aExp->u.let.decs; d; d = d->tail) {
                transDec(venv, tenv, d->head, level, label);
            }
            exp = transExp(venv, tenv, aExp->u.let.body, level, label);
            S_endScope(tenv);
            S_endScope(venv);

            return expTy(Tr_letExp(exp.exp, level), NULL);
        }

        case A_arrayExp: {
            Ty_ty base_ty = S_look(tenv, aExp->u.array.typ);
            if (!base_ty) {
                EM_error(aExp->pos, "array type %s not defined", S_name(aExp->u.array.typ));
                return expTy(NULL, Ty_Void());
            }
            expty_t array_ty = transExp(venv, tenv, aExp->u.array.init, level, label);
            expty_t size_ty = transExp(venv, tenv, aExp->u.array.size, level, label);
            if (array_ty.ty->kind != base_ty->u.array->kind) {
                EM_error(aExp->u.array.init->pos, "type mismatch");
            }
            if (size_ty.ty->kind != Ty_int) {
                EM_error(aExp->u.array.size->pos, "array size should be integer");
            }
            return expTy(Tr_arrayExp(size_ty.exp, array_ty.exp), base_ty);
        }

        case A_recordExp:
        {
            Ty_ty record_ty = S_look(tenv, aExp->u.record.typ);

            if (!record_ty) {
                EM_error(aExp->pos, "undefined type %s", S_name(aExp->u.record.typ));
                return expTy(NULL, Ty_Record(NULL));
            }
            if (record_ty->kind != Ty_record) {
                EM_error(aExp->pos, "%s is not a record", S_name(aExp->u.record.typ));
                return expTy(NULL, Ty_Name(aExp->u.record.typ, Ty_Record(record_ty->u.record)));
            }

            Tr_expList expList;
            if (!aExp->u.record.fields && record_ty->u.record) {
                EM_error(aExp->pos, "too less assign to record");
                expList = NULL;
            } else {
                expList = check_fields_match(aExp->pos, venv, tenv, aExp->u.record.fields, record_ty->u.record, level);
            }

            /* A_exp trExp = A_fieldToExp(aExp->pos, aExp->u.record.fields, aExp->u.record.typ); */
            /* return transExp(venv, tenv, trExp, level, label); */
            return expTy(Tr_recordExp(expList), record_ty);
        }

        case A_nilExp:
            return expTy(Tr_nilExp(), Ty_Nil());
        default:
            return expTy(NULL, Ty_Void());

    }
}

Tr_exp transDec(S_table venv, S_table tenv, A_dec dec, Tr_level level, Temp_label label)
{
    switch (dec->kind) {
        case A_varDec: {
            expty_t e = transExp(venv, tenv, dec->u.var.init, level, label);
            if (e.ty->kind == Ty_nil) {
                if (!dec->u.var.typ ||
                        strlen(S_name(dec->u.var.typ)) == 0)
                    EM_error(dec->u.var.init->pos, "init should not be nil without type specified");
            }

            Tr_access access = Tr_allocLocal(level, TRUE);
            if (dec->u.var.typ &&
                    strlen(S_name(dec->u.var.typ)) != 0) {
                Ty_ty var_ty = S_look(tenv, dec->u.var.typ);
                if (e.ty->kind != var_ty->kind) {
                    if (e.ty->kind == Ty_nil) {
                        if (var_ty->kind != Ty_record) {
                            EM_error(dec->u.var.init->pos, "Nil can't not assign to var that is not record");
                        }
                    } else {
                        EM_error(dec->u.var.init->pos, "type mismatch");
                    }
                }

                S_enter(venv, dec->u.var.var, E_VarEntry(access, var_ty));
            } else {
                S_enter(venv, dec->u.var.var, E_VarEntry(access, e.ty));
            }

            return Tr_varDec(access);
        }

        case A_typeDec: {
            transTypeDecHeader(venv, tenv, dec->u.type);
            transTypeDecBody(venv, tenv, dec->u.type);
            if (checkTypeDecCircles(tenv, dec->u.type) == 1) {
                EM_error(dec->pos, "illegal type cycle");
            }
            return Tr_typeDec();
        }
        case A_functionDec: {
            transFunDecHeader(venv, tenv, dec->u.function, level);
            Tr_expList expList = transFunDecBody(venv, tenv, dec->u.function, level);

            return Tr_funListDec(expList);
        }
    }

    assert(0);
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

F_fragList SEM_transProg(A_exp exp)
{
    S_table tenv = E_base_tenv();
    S_table venv = E_base_venv();
    Tr_level outermost_lv = Tr_outermost();
    Tr_init();
    Temp_label label = Temp_newlabel();
    transExp(venv, tenv, exp, outermost_lv, label);
    return Tr_getResult();
}


Tr_expList check_function_params(S_table venv, S_table tenv, int pos,
                          A_expList args, Ty_tyList standards, S_symbol func,
                          Tr_level level, Temp_label label) {
    if (args == NULL && standards == NULL) {
        return NULL;
    }

    if (args == NULL && standards != NULL) {
        EM_error(pos, "too less params");
        return NULL;
    }

    if (standards == NULL && args != NULL) {
        EM_error(args->head->pos, "too many params in function %s", S_name(func));
        return NULL;
    }

    expty_t ty = transExp(venv, tenv, args->head, level, label);
    if (ty.ty->kind != standards->head->kind) {
        // EM_error(args->head->pos, "para type mismatch");
    }

    return Tr_ExpList(ty.exp, check_function_params(venv, tenv, args->head->pos,
                                                    args->tail, standards->tail, func, level, label));
}

Ty_tyList makeFormalTyList(S_table tenv, A_fieldList params)
{
    if (params == NULL) {
        return NULL;
    }

    Ty_ty ty = S_look(tenv, params->head->typ);
    if (!ty) {
        printf("unknow param type: %s\n", S_name(params->head->typ));
    }
    return Ty_TyList(ty, makeFormalTyList(tenv, params->tail));
}


Tr_expList check_fields_match(int pos, S_table venv, S_table tenv,
                        A_efieldList args, Ty_fieldList standards,
                        Tr_level level) {
    if (!args) {
        if (standards) {
            EM_error(pos, "too less assign to record");
        }
        return NULL;
    }

    if (!standards) {
        EM_error(args->head->exp->pos, "too match assign to record");
        return NULL;
    }

    Tr_exp tExp = check_field_match(venv, tenv, args->head, standards->head, level);
    return Tr_ExpList(tExp, check_fields_match(args->head->exp->pos, venv, tenv,
                args->tail, standards->tail, level));

    /*
     * A_efieldList e_list = args;
     * Ty_fieldList s_list = standards;
     * A_efield efield;
     * A_efield poss;
     * Ty_field field;
     * while (e_list && s_list) {
     *     efield = e_list->head;
     *     field = s_list->head;

     *     Tr_exp trExp = check_field_match(venv, tenv, efield, field, level);
     *     pos = efield;

     *     e_list = e_list->tail;
     *     s_list = s_list->tail;
     * }

     * if (e_list) {
     *     EM_error(e_list->head->exp->pos, "too match assign to record");
     * }
     * if (s_list) {
     *     EM_error(poss->exp->pos, "too less assign to record");
     * }
     */
}

Tr_exp check_field_match(S_table venv, S_table tenv, A_efield ef, Ty_field tf, Tr_level level)
{
    if (ef->name != tf->name) {
        EM_error(ef->exp->pos, "field name not match declared");
    }

    expty_t ef_ty = transExp(venv, tenv, ef->exp, level, ef->name);
    if (ef_ty.ty->kind != tf->ty->kind) {
        if (ef_ty.ty->kind == Ty_nil &&
                real_ty(tenv, tf->ty)->kind == Ty_record) {
        } else {
            EM_error(ef->exp->pos, "assign exp value not match declared");
        }
    }
    return ef_ty.exp;
}

void transTypeDecHeader(S_table venv, S_table tenv, A_nametyList types)
{
    if (types == NULL) {
        return;
    }

    Ty_ty type_ty = S_look(tenv, types->head->name);
    if (type_ty != NULL) {
        // EM_error(0, "two types have the same name", S_name(types->head->name));
    } else {
        S_enter(tenv, types->head->name, Ty_Name(types->head->name, NULL));
    }

    transTypeDecHeader(venv, tenv, types->tail);
}

void transTypeDecBody(S_table venv, S_table tenv, A_nametyList types)
{
    if (types == NULL) {
        return;
    }

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


void transFunDecHeader(S_table venv, S_table tenv, A_fundecList fundecList,
                       Tr_level level)
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

    E_enventry fun_entry = S_look(venv, fundecList->head->name);
    if (fun_entry != NULL && fun_entry->kind != E_funEntry) {
        EM_error(fundecList->head->pos, "two functions have the same name", fundecList->head->pos);
    } else {
        Temp_label func_name = Temp_namedlabel(S_name(fundecList->head->name));
        S_enter(venv, fundecList->head->name, E_FunEntry(
                Tr_newLevel(level, func_name, paramsToEscapes(fundecList->head->params)),
                func_name,
                transFuncParams(tenv, fundecList->head->params),
                result_ty
        ));
    }

    transFunDecHeader(venv, tenv, fundecList->tail, level);
}

U_boolList paramsToEscapes(A_fieldList params) {
    if (params == NULL) {
        return NULL;
    }

    return U_BoolList(TRUE, paramsToEscapes(params->tail));
}

Tr_expList transFunDecBody(S_table venv, S_table tenv, A_fundecList fundecList, Tr_level level)
{
    /*
     * parse function body
     */
    if (fundecList == NULL) {
        return NULL;
    }

    Ty_tyList formalTys = transFuncParams(tenv, fundecList->head->params);

    S_beginScope(venv);

    A_fieldList l;
    Ty_tyList t;
    for (l = fundecList->head->params, t = formalTys; l; l = l->tail, t = t->tail) {
        S_enter(venv, l->head->name, E_VarEntry(Tr_allocLocal(level, TRUE), t->head));
    }

    E_enventry fun_env = S_look(venv, fundecList->head->name);
    if (!fun_env || fun_env->kind == E_varEntry) {
        EM_error(fundecList->head->pos, "function %s not declared\n", S_name(fundecList->head->name));
    }
    expty_t body_ty = transExp(venv, tenv, fundecList->head->body, fun_env->u.fun.level, fun_env->u.fun.label);

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

    Tr_exp func_exp = Tr_funDec(body_ty.exp, fun_env->u.fun.level);

    return Tr_ExpList(func_exp,
                      transFunDecBody(venv, tenv, fundecList->tail, level));
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

