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

/**
 * @brief return the index of the graph's source node. 
 **/
static unsigned getSourceNode(Graph graph) {
    int node;
    for (node = 0; node < orderG(graph) && !isSource(graph, node); node++);

    return node;
}

/**
 * @brief return the index of the graph's target node. 
 **/
static unsigned getTargetNode(Graph graph) {
    int node;
    for (node = 0; node < orderG(graph) && !isTarget(graph, node); node++);

    return node;
}

/**
 * Le chemin de chaque graphe i commence par s_i et finit par t_k.
 **/
static Z3_ast firstPartFormula(Z3_context ctx, Graph* graphs, unsigned numGraphs, int k) {
    Z3_ast *formula = (Z3_ast*) malloc (numGraphs * sizeof(Z3_ast));
    
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

/**
 * Chaque position du chemin est occupée par au moins 1 sommet.
 **/
static Z3_ast secondPartFormula(Z3_context ctx, Graph* graphs, unsigned numGraphs, int k) {
    Z3_ast **orFormula = (Z3_ast **) malloc (numGraphs * sizeof (Z3_ast *));
    Z3_ast *andFormula = (Z3_ast *) malloc (numGraphs * sizeof (Z3_ast));

    for (int currentGraph = 0; currentGraph < numGraphs; currentGraph++) {
        orFormula[currentGraph] = (Z3_ast*) malloc((k - 2) * sizeof(Z3_ast));

        for (int position = 1; position < k - 1; position++) {
            orFormula[currentGraph][position] = getNodeVariable(ctx, currentGraph, position, k, 0);
            for (int node = 1; node < orderG(graphs[currentGraph]); node++) {
                Z3_ast tmpFormula[] = {
                    getNodeVariable(ctx, currentGraph, position, k, node),
                    orFormula[currentGraph][position]
                };

                orFormula[currentGraph][position] = Z3_mk_or(
                    ctx,
                    2,
                    tmpFormula
                );
            }
        }
        andFormula[currentGraph] = Z3_mk_and(
            ctx,
            k - 2,
            orFormula[currentGraph]
        );
    }

    return Z3_mk_and(
        ctx,
        numGraphs,
        andFormula
    );
}

Z3_ast graphsToPathFormula( Z3_context ctx, Graph *graphs,unsigned int numGraphs, int pathLength) {
    return 0;
}