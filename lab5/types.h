#ifndef TYPES_H
#define TYPES_H

#include "absyn.h"
#include "symbol.h"

/*
 * types.h -
 *
 * All types and functions declared in this header file begin with "Ty_"
 * Linked list types end with "..list"
 */

typedef struct Ty_ty_ *Ty_ty;
typedef struct Ty_tyList_ *Ty_tyList;
typedef struct Ty_field_ *Ty_field;
typedef struct Ty_fieldList_ *Ty_fieldList;

typedef enum {Ty_record, Ty_nil, Ty_int, Ty_string, Ty_array,
    Ty_name, Ty_void} ty_kind;

struct Ty_ty_
{
    ty_kind kind;

    union {
        Ty_fieldList record;
        Ty_ty array;
        struct {S_symbol sym; Ty_ty ty;} name;
    } u;
};

struct Ty_tyList_ {Ty_ty head; Ty_tyList tail;};
struct Ty_field_ {
    S_symbol name;
    Ty_ty ty;
};
struct Ty_fieldList_ {
    Ty_field head;
    Ty_fieldList tail;
};

Ty_ty Ty_Nil(void);
Ty_ty Ty_Int(void);
Ty_ty Ty_String(void);
Ty_ty Ty_Void(void);

Ty_ty Ty_Record(Ty_fieldList fields);
Ty_ty Ty_Array(Ty_ty ty);
Ty_ty Ty_Name(S_symbol sym, Ty_ty ty);

Ty_ty actual_ty(Ty_ty ty);

Ty_tyList Ty_TyList(Ty_ty head, Ty_tyList tail);
Ty_field Ty_Field(S_symbol name, Ty_ty ty);
Ty_fieldList Ty_FieldList(Ty_field head, Ty_fieldList tail);

void Ty_print(Ty_ty t);
void TyList_print(Ty_tyList list);

Ty_field FieldList_lookup(Ty_fieldList list, S_symbol sym);
void Exp_print(A_exp exp);


char * Ty_Kind_print(ty_kind kind);

#endif
