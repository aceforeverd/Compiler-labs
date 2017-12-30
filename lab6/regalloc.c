#include "absyn.h"
#include "color.h"
#include "flowgraph.h"
#include "graph.h"
#include "liveness.h"
#include "regalloc.h"
#include "symbol.h"
#include "table.h"
#include "tree.h"
#include "util.h"
#include <stdio.h>

/*! TODO: many tricks
 *  \todo many tricks
 */
struct RA_result RA_regAlloc(F_frame f, AS_instrList il) {
    G_graph graph = FG_AssemFlowGraph(il, f);
    struct Live_graph lg = Live_liveness(graph);
    struct COL_result color_result = COL_color(&lg, NULL, F_registers());
    struct RA_result result;
    result.coloring = color_result.coloring;
    result.il = il;
    return result;
}
