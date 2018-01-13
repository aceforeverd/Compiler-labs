
/*Lab5: This header file is not complete. Please finish it with more
 * definition.*/

#ifndef FRAME_H
#define FRAME_H

#include "tree.h"
#include "assem.h"

Temp_map F_tempMap;

extern const int F_wordSize;
extern const int SL_OFFSET;

typedef struct F_frame_ *F_frame;

typedef struct F_access_ *F_access;
typedef struct F_accessList_ *F_accessList;

struct F_accessList_ {
    F_access head;
    F_accessList tail;
};

/* declaration for fragments */
typedef struct F_frag_ *F_frag;
struct F_frag_ {
    enum { F_stringFrag, F_procFrag } kind;
    union {
        struct {
            Temp_label label;
            string str;
        } stringg;
        struct {
            T_stm body;
            F_frame frame;
        } proc;
    } u;
};

F_frag F_StringFrag(Temp_label label, string str);
F_frag F_ProcFrag(T_stm body, F_frame frame);
F_frag F_string(Temp_label label, string str);
F_frag F_newProgFrag(T_stm body, F_frame frame);


typedef struct F_fragList_ *F_fragList;
struct F_fragList_ {
    F_frag head;
    F_fragList tail;
};

F_fragList F_FragList(F_frag head, F_fragList tail);
F_fragList F_FragListAppend(F_fragList list, F_frag item);

T_exp F_framePtr();
T_exp F_preFrame(T_exp exp);
T_exp F_preFramePtr(F_frame f);
T_exp F_Exp(F_access acc, T_exp framePtr);

F_frame F_newFrame(Temp_label name, U_boolList formals);
Temp_label F_name(F_frame f);
F_accessList F_formals(F_frame f);
F_access F_allocLocal(F_frame f, bool escape);

T_exp F_externalCall(string s, T_expList args);

T_stm F_procEntryExit1(F_frame frame, T_stm stm);

AS_instrList F_procEntryExit2(AS_instrList body);

AS_proc F_procEntryExit3(F_frame frame, AS_instrList body);

F_frame outermost_frame();

Temp_tempList F_registers();
string F_getLabel(F_frame frame);

Temp_temp F_FP();
Temp_temp F_SP();
Temp_temp F_ZERO();
/* Temp_temp F_RA(); */
Temp_temp F_RV();

Temp_tempList CalleeSaves();
/* return a list of temp that reserved by call function */
Temp_tempList CallDefs();
/* return a list of temps that reserved by MUL AND DIV */
Temp_tempList MulDefs();

Temp_map F_Temps();

Temp_temp Temp_regLookup(string name);
Temp_temp Temp_toTemp(string name);

void F_echoFrame(F_frame f);

Temp_map F_newMap();

Temp_tempList F_ava_registers();

string gen_instruction(string i);

T_exp F_ExpAddress(F_access acc, T_exp framePtr);

F_access F_allocParam(F_frame f, int nth);

#endif
