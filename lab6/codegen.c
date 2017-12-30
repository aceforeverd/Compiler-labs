#include "absyn.h"
#include "codegen.h"
#include "errormsg.h"
#include "symbol.h"
#include "table.h"
#include "temp.h"
#include "tree.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Lab 6: put your code here
#define INSTLEN 128

static char *AS_ADD = "ADD";
static char *AS_ADDI = "ADDI";
static char *AS_SUB = "SUB";
static char *AS_SUBI = "SUBI";
static char *AS_MUL = "MUL";
static char *AS_DIV = "DIV";
static char *AS_MOVE = "MOVE";
static char *AS_STORE = "STORE";
static char *AS_LOAD = "LOAD";
static char *AS_JMP = "JMP";
static char *AS_CALL = "CALL";
static char *AS_UNKNOW = "UNKNOW";


/* a list of instructions */
static AS_instrList iList = NULL;
/* the tail */
static AS_instrList last = NULL;

static Temp_temp munchExp(T_exp e);
static Temp_temp munchMemExp(T_exp e);
static Temp_temp munchBinExp(T_exp e);
static Temp_tempList munchArgs(int n, T_expList args);
static void munchMoveStm(T_stm stm);
static void munchStm(T_stm stm);

/*
 * accumulate a list of instructions to be returned later
 * just append inst to the end of last
 */
static void emit(AS_instr inst) {
    if (last != NULL) {
        last->tail = AS_InstrList(inst, NULL);
        last = last->tail;
    } else {
        iList = AS_InstrList(inst, NULL);
        last = iList;
    }
}

AS_instrList F_codegen(F_frame f, T_stmList stmList) {
    AS_instrList list;
    T_stmList sl;
    /*
     * miscellaneous initializations as necessary
     */

    for (sl = stmList; sl; sl = sl->tail) {
        munchStm(sl->head);
    }

    list = iList;
    iList = last = NULL;
    return list;
}

Temp_tempList L(Temp_temp h, Temp_tempList t) {
    return Temp_TempList(h, t);
}

static Temp_temp munchExp(T_exp e) {
    assert(e);

    char str[INSTLEN];
    switch (e->kind) {
        case T_MEM:
            return munchMemExp(e);
        case T_BINOP:
            return munchBinExp(e);
        case T_CONST:
            {
                Temp_temp r = Temp_newtemp();
                sprintf(str, "%s `d0 <- `s0 + %d\n",AS_ADDI, e->u.CONST);
                emit(AS_Oper(str, NULL, NULL, NULL));
                return r;
            }
        case T_TEMP:
            return e->u.TEMP;
        case T_NAME:
            {
                sprintf(str, "%s: ", Temp_labelstring(e->u.NAME));
                emit(AS_Label(str, e->u.NAME));

                /*! TODO: return need fix
                */
                return Temp_newtemp();
            }

        case T_ESEQ:
            munchStm(e->u.ESEQ.stm);
            return munchExp(e->u.ESEQ.exp);
        case T_CALL:
            {
                Temp_temp r = munchExp(e->u.CALL.fun);
                Temp_tempList list = munchArgs(0, e->u.CALL.args);
                sprintf(str, "%s `s0\n", AS_CALL);
                emit(AS_Oper(str, CallDefs(), L(r, list), NULL));
            }

    }

    /* you can not touch here */
    assert(0);
}

static void munchStm(T_stm stm) {
    if (stm == NULL) {
        return;
    }

    char instr[INSTLEN];
    switch(stm->kind) {
        case T_MOVE:
            munchMoveStm(stm);
            return;
        case T_LABEL:
            sprintf(instr, "%s\n", Temp_labelstring(stm->u.LABEL));
            emit(AS_Label(instr, stm->u.LABEL));
            return;
        case T_SEQ:
            munchStm(stm->u.SEQ.left);
            munchStm(stm->u.SEQ.right);
            return;
        case T_EXP:
            munchExp(stm->u.EXP);
            return;
        case T_JUMP:
            sprintf(instr, "%s `j0\n", AS_JMP);
            emit(AS_Oper(instr, NULL, Temp_TempList(munchExp(stm->u.JUMP.exp), NULL),
                        AS_Targets(stm->u.JUMP.jumps)));
            return;
        case T_CJUMP:
            /*! TODO: not complete
             */
            munchExp(stm->u.CJUMP.left);
            munchExp(stm->u.CJUMP.right);
    }

    /* you can not touch here */
    assert(0);
}

static Temp_temp munchMemExp(T_exp e) {
    Temp_temp r = Temp_newtemp();
    char str[INSTLEN];
    T_exp exp = e->u.MEM;

    if (exp->kind == T_BINOP && exp->u.BINOP.op == T_plus) {
        if (exp->u.BINOP.left->kind == T_CONST) {
            /* MEM(BINOP(oper, CONST(i), e1)) */
            sprintf(str, "%s `d0 <- M[`s0+%d]\n", AS_LOAD, exp->u.BINOP.left->u.CONST);
            emit(AS_Oper(str, L(r, NULL), L(munchExp(exp->u.BINOP.right), NULL),
                         NULL));
            return r;
        }

        if (exp->u.BINOP.right->kind == T_CONST) {
            /* MEM(BINOP(oper, e1, CONST(i))) */
            sprintf(str, "%s `d0 <- M[`s0+%d]\n", AS_LOAD,
                    exp->u.BINOP.right->u.CONST);
            emit(AS_Oper(str, L(r, NULL), L(munchExp(exp->u.BINOP.left), NULL),
                         NULL));
        }
    }

    if (exp->kind == T_CONST) {
        /* MEM(CONST(i)) */
        sprintf(str, "%s `d0 <- M[r0+%d]\n", AS_LOAD, exp->u.CONST);
        emit(AS_Oper(str, L(r, NULL), NULL, NULL));
        return r;
    }

    /* MEM(e) */
    sprintf(str, "%s `d0 <- M[r0+0]\n", AS_LOAD);
    emit(AS_Oper(str, L(r, NULL),
                L(munchExp(exp), NULL), NULL
                ));
    return r;
}

static Temp_temp munchBinExp(T_exp e) {
    char oper;
    char cmd[8];
    switch(e->u.BINOP.op) {
        case T_plus: {
            oper = '+';
            strncpy(cmd, AS_ADD, sizeof(cmd));
            if (e->u.BINOP.left->kind == T_CONST ||
                    e->u.BINOP.right->kind == T_CONST) {
                strncpy(cmd, AS_ADDI, sizeof(cmd));
            }
            break;
                     }
        case T_minus:
            oper = '-';
            strncpy(cmd, AS_SUB, sizeof(cmd));
            if (e->u.BINOP.left->kind == T_CONST ||
                    e->u.BINOP.right->kind == T_CONST) {
                strncpy(cmd, AS_SUBI, sizeof(cmd));
            }
            break;
        case T_mul:
            oper = '*';
            strncpy(cmd, AS_MUL, sizeof(cmd));
            break;
        case T_div:
            oper = '/';
            strncpy(cmd, AS_DIV, sizeof(cmd));
            break;
        default:
            oper = '?';
            strncpy(cmd, AS_UNKNOW, sizeof(cmd));
    }

    char str[INSTLEN];
    Temp_temp r = Temp_newtemp();

    if (e->u.BINOP.op == T_plus || e->u.BINOP.op == T_minus) {
        /*
         * work with ADDI and SUBI
         */
        if (e->u.BINOP.left->kind == T_CONST && e->u.BINOP.op == T_plus) {
            /* ADDI */
            sprintf(str, "%s `d0 <- `s0 %c %d\n", cmd, oper, e->u.BINOP.left->u.CONST);
            emit(AS_Oper(str,
                        L(r, NULL), L(munchExp(e->u.BINOP.right), NULL), NULL));
            return r;
        }

        if (e->u.BINOP.right->kind == T_CONST) {
            /* ADDI or SUBI */
            sprintf(str, "%s `d0 <- `s0 %c %d\n", cmd, oper, e->u.BINOP.right->u.CONST);
            emit(AS_Oper(str,
                        L(r, NULL), L(munchExp(e->u.BINOP.left), NULL), NULL
                        ));
            return r;
        }
    }

    /*
     * the normal things
     * ADD, SUB, MUL, DIV
     */
    sprintf(str, "%s `d0 <- `s0 %c `s1\n", cmd, oper);
    emit(AS_Oper(str,
                L(r, NULL), L(munchExp(e->u.BINOP.left),
                    L(munchExp(e->u.BINOP.right), NULL)), NULL
                ));
    return r;
}


static void munchMoveStm(T_stm stm) {
    T_exp dst = stm->u.MOVE.dst;
    T_exp src = stm->u.MOVE.src;
    char instr[INSTLEN];

    /*
     * LOAD
     */
    if (dst->kind == T_MEM) {
        T_exp mem_exp = dst->u.MEM;
        if (mem_exp->kind == T_BINOP) {
            if (mem_exp->u.BINOP.left->kind == T_CONST) {
                /*
                 * STORE M[e1+CONST(i)] <- e2
                 */
                sprintf(instr, "%s M[`s0+%d] <- `s1\n", AS_STORE, mem_exp->u.BINOP.left->u.CONST);
                emit(AS_Oper(instr, NULL,
                            L(munchExp(mem_exp->u.BINOP.right),
                                L(munchExp(src), NULL)), NULL
                                ));
                return;
            }

            if (mem_exp->u.BINOP.right->kind == T_CONST) {
                /*
                 * STORE M[e1+CONST(i)] <- e2
                 */
                sprintf(instr, "%s M[`s0+%d] <- `s1\n",AS_STORE, mem_exp->u.BINOP.right->u.CONST);
                emit(AS_Oper(instr, NULL,
                            L(munchExp(mem_exp->u.BINOP.left),
                                L(munchExp(src), NULL)), NULL
                            ));
                return;
            }
        }

        if (src->kind == T_MEM) {
            /*
             * MOVE(MEM(e1), MEM(e2))
             */
            sprintf(instr, "%s M[`s0] <- M[`s1]\n", AS_MOVE);
            emit(AS_Oper(instr, NULL,
                        L(munchExp(dst->u.MEM),
                            L(munchExp(src->u.MEM), NULL)), NULL
                        ));
            return;
        }

        if (mem_exp->kind == T_CONST) {
            /* MOVE(MEM(CONST(i)), e) */
            sprintf(instr, "%s M[`r0+%d] <- `s0\n", AS_STORE, mem_exp->u.CONST);
            emit(AS_Oper(instr, NULL,
                        L(munchExp(src), NULL), NULL
                        ));
            return;
        }

        /* MOVE(MEM(a), e) */
        sprintf(instr, "%s M[`s0] <- `s1\n", AS_STORE);
        emit(AS_Oper(instr, NULL, L(munchExp(mem_exp),
                        L(munchExp(src), NULL)), NULL
                        ));
        return;
    }

    if (dst->kind == T_TEMP) {
        sprintf(instr, "%s `d0 <- `s0 + r0\n", AS_ADD);
        emit(AS_Move(instr, Temp_TempList(dst->u.TEMP, NULL),
                    Temp_TempList(munchExp(src), NULL)));
        return;
    }
}

static Temp_tempList munchArgs(int n, T_expList args) {
    if (args == NULL) {
        return NULL;
    }

    return Temp_TempList(munchExp(args->head),
            munchArgs(n + 1, args->tail));
}
