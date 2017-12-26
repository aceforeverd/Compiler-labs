/*
 * graph.c - Functions to manipulate and create control flow and
 *           interference graphs.
 */

#include "absyn.h"
#include "assem.h"
#include "errormsg.h"
#include "frame.h"
#include "graph.h"
#include "symbol.h"
#include "table.h"
#include "temp.h"
#include "tree.h"

struct G_graph_ {
    int nodecount;
    G_nodeList mynodes;
    /* the tail */
    G_nodeList mylast;
};

struct G_node_ {
    G_graph mygraph;
    /*
     * somewhat a index key
     */
    int mykey;
    G_nodeList succs;
    G_nodeList preds;

    /*
     * extra info
     * a pointer to AS_instr in flowgraph.h
     * or a temporary (list)? in interference graph
     */
    void *info;
};

/*
 * create a empty grpah
 */
G_graph G_Graph(void) {
    G_graph g = (G_graph)checked_malloc(sizeof *g);
    g->nodecount = 0;
    g->mynodes = NULL;
    g->mylast = NULL;
    return g;
}

G_nodeList G_NodeList(G_node head, G_nodeList tail) {
    G_nodeList n = (G_nodeList)checked_malloc(sizeof *n);
    n->head = head;
    n->tail = tail;
    return n;
}

G_nodeList G_reverseNodeList(G_nodeList list) {
    G_nodeList reverse_list = NULL;
    while (list) {
        reverse_list = G_NodeList(list->head, reverse_list);
        list = list->tail;
    }
    return reverse_list;
}

G_node G_finalNode(G_nodeList list) {
    while (list->tail) {
        list = list->tail;
    }
    return list->head;
}

/* get the last node of a graph */
G_node G_lastNode(G_graph g) {
    assert(g);
    if (g->mylast == NULL) {
        return NULL;
    }

    return g->mylast->head;
}

/* generic creation of G_node */
G_node G_Node(G_graph g, void *info) {
    G_node n = (G_node)checked_malloc(sizeof *n);
    G_nodeList p = G_NodeList(n, NULL);
    assert(g);
    n->mygraph = g;
    n->mykey = g->nodecount++;

    if (g->mylast == NULL) {
        g->mylast = p;
        g->mynodes = g->mylast;
    } else {
        g->mylast->tail = p;
        g->mylast = g->mylast->tail;
    }

    n->succs = NULL;
    n->preds = NULL;
    n->info = info;
    return n;
}

G_nodeList G_nodes(G_graph g) {
    assert(g);
    return g->mynodes;
}

/* return true if a is in l list */
bool G_inNodeList(G_node a, G_nodeList l) {
    G_nodeList p;
    for (p = l; p != NULL; p = p->tail)
        if (p->head == a) return TRUE;
    return FALSE;
}

bool G_inGraph(G_graph graph, G_node node) {
    return G_inNodeList(node, graph->mynodes);
}

/* add an edge */
void G_addEdge(G_node from, G_node to) {
    assert(from);
    assert(to);
    assert(from->mygraph == to->mygraph);

    if (G_goesTo(from, to))
        return;

    to->preds = G_NodeList(from, to->preds);
    from->succs = G_NodeList(to, from->succs);
}

/*
 * delete a node from NodeList L
 */
static G_nodeList delete(G_node a, G_nodeList l) {
    assert(a && l);
    if (a == l->head) {
        return l->tail;
    }

    return G_NodeList(l->head, delete(a, l->tail));
}

void G_rmEdge(G_node from, G_node to) {
    assert(from && to);
    to->preds = delete (from, to->preds);
    from->succs = delete (to, from->succs);
}

/**
 * Print a human-readable dump for debugging.
 */
void G_show(FILE *out, G_nodeList p, void showInfo(void *)) {
    for (; p != NULL; p = p->tail) {
        G_node n = p->head;
        G_nodeList q;
        assert(n);
        if (showInfo) showInfo(n->info);
        fprintf(out, " (%d): ", n->mykey);
        for (q = G_succ(n); q != NULL; q = q->tail)
            fprintf(out, "%d ", q->head->mykey);
        fprintf(out, "\n");
    }
}

/*
 * get successor of node n
 */
G_nodeList G_succ(G_node n) {
    assert(n);
    return n->succs;
}

/*
 * get previous node of n
 */
G_nodeList G_pred(G_node n) {
    assert(n);
    return n->preds;
}

/* get number of the nodes in a graph */
int G_grapthNodes(G_graph graph) {
    return graph->nodecount;
}

/*
 * if n is in the list of succ(from)
 */
bool G_goesTo(G_node from, G_node n) { return G_inNodeList(n, G_succ(from)); }

/* return length of predecessor list for node n */
static int inDegree(G_node n) {
    int deg = 0;
    G_nodeList p;
    for (p = G_pred(n); p != NULL; p = p->tail) {
        deg++;
    }
    return deg;
}

/* return length of successor list for node n */
static int outDegree(G_node n) {
    int deg = 0;
    G_nodeList p;
    for (p = G_succ(n); p != NULL; p = p->tail) {
        deg++;
    }
    return deg;
}

int G_degree(G_node n) { return inDegree(n) + outDegree(n); }

/* put list b at the back of list a and return the concatenated list */
static G_nodeList cat(G_nodeList a, G_nodeList b) {
    if (a == NULL)
        return b;

    return G_NodeList(a->head, cat(a->tail, b));
}

/* create the adjacency list for node n by combining the successor and
 * predecessor lists of node n */
G_nodeList G_adj(G_node n) { return cat(G_succ(n), G_pred(n)); }

/*  get node info
 *  AS_instr or templist
 */
void *G_nodeInfo(G_node n) { return n->info; }

/* G_node table functions */

G_table G_empty(void) { return TAB_empty(); }

void G_enter(G_table t, G_node node, void *value) { TAB_enter(t, node, value); }

void *G_look(G_table t, G_node node) { return TAB_look(t, node); }
