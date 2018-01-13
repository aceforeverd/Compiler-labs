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
static char *AS_MUL = "imull";
static char *AS_DIV = "DIV";
static char *AS_MOVE = "movl";
static char *AS_STORE = "movl";
static char *AS_LOAD = "movl";
static char *AS_JMP = "jmp";
static char *AS_CALL = "call";
static char *AS_UNKNOW = "UNKNOW";
static char *AS_CMP = "cmpl";


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
                sprintf(str, "%s $%d, `d0\n", AS_MOVE, e->u.CONST);
                emit(AS_Oper(str, L(r, NULL), NULL, NULL));
                return r;
            }
        case T_TEMP:
            return e->u.TEMP;
        case T_NAME:
            {
                Temp_temp r = Temp_newtemp();
                sprintf(str, "leal %s, `d0\n", S_name(e->u.NAME));
                emit(AS_Oper(str, L(r, NULL), NULL, NULL));
                return r;
                /* return NULL; */
            }

        case T_ESEQ:
            munchStm(e->u.ESEQ.stm);
            return munchExp(e->u.ESEQ.exp);
        case T_CALL:
            {
                assert(e->u.CALL.fun->kind == T_NAME);
                /* munchExp(e->u.CALL.fun); */
                Temp_tempList list = munchArgs(0, e->u.CALL.args);
                sprintf(str, "%s %s\n", AS_CALL, S_name(e->u.CALL.fun->u.NAME));
                emit(AS_Oper(str, CallDefs(), L(F_RV(), list),
                            AS_Targets(Temp_LabelList(e->u.CALL.fun->u.NAME, NULL))));
                return F_RV();
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
            sprintf(instr, "%s ", Temp_labelstring(stm->u.LABEL));
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
            // suck
            munchExp(stm->u.JUMP.exp);
            emit(AS_Oper(instr, NULL, NULL,
                        AS_Targets(stm->u.JUMP.jumps)));
            return;
        case T_CJUMP:
        {
            if (stm->u.CJUMP.left->kind == T_CONST) {
                sprintf(instr, "%s `s0, $%d\n", AS_CMP, stm->u.CJUMP.left->u.CONST);
                emit(AS_Oper(instr, NULL, L(munchExp(stm->u.CJUMP.right), NULL), NULL));
            } else if (stm->u.CJUMP.right->kind == T_CONST) {
                sprintf(instr, "%s $%d, `s0\n", AS_CMP, stm->u.CJUMP.right->u.CONST);
                emit(AS_Oper(instr, NULL, L(munchExp(stm->u.CJUMP.left), NULL), NULL));
            } else {
                Temp_temp right = munchExp(stm->u.CJUMP.right);
                Temp_temp left = munchExp(stm->u.CJUMP.left);
                sprintf(instr, "%s `s1, `s0\n", AS_CMP);
                emit(AS_Oper(instr, NULL, L(left, L(right, NULL)), NULL));
            }

            char jmp_cmd[8];
            switch (stm->u.CJUMP.op) {
                case T_eq:
                    strncpy(jmp_cmd, "je", 8);
                    break;
                case T_ne:
                    strncpy(jmp_cmd, "jne", 8);
                    break;
                case T_lt:
                    strncpy(jmp_cmd, "jl", 8);
                    break;
                case T_gt:
                    strncpy(jmp_cmd, "jg", 8);
                    break;
                case T_le:
                    strncpy(jmp_cmd, "jle", 8);
                    break;
                case T_ge:
                    strncpy(jmp_cmd, "jge", 8);
                    break;
                default:
                    assert(0);
            }
            sprintf(instr, "%s %s\n",jmp_cmd, Temp_labelstring(stm->u.CJUMP.true_l));
            emit(AS_Oper(instr, NULL, NULL,
                        AS_Targets(Temp_LabelList(stm->u.CJUMP.true_l, NULL))));
            return ;
            }
    }

    /* you can not touch here */
    assert(0);
}

static Temp_temp munchMemExp(T_exp e) {
    assert(e->kind == T_MEM);
    Temp_temp r = Temp_newtemp();
    char str[INSTLEN];
    T_exp exp = e->u.MEM;

    if (exp->kind == T_BINOP && exp->u.BINOP.op == T_plus) {
        if (exp->u.BINOP.left->kind == T_CONST) {
            /* MEM(BINOP(oper, CONST(i), e1)) */
            sprintf(str, "%s %d(`s0), `d0\n", AS_LOAD, exp->u.BINOP.left->u.CONST);
            emit(AS_Oper(str, L(r, NULL), L(munchExp(exp->u.BINOP.right), NULL),
                         NULL));
            return r;
        }

        if (exp->u.BINOP.right->kind == T_CONST) {
            /* MEM(BINOP(oper, e1, CONST(i))) */
            sprintf(str, "%s %d(`s0), `d0\n", AS_LOAD,
                    exp->u.BINOP.right->u.CONST);
            Temp_temp left = munchExp(exp->u.BINOP.left);
            emit(AS_Oper(str, L(r, NULL), L(left, NULL),
                         NULL));
            return r;
        }
    }

    if (exp->kind == T_CONST) {
        /* MEM(CONST(i)) */
        sprintf(str, "%s %d(r0), `d0\n", AS_LOAD, exp->u.CONST);
        emit(AS_Oper(str, L(r, NULL), NULL, NULL));
        return r;
    }

    /* MEM(e) */
    sprintf(str, "%s (`s0), `d0\n", AS_LOAD);
    emit(AS_Oper(str, L(r, NULL),
                L(munchExp(exp), NULL), NULL));
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
            Temp_temp rs = munchExp(e->u.BINOP.right);
            sprintf(str, "addi %d, `d0\n", e->u.BINOP.left->u.CONST);
            emit(AS_Oper(str,
                     L(r, NULL), L(rs, NULL), NULL));
            return r;
        }

        if (e->u.BINOP.right->kind == T_CONST) {
            /* ADDI or SUBI */
            Temp_temp rs = munchExp(e->u.BINOP.left);
            if (e->u.BINOP.op == T_plus) {
                sprintf(str, "leal %d(`s0), `d0\n", e->u.BINOP.right->u.CONST);
            } else if (e->u.BINOP.op == T_minus) {
                sprintf(str, "leal %d(`s0), `d0\n", -e->u.BINOP.right->u.CONST);
            }
            emit(AS_Oper(str, L(r, NULL), L(rs, NULL), NULL));
            return r;
        }
    }


    T_exp left = e->u.BINOP.left;
    if (left->kind == T_MEM)
    {
        printf("=dafas========>>><<\n");
        T_exp m_exp = left->u.MEM;
        if (m_exp->kind == T_BINOP && m_exp->u.BINOP.left->kind == T_CONST
                && m_exp->u.BINOP.right->kind == T_TEMP) {
            sprintf(str, "%s %d(`s0), `d0\n", cmd, m_exp->u.BINOP.left->u.CONST);
            Temp_temp final = munchExp(e->u.BINOP.right);
            emit(AS_Oper(str, L(final, NULL), L(m_exp->u.BINOP.right->u.TEMP, NULL), NULL));
            return final;
        }
    }

        /*
         *      {
         * sprintf(str, "%s `s0, `d0\n", AS_MUL);
         * Temp_temp right = munchExp(e->u.BINOP.right);
         * emit(AS_Oper(str, L(right, NULL), L(munchExp(e->u.BINOP.left), L(right, NULL)), NULL));
         * return F_RV();
         */
    /* } */

    /*
     * the normal things
     * ADD, SUB, MUL, DIV
     */
    Temp_temp left_r = munchExp(e->u.BINOP.left);
    if (e->u.BINOP.right->kind == T_CALL) {
        emit(AS_Oper("pushl `s0\n", NULL, Temp_TempList(left_r, NULL), NULL));
    }
    Temp_temp right_r = munchExp(e->u.BINOP.right);
    sprintf(str, "%s `s1, `d0\n", cmd);
    emit(AS_Oper(str,
                L(right_r, NULL), L(right_r, L(left_r, NULL)), NULL));
    return right_r;
}


static void munchMoveStm(T_stm stm) {
    assert(stm->kind == T_MOVE);

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
                if (src->kind == T_CONST) {
                    sprintf(instr, "%s $%d, %d(`s0)\n", AS_STORE, src->u.CONST, mem_exp->u.BINOP.left->u.CONST);
                    emit(AS_Oper(instr, NULL, L(munchExp(mem_exp->u.BINOP.right), NULL), NULL));
                    return ;
                }
                sprintf(instr, "%s `s1, %d(`s0)\n", AS_STORE, mem_exp->u.BINOP.left->u.CONST);
                emit(AS_Oper(instr, NULL,
                            L(munchExp(mem_exp->u.BINOP.right),
                                L(munchExp(src), NULL)), NULL));
                return;
            }

            if (mem_exp->u.BINOP.right->kind == T_CONST) {
                /*
                 * STORE M[e1+CONST(i)] <- e2
                 */
                if (src->kind == T_CONST) {
                    sprintf(instr, "%s $%d, %d(`s0)\n", AS_STORE, src->u.CONST, mem_exp->u.BINOP.right->u.CONST);
                    emit(AS_Oper(instr, NULL, L(munchExp(mem_exp->u.BINOP.left), NULL), NULL));
                    return ;
                }
                sprintf(instr, "%s `s1, %d(`s0)\n",AS_STORE, mem_exp->u.BINOP.right->u.CONST);
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
            Temp_temp r = Temp_newtemp();
            sprintf(instr, "%s 0(`s1), `d0\n%s `s2, (`s0)\n", AS_MOVE, AS_MOVE);
            emit(AS_Oper(instr, L(r, NULL),
                        L(munchExp(mem_exp),
                            L(munchExp(src->u.MEM), L(r, NULL))), NULL));
            return;
        }

        if (mem_exp->kind == T_CONST) {
            /* MOVE(MEM(CONST(i)), e) */
            /*! TODO: fix
             */
            sprintf(instr, "%s `s0, %d\n", AS_STORE, mem_exp->u.CONST);
            emit(AS_Oper(instr, NULL,
                        L(munchExp(src), NULL), NULL));
            return;
        }

        /* MOVE(MEM(a), e) */
        sprintf(instr, "%s `s1, (`s0)\n", AS_STORE);
        emit(AS_Oper(instr, NULL, L(munchExp(mem_exp),
                        L(munchExp(src), NULL)), NULL));
        return;
    }

    if (dst->kind == T_TEMP) {
        /* move e -> temp */
        if (src->kind == T_BINOP && src->u.BINOP.right->kind == T_CONST && src->u.BINOP.op == T_plus) {
            sprintf(instr, "leal %d(`s0), `d0\n", src->u.BINOP.right->u.CONST);
            emit(AS_Oper(instr, L(dst->u.TEMP, NULL), L(munchExp(src->u.BINOP.left), NULL), NULL));
            return;
        }
        sprintf(instr, "%s `s0, `d0\n", AS_MOVE);
        emit(AS_Oper(instr, Temp_TempList(dst->u.TEMP, NULL),
                    Temp_TempList(munchExp(src), L(dst->u.TEMP, NULL)), NULL));
        return;
    }

    assert(0);
}

static Temp_tempList munchArgs(int n, T_expList args) {
    if (args == NULL) {
        return NULL;
    }

    Temp_tempList list = munchArgs(n+1, args->tail);
    Temp_temp r;
    char buf[200];
    if (args->head->kind == T_NAME) {
        r = Temp_newtemp();
        sprintf(buf,"leal %s, `d0\n", S_name(args->head->u.NAME));
        emit(AS_Oper(buf, L(r, NULL), NULL, NULL));
    } else {
        r = munchExp(args->head);
    }
    sprintf(buf, "pushl `s0\n");
    emit(AS_Oper(buf, NULL, L(r, NULL), NULL));
    return Temp_TempList(r, list);
}
