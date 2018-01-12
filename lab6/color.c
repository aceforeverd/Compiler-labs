#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "absyn.h"
#include "assem.h"
#include "color.h"
#include "frame.h"
#include "graph.h"
#include "symbol.h"
#include "table.h"
#include "tree.h"
#include "util.h"

typedef G_nodeList SelectStack;
/* static void COL_Label(T_exp exp); */
/* the simplest coloring */
static void COL_make_worklist(LiveGraph lg, G_nodeList *simplify_list,
                              G_nodeList *freeze_list, G_nodeList *spill_list,
                              int n_regs);
static void COL_build(LiveGraph static_ig, G_nodeList move_list);
static int COL_simplify(LiveGraph ig, G_nodeList *simp, SelectStack *sstack);
static int select_spill(G_graph ig, SelectStack *sstack, int K);
static void assign_color(G_graph graph, SelectStack sstack, int *temps_mapping,
                         Temp_tempList regs);
static int assign_color_to_node(G_node node, int *temps_mapping,
                                Temp_tempList regs);
static int node_color(G_node node, int *temp_mapping);
static int *make_color_list(Temp_tempList list, int n);
static Temp_temp nthTemp(Temp_tempList list, int i);
static void dump_temp_mapping(int *temp_mapping, int length);

/*
 * @parameters:
 * lg: live graph
 * initial: precoloring of temporaries imposed by calling conventions
 * (arguments) regs: a list of colors
 * @return:
 * COL_result.coloring: a Temp_map describe register allocation. [Temp_temp =>
 * String(registers)] COL_result.spills: list of spills
 */
struct COL_result COL_color(LiveGraph lg, Temp_map initial,
                            Temp_tempList regs, Temp_tempList ava_regs) {
    struct COL_result ret;
    ret.coloring = F_newMap();
    ret.spills = NULL;

    int n_regs = Temp_ListLength(regs);
    /* int *int_regs = make_color_list(regs, n_regs); */

    G_nodeList simplify_list ;
    G_nodeList freeze_list ;
    G_nodeList spill_list ;
    COL_make_worklist(lg, &simplify_list, &freeze_list, &spill_list, n_regs);

    G_graph graph = lg->graph;
    G_nodeList node_list = G_nodes(graph);

    int temp_num = G_nodeCount(graph);
    int temps_mapping[temp_num];
    for (int i = 0; i < temp_num; i++) {
        temps_mapping[i] = -1;
    }

    while (node_list) {
        G_node node = node_list->head;
        Temp_temp t = (Temp_temp) G_nodeInfo(node);
        int index = Temp_index(regs, t);
        if (index >= 0) {
            printf("already assigned r%d => %s\n", Temp_num(t), Temp_look(F_Temps(), t));
            temps_mapping[G_nodeKey(node)] = index;
        } else {
            assign_color_to_node(node, temps_mapping, ava_regs);

            int color_index = temps_mapping[G_nodeKey(node)];
            assert(color_index >= 0 && color_index < temp_num);
            string reg_name = Temp_look(F_Temps(), nthTemp(ava_regs, color_index));
            printf("assign register: r%d => %s\n", Temp_num(t), reg_name);
            Temp_enter(ret.coloring, t, String(reg_name));
        }
        node_list = node_list->tail;
    }

    dump_temp_mapping(temps_mapping, temp_num);

    return ret;
}

/*
 * remove a non-move-related node from graph
 * return 1 on success, 0 for not remved
 */
static int COL_simplify(LiveGraph liveGraph, G_nodeList *simp_list,
                        SelectStack *sstack) {
    G_nodeList simplify_list = *simp_list;
    while (simplify_list) {
        if (!Live_move_related(liveGraph->moves, simplify_list->head)) {
            /* push to select stack */
            *sstack = G_NodeList(simplify_list->head, *sstack);
            /* remove node from its graph */
            G_rmNode(simplify_list->head);
            /* remove node from simp_list */
            *simp_list = G_delete(simplify_list->head, *simp_list);

            return 1;
        }
        simplify_list = simplify_list->tail;
    }
    return 0;
}

static void assign_color(G_graph graph, SelectStack sstack, int *temp_mapping,
                         Temp_tempList regs) {
    while (sstack) {
        G_addNode(sstack->head);
        if (assign_color_to_node(sstack->head, temp_mapping, regs) < 0) {
            printf("failed to assign color\n");
            assert(0);
        }

        sstack = sstack->tail;
    }
}

static int assign_color_to_node(G_node node, int *temps_mapping, Temp_tempList regs) {
    int reg_num = Temp_ListLength(regs);
    int reg_list[reg_num];
    for (int i = 0; i< reg_num; i++) {
        reg_list[i] = 0;
    }

    G_nodeList neighbors = G_succ(node);
    while (neighbors) {
        /* printf("%d\t", Temp_num(G_nodeInfo(neighbors->head))); */
        int index = node_color(neighbors->head, temps_mapping);
        assert(index < reg_num);
        if (index >= 0) {
            /* exclude the register */
            reg_list[index] = 1;
        }
        neighbors = neighbors->tail;
    }
    printf("\n");

    for (int i = 0 ; i < reg_num; i++) {
        if (reg_list[i] == 0 ) {
            /* use this color */
            temps_mapping[G_nodeKey(node)] = i;
            return 0;
        }
    }

    /* assign color failed */
    return -1;
}

static int node_color(G_node node, int *temp_mapping) {
    assert(temp_mapping);
    assert(node);
    return temp_mapping[G_nodeKey(node)];
}

/* select one node and spill */
static int select_spill(G_graph ig, SelectStack *sstack, int K) {
    G_node node = NULL;
    G_nodeList list = G_nodes(ig);
    while (list) {
        if (G_xDegree(list->head) > K) {
            node = list->head;
            break;
        }
        list = list->tail;
    }

    if (node) {
        /* remove node */
        G_rmNode(node);
        *sstack = G_NodeList(node, *sstack);
        return 1;
    }
    return 0;
}


static void COL_make_worklist(LiveGraph lg, G_nodeList *simplify_list,
                              G_nodeList *freeze_list, G_nodeList *spill_list, int n_regs) {
    *simplify_list = NULL;
    *freeze_list = NULL;
    *spill_list = NULL;

    G_nodeList node_list = G_nodes(lg->graph);

    while (node_list) {
        int degree = G_xDegree(node_list->head);
        if (degree >= n_regs) {
            *spill_list = G_NodeList(node_list->head, *spill_list);
            /* G_rmNode(node_list->head); */
        } else if (Live_move_related(lg->moves, node_list->head)) {
            *freeze_list = G_NodeList(node_list->head, *freeze_list);
            /* G_rmNode(node_list->head); */
        } else {
            *simplify_list = G_NodeList(node_list->head, *simplify_list);
            /* G_rmNode(node_list->head); */
        }
        node_list = node_list->tail;
    }
}

/* make a array of int from a Temp_tempList */
static int *make_color_list(Temp_tempList list, int n) {
    int *ret = (int *) checked_malloc(n * sizeof(int));
    int i = 0;
    while (list && i < n) {
        ret[i] = Temp_num(list->head);

        i ++;
        list = list->tail;
    }

    return ret;
}

static Temp_temp nthTemp(Temp_tempList list, int i) {
    assert(list);
    if (i == 0) {
        return list->head;
    }
    return nthTemp(list->tail, i - 1);
}

static void dump_temp_mapping(int *temp_mapping, int length) {
    for (int i = 0; i < length; i++) {
        printf("%d => %d, ", i, temp_mapping[i]);
    }
    printf("\n");
}
