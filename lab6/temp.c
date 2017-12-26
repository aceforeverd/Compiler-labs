/*
 * temp.c - functions to create and manipulate temporary variables which are
 *          used in the IR tree representation before it has been determined
 *          which variables are to go into registers.
 *
 */

#include "symbol.h"
#include "table.h"
#include "temp.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Temp_temp_ {
    int num;
};

string Temp_labelstring(Temp_label s) { return S_name(s); }

static int labels = 0;

Temp_label Temp_newlabel(void) {
    char buf[100];
    sprintf(buf, "L%d", labels++);
    return Temp_namedlabel(String(buf));
}

/* The label will be created only if it is not found.
 * else it will just return the original one */
Temp_label Temp_namedlabel(string s) { return S_Symbol(s); }

/* tmps < 100 for reserved registers, or to say, explicit registers */
static int temps = 100;

Temp_temp Temp_newtemp(void) {
    Temp_temp temp = (Temp_temp)checked_malloc(sizeof(*temp));
    temp->num = temps++;
    {
        char name[16];
        sprintf(name, "%d", temp->num);
        Temp_enter(Temp_name(), temp, String(name));
    }
    return temp;
}

Temp_temp Temp_explicitTemp(int num) {
    assert(num < 100);
    Temp_temp temp = (Temp_temp) checked_malloc(sizeof(*temp));

    temp->num = num;

    return temp;
}

Temp_tempList Temp_TempList(Temp_temp h, Temp_tempList t) {
    Temp_tempList p = (Temp_tempList)checked_malloc(sizeof(*p));
    p->head = h;
    p->tail = t;
    return p;
}

Temp_tempList Temp_ListSplice(Temp_tempList left, Temp_tempList right) {
    if (left == NULL) {
        return right;
    }

    Temp_tempList ls = left;
    while (ls->tail) {
        ls = ls->tail;
    }

    ls->tail = right;
    return left;
}

bool Temp_equal(Temp_temp a, Temp_temp b) {
    assert(a && b);
    return a->num == b->num;
}

/* a >= b ? */
bool Temp_compare(Temp_temp a, Temp_temp b) {
    return a->num >= b->num;
}

bool Temp_ListEqual(Temp_tempList a, Temp_tempList b) {
    if (!a && !b) return TRUE;
    if (!a || !b) return FALSE;

    int l1 = Temp_ListLength(a);
    int l2 = Temp_ListLength(b);
    if (l1 != l2) return FALSE;

    bool equal = TRUE;
    while (b) {
        if (!Temp_ListInclude(a, b->head)) {
            equal = FALSE;
        }
        b = b->tail;
    }

    return equal;
}

bool Temp_ListInclude(Temp_tempList body, Temp_temp item) {
    if (!item || !body) {
        return FALSE;
    }

    while (body) {
        if (Temp_equal(body->head, item)) {
            return TRUE;
        }
        body = body->tail;
    }

    return FALSE;
}

void Temp_TempSwap(Temp_temp a, Temp_temp b) {
    Temp_temp tmp = a;
    a = b;
    b = tmp;
}

/* high to low */
Temp_tempList Temp_ListSort(Temp_tempList list) {
    Temp_tempList link = list;
    Temp_tempList nlink = list;
    while (link) {
        nlink = link;

        while (nlink && nlink->tail) {
            if (!Temp_compare(nlink->head, nlink->tail->head)) {
                Temp_TempSwap(nlink->head, nlink->tail->head);
            }
            nlink = nlink->tail;
        }

        link = link->tail;
    }

    return list;
}

Temp_tempList Temp_ListInsert(Temp_tempList list, Temp_temp item) {
    Temp_tempList current_list = list;
    Temp_tempList last_list = NULL;
    while (current_list) {
        if (Temp_compare(item, current_list->head)) {
            current_list = Temp_TempList(item, current_list);
            if (!last_list) {
                return current_list;
            } else {
                last_list->tail = current_list;
                return list;
            }
        }
        last_list = current_list;
        current_list = current_list->tail;
    }

    last_list->tail = Temp_TempList(item, NULL);
    return list;
}

int Temp_ListLength(Temp_tempList list) {
    int length = 0;
    while (list && list->head) {
        length ++;
        list = list->tail;
    }
    return length;
}
/*
 * templsit += temp
 * put item at top of the list
 */
Temp_tempList Temp_ListAppend(Temp_tempList body, Temp_temp item) {
    if (Temp_ListInclude(body, item)) {
        return body;
    }

    return Temp_TempList(item, body);
}

/*
 * templsit -= temp
 */
Temp_tempList Temp_ListExcludeTemp(Temp_tempList body, Temp_temp item) {
    if (!item || !body) {
        return body;
    }

    if (Temp_equal(body->head, item)) {
        return body->tail;
    }

    return Temp_TempList(body->head, Temp_ListExcludeTemp(body->tail, item));
}

/*
 * templsit -= temp
 */
Temp_tempList Temp_ListExcludeTemp2(Temp_tempList body, Temp_temp item) {
    assert(body);
    if (!item) {
        return body;
    }

    Temp_tempList list = body;
    Temp_tempList pre = NULL;
    while (list) {
        if (Temp_equal(list->head, item)) {
            if (!pre) {
                return list->tail;
            } else {
                pre->tail = list->tail;
                return body;
            }
        }

        pre = list;
        list = list->tail;
    }

    return body;
}
/*
 * exclude tempList right from left
 * templist -= templist2
 */
Temp_tempList Temp_ListExclude(Temp_tempList left, Temp_tempList right) {
    while (right) {
        left = Temp_ListExcludeTemp(left, right->head);
        right = right->tail;
    }

    return left;
}

/*
 * union TempList right with left
 * templist = templist1 + templist2
 */
Temp_tempList Temp_ListUnion(Temp_tempList left, Temp_tempList right) {
    if (!left) {
        return right;
    }
    while (right) {
        left = Temp_ListAppend(left, right->head);
        right = right->tail;
    }
    return left;
}

struct Temp_map_ {
    TAB_table tab;
    Temp_map under;
};

/* return the global resgister(temp) map */
Temp_map Temp_name(void) {
    // a global register map
    static Temp_map m = NULL;
    if (!m) {
        m = Temp_empty();
    }
    return m;
}

int Temp_num(Temp_temp temp) {
    return temp->num;
}

Temp_map newMap(TAB_table tab, Temp_map under) {
    Temp_map m = checked_malloc(sizeof(*m));
    m->tab = tab;
    m->under = under;
    return m;
}

Temp_map Temp_empty(void) {
    return newMap(TAB_empty(), NULL);
}

Temp_map Temp_layerMap(Temp_map over, Temp_map under) {
    if (over == NULL)
        return under;
    else
        return newMap(over->tab, Temp_layerMap(over->under, under));
}

void Temp_enter(Temp_map m, Temp_temp t, string s) {
    assert(m && m->tab);
    TAB_enter(m->tab, t, s);
}

string Temp_look(Temp_map m, Temp_temp t) {
    string s;
    assert(m && m->tab);
    s = TAB_look(m->tab, t);
    if (s)
        return s;
    else if (m->under)
        return Temp_look(m->under, t);
    else
        return NULL;
}

Temp_labelList Temp_LabelList(Temp_label h, Temp_labelList t) {
    Temp_labelList p = (Temp_labelList)checked_malloc(sizeof(*p));
    p->head = h;
    p->tail = t;
    return p;
}

static FILE *outfile;
// show the mapping, id -> variable name
void showit(Temp_temp t, string r) {
    fprintf(outfile, "t%d -> %s\n", t->num, r);
}

void Temp_dumpMap(FILE *out, Temp_map m) {
    outfile = out;
    TAB_dump(m->tab, (void (*)(void *, void *))showit);
    if (m->under) {
        fprintf(out, "---------\n");
        Temp_dumpMap(out, m->under);
    }
}
