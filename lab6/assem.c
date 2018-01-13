/*
 * mipscodegen.c - Functions to translate to Assem-instructions for
 *             the Jouette assembly language using Maximal Munch.
 */

#include "absyn.h"
#include "assem.h"
#include "errormsg.h"
#include "tree.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h> /* for atoi */
#include <string.h> /* for strcpy */



AS_targets AS_Targets(Temp_labelList labels) {
    AS_targets p = checked_malloc(sizeof *p);
    p->labels = labels;
    return p;
}

AS_instr AS_Oper(string a, Temp_tempList d, Temp_tempList s, AS_targets j) {
    AS_instr p = (AS_instr)checked_malloc(sizeof *p);
    p->kind = I_OPER;
    p->u.OPER.assem = String(a);
    p->u.OPER.dst = d;
    p->u.OPER.src = s;
    p->u.OPER.jumps = j;
    return p;
}

AS_instr AS_Label(string a, Temp_label label) {
    AS_instr p = (AS_instr)checked_malloc(sizeof *p);
    p->kind = I_LABEL;
    p->u.LABEL.assem = String(a);
    p->u.LABEL.label = label;
    return p;
}

AS_instr AS_Move(string a, Temp_tempList d, Temp_tempList s) {
    AS_instr p = (AS_instr)checked_malloc(sizeof *p);
    p->kind = I_MOVE;
    p->u.MOVE.assem = String(a);
    p->u.MOVE.dst = d;
    p->u.MOVE.src = s;
    return p;
}

AS_instrList AS_InstrList(AS_instr head, AS_instrList tail) {
    AS_instrList p = (AS_instrList)checked_malloc(sizeof *p);
    p->head = head;
    p->tail = tail;
    return p;
}

/* put list b at the end of list a */
AS_instrList AS_splice(AS_instrList a, AS_instrList b) {
    AS_instrList p;
    if (a == NULL) return b;
    for (p = a; p->tail != NULL; p = p->tail)
        ;
    p->tail = b;
    return a;
}

static Temp_temp nthTemp(Temp_tempList list, int i) {
    assert(list);
    if (i == 0)
        return list->head;
    else
        return nthTemp(list->tail, i - 1);
}

static Temp_label nthLabel(Temp_labelList list, int i) {
    assert(list);
    if (i == 0)
        return list->head;
    else
        return nthLabel(list->tail, i - 1);
}

/* first param is string created by this function by reading 'assem' string
 * and replacing `d `s and `j stuff.
 * Last param is function to use to determine what to do with each temp.
 */
static void format(char *result, string assem, Temp_tempList dst,
                   Temp_tempList src, AS_targets jumps, Temp_map m) {
    // fprintf(stdout, "a format: assem=%s, dst=%p, src=%p\n", assem, dst, src);
    char *p;
    int i = 0; /* offset to result string */
    for (p = assem; p && *p != '\0'; p++) {
        if (*p == '`') {
            switch (*(++p)) {
                case 's': {
                    int n = atoi(++p);
                    string s = Temp_look(m, nthTemp(src, n));
                    if (!s) {
                        fprintf(stderr, "entry not found for temp %d\n", Temp_num(nthTemp(src, n)));
                        assert(0);
                    }
                    strcpy(result + i, s);
                    i += strlen(s);
                } break;
                case 'd': {
                    int n = atoi(++p);
                    string s = Temp_look(m, nthTemp(dst, n));
                    if (!s) {
                        fprintf(stderr, "entry not found for temp %d\n", Temp_num(nthTemp(dst, n)));
                        printf("instr: %s\n", assem);
                        s = String("%ebx");
                    }
                    strcpy(result + i, s);
                    i += strlen(s);
                } break;
                case 'j':
                    assert(jumps);
                    {
                        int n = atoi(++p);
                        string s = Temp_labelstring(nthLabel(jumps->labels, n));
                        strcpy(result + i, s);
                        i += strlen(s);
                    }
                    break;
                case '`':
                    result[i] = '`';
                    i++;
                    break;
                default:
                    fprintf(stderr, "UNKNOW character: %c\n", *(p));
                    assert(0);
            }
        } else {
            result[i] = *p;
            i++;
        }
    }
    result[i] = '\0';
}

void AS_print(FILE *out, AS_instr i, Temp_map m) {
    assert(i);
    char r[200]; /* result */
    switch (i->kind) {
        case I_OPER:
            format(r, i->u.OPER.assem, i->u.OPER.dst, i->u.OPER.src,
                   i->u.OPER.jumps, m);
            fprintf(out, "%s\n", r);
            break;
        case I_LABEL:
            format(r, i->u.LABEL.assem, NULL, NULL, NULL, m);
            fprintf(out, "%s:\n", r);
            /* i->u.LABEL->label); */
            break;
        case I_MOVE: {
            if ((i->u.MOVE.dst == NULL) && (i->u.MOVE.src == NULL)) {
                char *src = strchr(i->u.MOVE.assem, '%');
                if (src != NULL) {
                    char *dst = strchr(src + 1, '%');
                    if (dst != NULL) {
                        // fprintf(out, "src: %s; dst: %s\n", src, dst);
                        if ((src[1] == dst[1]) && (src[2] == dst[2]) &&
                            (src[3] == dst[3]))
                            break;
                    }
                }
            }
            format(r, i->u.MOVE.assem, i->u.MOVE.dst, i->u.MOVE.src, NULL, m);
            fprintf(out, "%s\n", r);
            break;
        }
    }
}

/* c should be COL_color; temporarily it is not */
void AS_printInstrList(FILE *out, AS_instrList iList, Temp_map m) {
    for (; iList; iList = iList->tail) {
        AS_print(out, iList->head, m);
    }
    fprintf(out, "\n");
}

AS_proc AS_Proc(string p, AS_instrList b, string e) {
    AS_proc proc = checked_malloc(sizeof(*proc));
    proc->prolog = p;
    proc->body = b;
    proc->epilog = e;
    return proc;
}

void AS_rewrite(AS_instrList iList, Temp_map m) {
    while (iList) {
        iList->head = AS_rewrite_one(iList->head, m);

        iList = iList->tail;
    }
}

AS_instr AS_rewrite_one(AS_instr instr, Temp_map m) {
    assert(instr);
    char str[200];
    switch(instr->kind) {
        case I_OPER:
        {
            format(str, instr->u.OPER.assem, instr->u.OPER.dst, instr->u.OPER.src,
                    instr->u.OPER.jumps, m);
            instr->u.OPER.assem = String(str);
            break;
        }
        case I_LABEL:
        {
            format(str, instr->u.LABEL.assem, NULL, NULL, NULL, m);
            instr->u.LABEL.assem = String(str);
            break;
        }
        case I_MOVE:
        {
            if ((instr->u.MOVE.dst == NULL) && (instr->u.MOVE.src == NULL)) {
                char *src = strchr(instr->u.MOVE.assem, '%');
                if (src != NULL) {
                    char *dst = strchr(src + 1, '%');
                    if (dst != NULL) {
                        // fprintf(out, "src: %s; dst: %s\n", src, dst);
                        if ((src[1] == dst[1]) && (src[2] == dst[2]) &&
                            (src[3] == dst[3]))
                            break;
                    }
                }
            }
            format(str, instr->u.MOVE.assem, instr->u.MOVE.dst, instr->u.MOVE.src, NULL, m);
            instr->u.MOVE.assem = String(str);
            break;
        }
    }

    return instr;
}

