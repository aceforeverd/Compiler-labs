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

/*! TODO:
 */
struct RA_result RA_regAlloc(F_frame f, AS_instrList il) {
    G_graph graph = FG_AssemFlowGraph(il, f);
    printf("RA_regAlloc: FG_AssemFlowGraph done\n");
    struct Live_graph lg = Live_liveness(graph);
    printf("RA_regAlloc: Liveness done\n");
    struct COL_result color_result = COL_color(&lg, NULL, F_registers(), F_ava_registers());
    printf("RA_regAlloc: coloring done\n");
    struct RA_result result;
    result.coloring = color_result.coloring;
    result.il = il;
    return result;
}
