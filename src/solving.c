#include "Solving.h"
#include <stdio.h>
#include <stdlib.h>

#define MAX_NAME_SIZE 50

Z3_ast getNodeVariable(Z3_context ctx, int number, int position, int k, int node) {
    // Penser à free name à la fin du prog. Mettre dans une liste chainée ?
    char *name = (char *) malloc(MAX_NAME_SIZE * sizeof(char));
    snprintf(name, MAX_NAME_SIZE, "x(%d, %d, %d, %d)", number, position, k, node);

    return mk_bool_var(ctx, name);
}

static unsigned getSourceNode(Graph graph) {
    int node;
    for (node = 0; node < orderG(graph) && !isSource(graph, node); node++);

    return node;
}

static unsigned getTargetNode(Graph graph) {
    int node;
    for (node = 0; node < orderG(graph) && !isTarget(graph, node); node++);

    return node;
}

static Z3_ast firstPartFormula(Z3_context ctx, Graph* graphs, unsigned numGraphs, int k) {
    Z3_ast formula[numGraphs];
    
    for (int currentGraph = 0; currentGraph < numGraphs; currentGraph++) {
        Z3_ast tmpFormula[] = {
            getNodeVariable(ctx, currentGraph, 0, k, getSourceNode(graphs[currentGraph])),
            getNodeVariable(ctx, currentGraph, k, k, getTargetNode(graphs[currentGraph]))
        };

        formula[currentGraph] = Z3_mk_and(
            ctx,
            2,
            tmpFormula
        ); 
    }

    return Z3_mk_and(
        ctx,
        numGraphs,
        formula
    );
}

Z3_ast graphsToPathFormula( Z3_context ctx, Graph *graphs,unsigned int numGraphs, int pathLength) {
    return 0;
}