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

Live_moveList Live_MoveList(G_node src, G_node dst, Live_moveList tail) {
	Live_moveList lm = (Live_moveList) checked_malloc(sizeof(*lm));
	lm->src = src;
	lm->dst = dst;
	lm->tail = tail;
	return lm;
}


/*
 * what temporary variable is represent by n
 */
Temp_temp Live_gtemp(G_node n) {
    AS_instr instr = G_nodeInfo(n);
}

/*
 * construct the interference graph
 * from a control flow graph
 */
struct Live_graph Live_liveness(G_graph flow) {
	/* struct Live_graph lg; */
    LiveGraph lg = (LiveGraph) checked_malloc(sizeof(*lg));
    G_table def_tmps = G_empty();
    G_table used_tmps = G_empty();
    int length = G_grapthNodes(flow);
    G_nodeList flowNodes = G_nodes(flow);
    while (flowNodes && flowNodes->head) {
        Temp_tempList list = FG_def(flowNodes->head);
        if (list) {
            enterLiveMap(def_tmps, flowNodes->head, list);
        }

        list = FG_use(flowNodes->head);
        if (list) {
            enterLiveMap(used_tmps, flowNodes->head, list);
        }

        flowNodes = flowNodes->tail;
    }


	return *lg;
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
