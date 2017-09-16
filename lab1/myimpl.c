#include "prog1.h"
#include <stdio.h>

typedef struct table *Table_;
typedef struct IntAndTable *IntAndTable_;
struct table {
    string id;
    int value;
    Table_ tail;
};

struct IntAndTable {
    int i;
    Table_ t;
};

Table_ Table(string id, int value, struct table *tail) {
    Table_ t = checked_malloc(sizeof(*t));
    t->id = id;
    t->value = value;
    t->tail = tail;
    return t;
}

IntAndTable_ IntAndTable(int value, Table_ table) {
    IntAndTable_ int_table = checked_malloc(sizeof(*int_table));
    int_table->t = table;
    int_table->i = value;
    return int_table;
}

int maxargs(A_stm stm);
void interp(A_stm stm);

int print_args(A_expList print);
int exp_args(A_exp exp);

Table_ interp_stm(A_stm s, Table_ t);
Table_ interp_assign_stm(A_stm s, Table_ t);

Table_ update_table(Table_ t, string key, int value);
int lookup_table(Table_ t, string key);

IntAndTable_ interp_exp(A_exp e, Table_ t);
IntAndTable_ interp_op_exp(A_exp e, Table_ t);
Table_ interp_exp_list(A_expList exps, Table_ t);

int maxargs(A_stm stm) {
    switch (stm->kind) {
        case A_compoundStm :
            return maxargs(stm->u.compound.stm1) > maxargs(stm->u.compound.stm2) ?
                maxargs(stm->u.compound.stm1) : maxargs(stm->u.compound.stm2);
        case A_assignStm :
            return exp_args(stm->u.assign.exp);
        case A_printStm :
            return print_args(stm->u.print.exps);
    }

}

void interp(A_stm stm) {
    Table_ table = NULL;
    interp_stm(stm, table);
}

int print_args(A_expList print) {
    if (print->kind == A_lastExpList) {
        return 1;
    }

    return 1 + print_args(print->u.pair.tail);
}

int exp_args(A_exp exp) {
    if (exp->kind == A_eseqExp) {
        return maxargs(exp->u.eseq.stm);
    }

    return 0;
}

Table_ interp_stm(A_stm s, Table_ t) {
    switch (s->kind) {
        case A_compoundStm :
            return interp_stm(s->u.compound.stm2,
                    interp_stm(s->u.compound.stm1, t));
        case A_printStm:
            return interp_exp_list(s->u.print.exps, t);
        case A_assignStm:
            return interp_assign_stm(s, t);
    }
}

IntAndTable_ interp_exp(A_exp e, Table_ t) {
    switch (e->kind) {
        case A_idExp :
            return IntAndTable(lookup_table(t, e->u.id), t);
        case A_numExp :
            return IntAndTable(e->u.num, t);
        case A_opExp :
            return interp_op_exp(e, t);
        case A_eseqExp :
            return interp_exp(e->u.eseq.exp,
                    interp_stm(e->u.eseq.stm, t));

    }
}

IntAndTable_ interp_op_exp(A_exp e, Table_ t) {
    IntAndTable_ left = interp_exp(e->u.op.left, t);
    IntAndTable_ right = interp_exp(e->u.op.right, left->t);
    switch (e->u.op.oper) {
        case A_plus :
            return IntAndTable(left->i + right->i, right->t);
        case A_minus :
            return IntAndTable(left->i - right->i, right->t);
        case A_times :
            return IntAndTable(left->i * right->i, right->t);
        case A_div :
            return IntAndTable(left->i / right->i, right->t);
    }
}

Table_ interp_assign_stm(A_stm s, Table_ t) {
    IntAndTable_ int_table = interp_exp(s->u.assign.exp, t);
    return Table(s->u.assign.id, int_table->i, int_table->t);
}

Table_ update_table(Table_ t, string key, int value) {
    return Table(key, value, t);
}


int lookup_table(Table_ t, string key) {
    Table_ tail = t;
    while (tail) {
        if (tail->id == key) {
            return tail->value;
        }

        tail = tail->tail;
    }

    return 0;
}


Table_ interp_exp_list(A_expList exps, Table_ t) {
    IntAndTable_ int_table;

    switch (exps->kind) {
        case A_pairExpList :
            int_table = interp_exp(exps->u.pair.head, t);
            printf("%d ", int_table->i);
            return interp_exp_list(exps->u.pair.tail,
                    int_table->t);

        case A_lastExpList :
            int_table = interp_exp(exps->u.last, t);
            printf("%d\n", int_table->i);
            return int_table->t;
    }
}
