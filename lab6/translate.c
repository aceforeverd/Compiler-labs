#include "absyn.h"
#include "frame.h"
#include "printtree.h"
#include "runtime.c"
#include "symbol.h"
#include "table.h"
#include "temp.h"
#include "translate.h"
#include "tree.h"
#include "util.h"
#include <stdio.h>
#include <string.h>


struct Tr_access_ {
    Tr_level level;
    F_access access;
};

struct Tr_accessList_ {
    Tr_access head;
    Tr_accessList tail;
};

struct Tr_level_ {
    F_frame frame;
    Tr_level parent;
};

typedef struct patchList_ *patchList;
struct patchList_ {
    Temp_label *head;
    patchList tail;
};

struct Cx {
    patchList trues;
    patchList falses;
    T_stm stm;
};
typedef struct Cx *Cx_t;

struct Tr_exp_ {
    enum { Tr_ex, Tr_nx, Tr_cx } kind;
    union {
        T_exp ex;
        T_stm nx;
        struct Cx cx;
    } u;
};

struct Tr_expList_ {
    Tr_exp head;
    Tr_expList tail;
};

Tr_expList  Tr_ExpList(Tr_exp head, Tr_expList tail) {
    Tr_expList list = checked_malloc(sizeof(*list));

    list->head = head;
    list->tail = tail;

    return list;
}

static struct Tr_level_ outermost_level = {NULL, NULL};
static F_fragList a_fraglist = NULL;
static F_fragList fragList_tail = NULL;
/*
 * function declarations
 */
static Tr_exp Tr_Ex(T_exp ex);
static Tr_exp Tr_Nx(T_stm nx);
static Tr_exp Tr_Cx(patchList trues, patchList falses, T_stm stm);

static Tr_access Tr_Access(Tr_level level, F_access access);
Tr_level Tr_Level(F_frame f, Tr_level l);
void doPatch(patchList tList, Temp_label label);
patchList joinPatch(patchList first, patchList second);
static T_exp unEx(Tr_exp e);
static T_stm unNx(Tr_exp e);
static struct Cx unCx(Tr_exp e);
static T_expList unExList(Tr_expList list);

static Tr_exp Tr_Ex(T_exp ex) {
    Tr_exp exp = checked_malloc(sizeof(*exp));

    exp->kind = Tr_ex;
    exp->u.ex = ex;

    return exp;
}

static Tr_exp Tr_Nx(T_stm nx) {
    Tr_exp exp = checked_malloc(sizeof(*exp));

    exp->kind = Tr_nx;
    exp->u.nx = nx;

    return exp;
}

static Tr_exp Tr_Cx(patchList trues, patchList falses, T_stm stm) {
    Tr_exp exp = checked_malloc(sizeof(*exp));

    exp->kind = Tr_cx;
    exp->u.cx.trues = trues;
    exp->u.cx.falses = falses;
    exp->u.cx.stm = stm;

    return exp;
}

static Tr_access Tr_Access(Tr_level level, F_access access) {
    Tr_access ta = checked_malloc(sizeof(*ta));

    ta->level = level;
    ta->access = access;

    return ta;
}

/*
 * static Tr_accessList TrtoF_accessList(F_accessList list) {
 *     return Tr_AccessList(
 *
 *             );
 * }
 */

static patchList PatchList(Temp_label *head, patchList tail) {
    patchList list;

    list = (patchList)checked_malloc(sizeof(struct patchList_));
    list->head = head;
    list->tail = tail;
    return list;
}

Tr_level Tr_Level(F_frame f, Tr_level l) {
    Tr_level tr_level = checked_malloc(sizeof(*tr_level));

    tr_level->frame = f;
    tr_level->parent = l;

    return tr_level;
}

static T_exp unEx(Tr_exp e) {
    assert(e);
    /* if (!e) { */
    /*     return T_Const(0); */
    /* } */

    switch (e->kind) {
        case Tr_ex:
            /*
             * ex => ex
             */
            return e->u.ex;
        case Tr_cx: {
            /*
             * cx => ex
             */
            Temp_temp r = Temp_newtemp();
            Temp_label t = Temp_newlabel();
            Temp_label f = Temp_newlabel();
            doPatch(e->u.cx.trues, t);
            doPatch(e->u.cx.falses, f);
            return T_Eseq(
                T_Move(T_Temp(r), T_Const(1)),
                T_Eseq(
                    e->u.cx.stm,
                    T_Eseq(T_Label(f), T_Eseq(T_Move(T_Temp(r), T_Const(0)),
                                              T_Eseq(T_Label(t), T_Temp(r))))));
        }
        case Tr_nx:
            /*
             * nx => ex
             */
            return T_Eseq(e->u.nx, T_Const(0));
    }

    assert(0);
}

static T_stm unNx(Tr_exp e) {
    assert(e);
    /* if (!e || !e->kind) { */
    /*     return T_Exp(T_Const(0)); */
    /* } */

    switch (e->kind) {
        case Tr_ex:
            assert(e->u.ex);
            return T_Exp(e->u.ex);
        case Tr_nx:
            assert(e->u.nx);
            return e->u.nx;
        case Tr_cx: {
            Temp_temp r = Temp_newtemp();
            Temp_label t = Temp_newlabel();
            Temp_label f = Temp_newlabel();
            doPatch(e->u.cx.trues, t);
            doPatch(e->u.cx.falses, f);
            return T_Seq(
                    T_Move(T_Temp(r), T_Const(1)),
                    T_Seq(
                        e->u.cx.stm,
                        T_Seq(T_Label(f), T_Seq(T_Move(T_Temp(r), T_Const(0)),
                                T_Seq(T_Label(t), T_Exp(T_Temp(r)))))));
        }
    }
}

static struct Cx unCx(Tr_exp e) {
    assert(e);

    /* if (!e || !e->kind) { */
    /*     cx.stm = T_Exp(T_Const(0)); */
    /*     cx.falses = NULL; */
    /*     cx.trues = NULL; */
    /*     return cx; */
    /* } */
    struct Cx cx;

    switch (e->kind) {
        case Tr_ex: {
            /*
             * ex => cx
             * this is not so useful ?
             */
            assert(e->u.ex);
            T_exp exp = e->u.ex;
            cx.stm = T_Exp(exp);
            if (exp->kind == T_CONST) {

            }
        }
        case Tr_nx:
            assert(e->u.nx);
            printf("unCx see a Tr_exp with kind Tr_nx\n");
            cx.trues = NULL;
            cx.falses = NULL;
            cx.stm = e->u.nx;
            return cx;
        case Tr_cx:
            return e->u.cx;
    }

    assert(0);
}

static T_expList unExList(Tr_expList list) {
    if (!list) {
        return NULL;
    }

    return T_ExpList(unEx(list->head), unExList(list->tail));
}


void doPatch(patchList tList, Temp_label label) {
    for (; tList; tList = tList->tail) {
        *(tList->head) = label;
    }
}

patchList joinPatch(patchList first, patchList second) {
    if (!first) {
        return second;
    }

    for (; first->tail; first = first->tail) {
    };

    first->tail = second;
    return first;
}

Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail) {
    Tr_accessList l = checked_malloc(sizeof(*l));
    l->head = head;
    l->tail = tail;
    return l;
}

Tr_level Tr_outermost(void) {
    return &outermost_level;
}

Tr_level Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals) {
    return Tr_Level(F_newFrame(name, formals), parent);
}

Tr_accessList Tr_formals(Tr_level level) {
    F_formals(level->frame);
}

Tr_access Tr_allocLocal(Tr_level level, bool escape) {
    return Tr_Access(level, F_allocLocal(level->frame, escape));
}

void Tr_procEntryExit(Tr_level level, Tr_exp body, Tr_accessList formals) {

}

F_fragList Tr_getResult() {
    return a_fraglist;
}

void Tr_fragListAdd(F_frag frag) {
    a_fraglist = F_FragList(frag, a_fraglist);
}

void Tr_init() {
    outermost_level.frame = outermost_frame();
    outermost_level.parent = NULL;

    /* a_fraglist = F_FragList(F_ProcFrag(NULL, outermost_level.frame), NULL); */
}

/* @declared_access: Tr_access of decalcification
 * @call_level: Tr_level of invocation
 */
Tr_exp Tr_simpleVar(Tr_access declared_access, Tr_level call_level) {
    return Tr_Ex(F_Exp(declared_access->access,
                       follow_static_link(call_level, declared_access->level)));
}

/*
 * get the frame pointer of the F_frame where declared that variable/function
 * since register ebp is overwritten, it should query recursively from call level
 */
T_exp follow_static_link(Tr_level call_level, Tr_level dec_level) {
    assert(call_level && dec_level);

    T_exp frame = F_framePtr(call_level->frame);
    Tr_level level = call_level;
    while (level && level != dec_level) {
        frame = F_preFrame(frame);
        level = level->parent;
    }

    return frame;
}

Tr_exp Tr_subscriptVar(Tr_exp head, Tr_exp sub) {
    return Tr_Ex(T_Mem(
                T_Binop(T_plus, T_Mem(unEx(head)),
                    T_Binop(T_mul, unEx(sub), T_Const(F_wordSize)))));
}


Tr_exp Tr_intExp(int var) {
    return Tr_Ex(T_Const(var));
}

Tr_exp Tr_stringExp(string str) {
    Temp_label label = Temp_newlabel();

    Tr_fragListAdd(F_StringFrag(label, str));

    return Tr_Ex(T_Name(label));
}

Tr_exp Tr_assignExp(Tr_exp var, Tr_exp exp) {
    return Tr_Nx(T_Move(unEx(var), unEx(exp)));
}

Tr_exp Tr_opExp(A_oper op, Tr_exp left, Tr_exp right) {
    switch (op) {
        case A_plusOp:
            return Tr_Ex(T_Binop(T_plus, unEx(left), unEx(right)));
        case A_minusOp:
            return Tr_Ex(T_Binop(T_minus, unEx(left), unEx(right)));
        case A_timesOp:
            return Tr_Ex(T_Binop(T_mul, unEx(left), unEx(right)));
        case A_divideOp:
            return Tr_Ex(T_Binop(T_div, unEx(left), unEx(right)));

        case A_ltOp:
            return Tr_Nx(T_Cjump(T_lt, unEx(left), unEx(right), NULL, NULL));
        case A_leOp:
            return Tr_Nx(T_Cjump(T_le, unEx(left), unEx(right), NULL, NULL));
        case A_gtOp:
            return Tr_Nx(T_Cjump(T_gt, unEx(left), unEx(right), NULL, NULL));
        case A_geOp:
            return Tr_Nx(T_Cjump(T_ge, unEx(left), unEx(right), NULL, NULL));

        case A_eqOp:
            return Tr_Nx(T_Cjump(T_eq, unEx(left), unEx(right), NULL, NULL));
        case A_neqOp:
            return Tr_Nx(T_Cjump(T_ne, unEx(left), unEx(right), NULL, NULL));

        default:
            return NULL;
    }
}

Tr_exp Tr_ifExp(Tr_exp test, Tr_exp then, Tr_exp els, Tr_level level) {
    // cj should be a CJUMP
    T_stm cj = unNx(test);
    if (!cj->u.CJUMP.true_l) {
        cj->u.CJUMP.true_l = Temp_newlabel();
    }
    if (!cj->u.CJUMP.false_l) {
        cj->u.CJUMP.false_l = Temp_newlabel();
    }

    T_exp ep = T_Eseq( cj, T_Eseq(
                    T_Seq(T_Label(cj->u.CJUMP.true_l), T_Exp(unEx(then))),
                    T_Eseq(T_Label(cj->u.CJUMP.false_l), unEx(els))
            ));

    return Tr_Ex(ep);
}

Tr_exp Tr_forExp(Tr_level level) {
    /* not used */
    return NULL;
}

Tr_exp Tr_breakExp(Tr_level level) {
    return Tr_Nx(T_Jump(NULL, Temp_LabelList(Temp_namedlabel("done"), NULL)));
}

Tr_exp Tr_whileExp(Tr_exp test, Tr_exp body) {
    T_stm cj = unNx(test);
    if (!cj->u.CJUMP.false_l) {
        cj->u.CJUMP.false_l = Temp_newlabel();
    }
    if (!cj->u.CJUMP.true_l) {
        cj->u.CJUMP.true_l = Temp_newlabel();
    }

    return Tr_Nx(T_Seq(cj,
                T_Seq(T_Seq(T_Label(cj->u.CJUMP.true_l), unNx(body)),
                      T_Label(cj->u.CJUMP.false_l)
    )));
}

Tr_exp Tr_callExp(Temp_label label, Tr_level call_level, Tr_level dec_level, Tr_expList args) {
    /* one more argument, static link */
    T_exp fp = follow_static_link(call_level, dec_level);

    if (fp) {
        return Tr_Ex(T_Call(T_Name(label),
                    T_ExpList(fp, unExList(args))));
    }

    return Tr_Ex(F_externalCall(S_name(label), unExList(args)));
}

Tr_exp Tr_letExp(Tr_exp body, Tr_level level) {
    Tr_fragListAdd(F_ProcFrag(unNx(body), level->frame));

    return body;
}

Tr_exp Tr_fieldVar(Tr_exp var, int order) {
    return Tr_Ex(T_Mem(T_Binop(T_plus, T_Mem(unEx(var)), T_Const(order))));
}

Tr_exp Tr_arrayExp(Tr_exp size, Tr_exp init) {
    T_exp call =F_externalCall("initArray", T_ExpList(unEx(size), T_ExpList(unEx(init), NULL)));

    Temp_temp temp = Temp_newtemp();
    return Tr_Ex(T_Eseq(T_Move(T_Temp(temp), call), T_Temp(temp)));
}

Tr_exp Tr_recordExp(Tr_expList expList) {
    int size = 0;
    Tr_expList list = expList;
    while (list->head) {
        size ++;
        list = list->tail;
    }

    T_exp call = F_externalCall("allocRecord", T_ExpList(T_Const(size * F_wordSize), NULL));
    Temp_temp t = Temp_newtemp();
    return Tr_Ex(T_Eseq(T_Seq(T_Move(T_Temp(t), call),
                    Tr_recordInit(T_Temp(t), expList, 0)),
                T_Temp(t)));
}

T_stm Tr_recordInit(T_exp start, Tr_expList expList, int offset) {
    if (!expList) {
        return NULL;
    }

    return T_Seq(T_Move(
                T_Mem(T_Binop(T_plus, start, T_Const(offset))),
                unEx(expList->head)),
            Tr_recordInit(start, expList->tail, offset + 4));
}


Tr_exp Tr_varDec(Tr_access access) {
    return Tr_Ex(F_Exp(access->access, F_framePtr(access->level->frame)));
}

Tr_exp Tr_arrayDec(Tr_access access, int length, int init) {
    /* int *array = initArray(length, init); */
    // do variable init somewhere else
    return Tr_Ex(T_Mem(F_Exp(access->access,
                    F_framePtr(access->level->frame))));
}


Tr_exp Tr_funListDec(Tr_expList funcList) {
    return Tr_Ex(T_Const(0));
}

/* fun_level->frame has the function name
 * will handle prologue and epilogue of function later */
Tr_exp Tr_funDec(Tr_exp body, Tr_level fun_level) {
    Tr_fragListAdd(F_ProcFrag(unNx(body), fun_level->frame));
    /* just reutnr nothing */
    return Tr_Ex(T_Const(0));
}

Tr_exp Tr_typeDec() {
    return Tr_Ex(T_Const(0));
}

Tr_exp Tr_nilExp() {
    return Tr_Ex(T_Const(0));
}

