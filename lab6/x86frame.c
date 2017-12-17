#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "frame.h"
#include "symbol.h"
#include "table.h"
#include "temp.h"
#include "tree.h"
#include "util.h"

#define WORDSIZE 4
const int F_wordSize = 4;
// frame pointer
const int REG_FP = 32;
const int REG_SP= 19;
const int REG_CALLER_SAVE = 21;
const int REG_RET_ADDR = 22;
const int REG_RET_VAL = 23;
const int REG_MUL_HIGH = 24;
const int REG_MUL_LOW = 25;
const int REG_ZERO = 26;

// varibales
struct F_access_ {
    enum { inFrame, inReg } kind;
    union {
        int offset;     // inFrame
        Temp_temp reg;  // inReg
    } u;
};

struct F_frame_ {
    unsigned int n_locals;
    unsigned int n_inFrame;
    /* name of the frame */
    Temp_label name;
    // location of frame pointer in frame stack

    /*
     * the frame pointer
     * should be stored in a special register
     * usually ebp
     *
     * T_Temp()
     */
    T_exp fp;

    U_boolList formals;
    F_accessList formals_list;
};

static struct F_frame_ root_frame = {0, 0, NULL, NULL, NULL, NULL};

static F_accessList F_AccessList(F_access head, F_accessList tail);
static F_accessList F_gen_formals(F_frame frame, U_boolList boolList);
static F_access InFrame(int offset);
static F_access InReg(Temp_temp reg);

/*
 * make a label for a string
 * the string length followed by the string
 */
F_frag F_StringFrag(Temp_label label, string str) {
    F_frag f = checked_malloc(sizeof(*f));

    f->kind = F_stringFrag;
    f->u.stringg.str = str;
    f->u.stringg.label = label;

    /*! TODO: write to memory
     */

    return f;
}

F_frag F_ProcFrag(T_stm body, F_frame frame) {
    F_frag f = checked_malloc(sizeof(*f));

    f->kind = F_procFrag;
    f->u.proc.body = body;
    f->u.proc.frame = frame;

    return f;
}

F_fragList F_FragList(F_frag head, F_fragList tail) {
    F_fragList a_fragList = checked_malloc(sizeof(*a_fragList));

    a_fragList->head = head;
    a_fragList->tail = tail;

    return a_fragList;
}


/*
 * append item to the end of the list
 */
void F_FragListAppend(F_fragList list, F_frag item) {
    for (; list->tail; list = list->tail) {}

    list->tail = F_FragList(item, NULL);
}

Temp_temp F_FP() {
    return Temp_explicitTemp(REG_FP);
}

/*
 * return frame pointer of f
 */
T_exp F_framePtr(F_frame f) {
    assert(f);
    return f->fp;
}

/*
 * return previous frame pointer
 */
T_exp F_preFramPtr(F_frame f) {
    return T_Mem(F_framePtr(f));
}

/*
 * return a T_exp for the F_access
 */
T_exp F_Exp(F_access acc, T_exp framePtr) {
    switch (acc->kind) {
        case inFrame:
            return T_Mem(T_Binop(T_plus, framePtr, T_Const(acc->u.offset)));
        case inReg:
            return T_Temp(acc->u.reg);
    }
}

Temp_temp F_RV() {
    return Temp_explicitTemp(REG_RET_VAL);
}

F_frame F_newFrame(Temp_label name, U_boolList formals) {
    F_frame frame = checked_malloc(sizeof(*frame));

    frame->name = name;
    frame->formals = formals;
    frame->fp = T_Temp(F_FP());
    frame->n_locals = 0;
    frame->n_inFrame = 0;

    return frame;
}

Temp_label F_name(F_frame f) {
    return f->name;
}

F_accessList F_formals(F_frame f) {
    return F_gen_formals(f, f->formals);
}

static F_accessList F_gen_formals(F_frame frame, U_boolList boolList) {
    if (!boolList || !boolList->head) {
        return NULL;
    }

    return F_AccessList(F_allocLocal(frame, boolList->head),
                        F_gen_formals(frame, boolList->tail));
}

F_access F_allocLocal(F_frame f, bool escape) {
    if (!escape) {
        Temp_temp reg = Temp_newtemp();
        f->n_locals ++;
        return InReg(reg);
    } else {
        f->n_locals ++;
        f->n_inFrame ++;
        return InFrame(WORDSIZE * (1 - f->n_inFrame));
    }
}

static F_accessList F_AccessList(F_access head, F_accessList tail) {
    F_accessList list = checked_malloc(sizeof(*list));

    list->head = head;
    list->tail = tail;

    return list;
}

static F_access InFrame(int offset) {
    F_access access = checked_malloc(sizeof(*access));

    access->kind = inFrame;
    access->u.offset = offset;

    return access;
}

static F_access InReg(Temp_temp reg) {
    F_access access = checked_malloc(sizeof(*access));

    access->kind = inReg;
    access->u.reg = reg;

    return access;
}

T_exp F_externalCall(string s, T_expList args) {
    return T_Call(T_Name(Temp_namedlabel(s)), args);
}

T_stm F_procEntryExit1(F_frame frame, T_stm stm) {
    return stm;
}

static Temp_tempList returnSink = NULL;
AS_instrList F_procEntryExit2(AS_instrList body) {
    if (!returnSink) {
        returnSink = Temp_TempList(Temp_explicitTemp(REG_ZERO),
                Temp_TempList(Temp_explicitTemp(REG_SP), CalleeSaves()));
    }

    return AS_splice(body, AS_InstrList(AS_Oper(
                    "", NULL, returnSink, NULL), NULL));
}

AS_proc F_procEntryExit3(F_frame frame, AS_instrList body) {
    char buf[100];
    sprintf(buf, "PROCEDURE %s\n", S_name(frame->name));
    return AS_Proc(String(buf), body, "END\n");
}


F_frame outermost_frame() {
    return &root_frame;
}

Temp_tempList CalleeSaves() {
    return Temp_TempList(Temp_explicitTemp(REG_RET_VAL),
            Temp_TempList(Temp_explicitTemp(REG_RET_ADDR), NULL));
}
/* return a list of temp that reserved by call function */
Temp_tempList CallDefs() {
    return Temp_TempList(Temp_explicitTemp(REG_CALLER_SAVE),
            CalleeSaves());
}
/* return a list of temps that reserved by MUL AND DIV */
Temp_tempList MulDefs() {
    return Temp_TempList(Temp_explicitTemp(REG_MUL_HIGH),
            Temp_TempList(Temp_explicitTemp(REG_MUL_LOW), NULL));
}
