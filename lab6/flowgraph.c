#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "absyn.h"
#include "errormsg.h"
#include "flowgraph.h"
#include "symbol.h"
#include "table.h"
#include "tree.h"

static G_nodeList make_label_list(G_nodeList list);
static G_node find_label_node(G_nodeList label_list, Temp_label label);

/*
 * destination of the instruction
 */
Temp_tempList FG_def(G_node n) {
    AS_instr instr = G_nodeInfo(n);

    switch(instr->kind) {
        case I_OPER:
            return instr->u.OPER.dst;
        case I_LABEL:
            return NULL;
        case I_MOVE:
            return instr->u.MOVE.dst;
    }
}

/*
 * source registers of the instruction
 */
Temp_tempList FG_use(G_node n) {
    AS_instr instr = G_nodeInfo(n);

    switch(instr->kind) {
        case I_OPER:
            return instr->u.OPER.src;
        case I_LABEL:
            return NULL;
        case I_MOVE:
            return instr->u.MOVE.src;
    }
}

/*
 * whether n is a MOVE instruction
 */
bool FG_isMove(G_node n) {
    AS_instr instr = G_nodeInfo(n);
    return instr->kind == I_MOVE;
}

/*
 * generate control flow graph
 * TODO: to use F_frame
 * TODO: to support JMP
 */
G_graph FG_AssemFlowGraph(AS_instrList il, F_frame f) {
    G_graph graph  = G_Graph();

    AS_instrList list = il;
    while (list) {
        /* add points */
        G_node last_node = G_lastNode(graph);
        G_node node = G_Node(graph, list->head);

        if (last_node != NULL) {
            /* add edge */
            G_addEdge(last_node, node);
        }

        list = list->tail;
    }

    /* jumps */
    G_nodeList node_list = G_nodes(graph);
    G_nodeList labe_list = make_label_list(node_list);
    while (node_list) {
        AS_instr instr = G_nodeInfo(node_list->head);
        if (instr->kind == I_OPER && instr->u.OPER.jumps) {
            Temp_labelList labels = instr->u.OPER.jumps->labels;
            while (labels) {
                G_node node = find_label_node(labe_list, labels->head);
                if (node) {
                    G_addEdge(node_list->head, node);
                }
                labels = labels->tail;
            }
        }

        node_list = node_list->tail;
    }

    return graph;
}

static G_nodeList make_label_list(G_nodeList list) {
    if (!list) {
        return NULL;
    }

    AS_instr instr = G_nodeInfo(list->head);
    if (instr->kind == I_LABEL) {
        return G_NodeList(list->head, make_label_list(list->tail));
    }

    return make_label_list(list->tail);
}

static G_node find_label_node(G_nodeList label_list, Temp_label label) {
    while (label_list) {
        AS_instr instr = G_nodeInfo(label_list->head);
        assert(instr->kind == I_LABEL);
        if (instr->u.LABEL.label == label) {
            return label_list->head;
        }

        label_list = label_list->tail;
    }

    return NULL;
}
