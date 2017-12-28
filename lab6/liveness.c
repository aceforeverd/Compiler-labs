#include "absyn.h"
#include "assem.h"
#include "flowgraph.h"
#include "frame.h"
#include "liveness.h"
#include "symbol.h"
#include "table.h"
#include "tree.h"
#include "util.h"
#include <stdio.h>

static void enterLiveMap(G_table t, G_node flowNode, Temp_tempList temps);
static Temp_tempList lookupLiveMap(G_table t, G_node flowNode);
static Temp_tempList calc_node_outs(G_node node, G_table in_tmps);

Live_moveList Live_MoveList(G_node src, G_node dst, Live_moveList tail) {
	Live_moveList lm = (Live_moveList) checked_malloc(sizeof(*lm));
	lm->src = src;
	lm->dst = dst;
	lm->tail = tail;
	return lm;
}

bool Live_move_related(Live_moveList move_list, G_node node) {
    if (!node || !move_list) return FALSE;
    while (move_list) {
        if (node == move_list->src || node == move_list->dst) {
            return TRUE;
        }

        move_list = move_list->tail;
    }
    return FALSE;
}

void Live_setup_nodes(G_graph graph, Temp_tempList temps) {
    G_nodeList node_list = NULL;
    while (temps) {
        /* add nodes */
        G_node node = Live_find_node(graph, temps->head);
        if (!node) {
            node = G_Node(graph, temps->head);
        }
        node_list = G_NodeList(node, node_list);
        temps = temps->tail;
    }

    while (node_list) {
        /* add edges(undirected/double directed) */
        Live_setup_edges(node_list->head, node_list->tail);
        node_list = node_list->tail;
    }
}

/* add edge for interference graph */
void Live_setup_edges(G_node node, G_nodeList list) {
    while (list) {
        /* addEdge will auto check if edge already exist */
        /* undirected graph */
        G_addEdge(node, list->head);
        G_addEdge(list->head, node);
        list = list->tail;
    }
}

G_node Live_find_node(G_graph graph, Temp_temp temp) {
    assert(temp);
    G_nodeList node_list = G_nodes(graph);
    while (node_list) {
        if (Temp_equal(temp, (Temp_temp) G_nodeInfo(node_list->head))) {
            return G_nodeInfo(node_list->head);
        }
        node_list = node_list->tail;
    }
    return NULL;
}
/*
 * what temporary variable is represent by n
 */
Temp_temp Live_gtemp(G_node n) {
    return (Temp_temp) G_nodeInfo(n);
}

/*
 * construct the interference graph
 * from a control flow graph
 * flow is a directed graph
 * while Live_graph.graph is a undirected graph
 * TODO: use ins from top node and outs from last node
 */
struct Live_graph Live_liveness(G_graph flow) {
	/* struct Live_graph lg; */
    struct Live_graph lg;
    lg.graph = G_Graph();
    lg.moves = NULL;

    G_table in_tmps = G_empty();
    G_table out_tmps = G_empty();

    G_nodeList flow_nodes = G_nodes(flow);

    /*
     * in[n] = use[n] U (out[n] - def[n])
     * out[n] = SUM(in[s]) where s belong succ[n]
     */
    G_nodeList reverse_node_list = G_reverseNodeList(flow_nodes);
    bool changed = TRUE;
    G_nodeList current_list;
    while (changed) {
        changed = FALSE;
        current_list = reverse_node_list;
        while (current_list) {
            G_node node = current_list->head;
            Temp_tempList node_in_old = lookupLiveMap(in_tmps, node);
            Temp_tempList node_out_old = lookupLiveMap(out_tmps, node);

            Temp_tempList node_out = calc_node_outs(node, in_tmps);
            Temp_tempList node_in = Temp_ListUnion(FG_use(node),
                    Temp_ListExclude(node_out, FG_def(node)));

            if (!Temp_ListEqual(node_in, node_in_old)) {
                enterLiveMap(in_tmps, node, node_in);
                changed = TRUE;
            }
            if (!Temp_ListEqual(node_out, node_out_old)) {
                enterLiveMap(out_tmps, node, node_out);
                changed = TRUE;
            }

            current_list = current_list->tail;
        }
    }

    current_list = reverse_node_list;
    while (current_list) {
        /* creating interference graph */
        G_node node = current_list->head;
        Temp_tempList node_ins = lookupLiveMap(in_tmps, node);
        Temp_tempList node_outs = lookupLiveMap(out_tmps, node);

        Live_setup_nodes(lg.graph, node_ins);
        Live_setup_nodes(lg.graph, node_outs);

        if (FG_isMove(node)) {
            Temp_tempList move_src_list = FG_use(node);
            Temp_tempList move_dst_list = FG_def(node);

            assert(move_src_list->tail == NULL);
            assert(move_dst_list->tail == NULL);

            G_node move_src = Live_find_node(lg.graph, move_src_list->head);
            G_node move_dst = Live_find_node(lg.graph, move_dst_list->head);

            lg.moves = Live_MoveList(move_src, move_dst, lg.moves);
        }

        current_list = current_list->tail;
    }

	return lg;
}

Temp_tempList calc_node_outs(G_node node, G_table in_tmps) {
    G_nodeList succ_nodes = G_succ(node);
    Temp_tempList outs = NULL;
    while (succ_nodes) {
        outs = Temp_ListUnion(outs, lookupLiveMap(in_tmps, succ_nodes->head));
    }
    return outs;
}


/*
 * add flowNode => temps to G_table t
 */
static void enterLiveMap(G_table t, G_node flowNode,
        Temp_tempList temps) {
    G_enter(t, flowNode, temps);
}

/*
 * lookup a node in G_table
 */
static Temp_tempList lookupLiveMap(G_table t, G_node flowNode) {
    return (Temp_tempList)G_look(t, flowNode);
}
