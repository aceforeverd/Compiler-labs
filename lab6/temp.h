/*
 * temp.h
 *
 */

#ifndef TEMP_H
#define TEMP_H

#include "symbol.h"
#include <stdio.h>

// Temps abstract names for local variables
typedef struct Temp_temp_ *Temp_temp;
Temp_temp Temp_newtemp(void);
Temp_temp Temp_explicitTemp(int num);

typedef struct Temp_tempList_ *Temp_tempList;
struct Temp_tempList_ {
    Temp_temp head;
    Temp_tempList tail;
};
Temp_tempList Temp_TempList(Temp_temp h, Temp_tempList t);
Temp_tempList Temp_ListSplice(Temp_tempList left, Temp_tempList right);
void Temp_TempSwap(Temp_temp a, Temp_temp b);
bool Temp_equal(Temp_temp a, Temp_temp b);
bool Temp_compare(Temp_temp a, Temp_temp b);
bool Temp_ListEqual(Temp_tempList a, Temp_tempList b);
bool Temp_ListEqual_next(Temp_tempList a, Temp_tempList b);
int Temp_ListLength(Temp_tempList list);
bool Temp_ListInclude(Temp_tempList body, Temp_temp item);
Temp_tempList Temp_ListSort(Temp_tempList list);
Temp_tempList Temp_ListInsert(Temp_tempList list, Temp_temp item);
Temp_tempList Temp_ListAppend(Temp_tempList body, Temp_temp item);
Temp_tempList Temp_ListExcludeTemp(Temp_tempList body, Temp_temp item);
Temp_tempList Temp_ListExcludeTemp2(Temp_tempList body, Temp_temp item);
Temp_tempList Temp_ListExclude(Temp_tempList left, Temp_tempList right);
Temp_tempList Temp_ListUnion(Temp_tempList left, Temp_tempList right);

// Labels abstract name for static memory address
typedef S_symbol Temp_label;
Temp_label Temp_newlabel(void);
Temp_label Temp_namedlabel(string name);
string Temp_labelstring(Temp_label s);

typedef struct Temp_labelList_ *Temp_labelList;
struct Temp_labelList_ {
    Temp_label head;
    Temp_labelList tail;
};
Temp_labelList Temp_LabelList(Temp_label h, Temp_labelList t);

typedef struct Temp_map_ *Temp_map;
Temp_map Temp_name(void);
Temp_map Temp_empty(void);
Temp_map Temp_layerMap(Temp_map over, Temp_map under);
void Temp_enter(Temp_map m, Temp_temp t, string s);
string Temp_look(Temp_map m, Temp_temp t);
void Temp_dumpMap(FILE *out, Temp_map m);

int Temp_num(Temp_temp temp);
string Temp_toString(Temp_temp temp);

int Temp_index(Temp_tempList list, Temp_temp t);

#endif
