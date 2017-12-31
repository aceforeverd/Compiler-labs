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
static void SelectSpill(G_graph ig, SelectStack *sstack, int K);
static void assign_color(G_graph graph, SelectStack sstack, Temp_map coloring,
                         Temp_tempList regs);
static void simple_assign_color(G_graph graph, Temp_map coloring, Temp_tempList reg);
static int assign_color_to_node(G_node node, Temp_map coloring, Temp_tempList regs);
static string node_color(G_node node, Temp_map coloring);
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
                            Temp_tempList regs) {
    struct COL_result ret;
    ret.coloring = Temp_empty();
    ret.spills = NULL;

    int n_regs = Temp_ListLength(regs);

    G_nodeList simplify_list ;
    G_nodeList freeze_list ;
    G_nodeList spill_list ;
    COL_make_worklist(lg, &simplify_list, &freeze_list, &spill_list, n_regs);

    if (G_NodeListLength(G_nodes(lg->graph)) <= 5) {
        printf("simple coloring \n");
        simple_assign_color(lg->graph, ret.coloring, regs);
    }

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

static void assign_color(G_graph graph, SelectStack sstack, Temp_map coloring,
                         Temp_tempList regs) {
    Temp_map neocoloring = Temp_empty();
    simple_assign_color(graph, neocoloring, regs);

    while (sstack) {
        G_addNode(sstack->head);
        assign_color_to_node(sstack->head, neocoloring, regs);

        sstack = sstack->tail;
    }

    Temp_map temp_map = F_Temps();
    G_nodeList nodes = G_nodes(graph);
    while (nodes) {
        G_node node = nodes->head;
        string color = node_color(node, neocoloring);
        string real_color = Temp_look(temp_map, Temp_toTemp(color));
        Temp_enter(coloring, G_nodeInfo(node), real_color);
    }
}

/* simple assign color, for node less or equal than 5 */
static void simple_assign_color(G_graph graph, Temp_map coloring, Temp_tempList reg) {
    G_nodeList node_list = G_nodes(graph);
    int node_num = G_graphNodes(graph);
    printf("node num %d\n", node_num);

    G_node node;
    int i = 0;
    while (node_list && reg) {
        node = node_list->head;
        string registe = Temp_look(F_Temps(), reg->head);
        printf("reg is %d\t", Temp_num(reg->head));
        Temp_enter(coloring, (Temp_temp) G_nodeInfo(node), registe);
        printf("entered %s\n", registe);

        i ++;
        reg = reg->tail;
        node_list = node_list->tail;
    }
}

static int assign_color_to_node(G_node node, Temp_map coloring, Temp_tempList regs) {
    Temp_tempList available_regs = regs;
    G_nodeList neighbors = G_succ(node);
    while (neighbors) {
        string str = node_color(neighbors->head, coloring);
        if (str) {
            Temp_temp temp = Temp_explicitTemp(atoi(str));
            available_regs = Temp_ListExcludeTemp(available_regs, temp);
        }
        neighbors = neighbors->tail;
    }

    if (available_regs) {
        string color = Temp_toString(available_regs->head);
        Temp_enter(coloring, (Temp_temp) G_nodeInfo(node), color);
        return 0;
    }

    return -1;
}

static string node_color(G_node node, Temp_map coloring) {
    return Temp_look(coloring, (Temp_temp) G_nodeInfo(node));
}

static void SelectSpill(G_graph ig, SelectStack *sstack, int K) {
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

    }

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
