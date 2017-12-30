#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "absyn.h"
#include "errormsg.h"
#include "flowgraph.h"
#include "symbol.h"
#include "table.h"
#include "tree.h"

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

    return graph;
}
