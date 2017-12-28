#ifndef LIVENESS_H
#define LIVENESS_H

#include "graph.h"
#include "temp.h"

typedef struct Live_moveList_ *Live_moveList;
struct Live_moveList_ {
	G_node src;
    G_node dst;
	Live_moveList tail;
};

Live_moveList Live_MoveList(G_node src, G_node dst, Live_moveList tail);
bool Live_move_related(Live_moveList move_list, G_node node);

typedef struct Live_graph *LiveGraph;
struct Live_graph {
	G_graph graph;
	Live_moveList moves;
};

Temp_temp Live_gtemp(G_node n);

struct Live_graph Live_liveness(G_graph flow);

void Live_setup_nodes(G_graph graph, Temp_tempList temps);
void Live_setup_edges(G_node node, G_nodeList list);
G_node Live_find_node(G_graph graph, Temp_temp temp);

#endif
