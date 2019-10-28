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
 * @brief Le chemin de chaque graphe i commence par s_i et finit par t_k.
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
 * @brief Chaque position du chemin est occupée par au moins 1 sommet.
 **/
static Z3_ast secondPartFormula(Z3_context ctx, Graph* graphs, unsigned numGraphs, int k) {
    Z3_ast **orFormula = (Z3_ast **) malloc (numGraphs * sizeof (Z3_ast *));
    Z3_ast *andFormula = (Z3_ast *) malloc (numGraphs * sizeof (Z3_ast));

    for (int currentGraph = 0; currentGraph < numGraphs; currentGraph++) {
        orFormula[currentGraph] = (Z3_ast*) malloc((k - 2) * sizeof(Z3_ast));

        for (int position = 1; position < k - 1; position++) {
            orFormula[currentGraph][position - 1] = getNodeVariable(ctx, currentGraph, position, k, 0);

            for (int node = 1; node < orderG(graphs[currentGraph]); node++) {
                Z3_ast tmpFormula[] = {
                    getNodeVariable(ctx, currentGraph, position, k, node),
                    orFormula[currentGraph][position - 1]
                };

                orFormula[currentGraph][position - 1] = Z3_mk_or(
                    ctx,
                    2,
                    tmpFormula
                );
            }
        }

        // Plante ici
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

/**
 * @brief Chaque position 0 < j < k est occupée par au maximum un sommet.
 **/
static Z3_ast thirdPartFormula(Z3_context ctx, Graph* graphs, unsigned numGraphs, int k) {
    Z3_ast* andFormula = (Z3_ast *) malloc(numGraphs * sizeof(Z3_ast));

    for (int currentGraph = 0; currentGraph < numGraphs; currentGraph++) {
        int size = orderG(graphs[currentGraph]);
        Z3_ast* orFormula = (Z3_ast*) malloc(k * size * (size - 1));

        for (int position = 0; position < k; position++) {
            for(int firstNode = 0; firstNode < size; firstNode++) {
                int index = 0;

                for(int secondNode = 0; secondNode < size; secondNode++) {
                    if (firstNode == secondNode) {
                        continue;
                    }
                    Z3_ast tmpFormula[] = {
                        Z3_mk_not(ctx, getNodeVariable(ctx, currentGraph, position, k, firstNode)),
                        Z3_mk_not(ctx, getNodeVariable(ctx, currentGraph, position, k, secondNode))
                    };

                    orFormula[position * k + firstNode * size + index] = Z3_mk_or(
                        ctx,
                        2,
                        tmpFormula
                    );
                    index ++;
                }
            }
        }

        andFormula[currentGraph] = Z3_mk_and(
            ctx,
            k * size * (size - 1),
            orFormula
        );
    }

    return Z3_mk_and(
        ctx,
        numGraphs,
        andFormula
    );
}

/**
 * @brief Chaque sommet occupe soit une position unique, soit aucune position.
 **/
static Z3_ast fourthPartFormula(Z3_context ctx, Graph* graphs, unsigned numGraphs, int k) {
    Z3_ast* andFormula = (Z3_ast *) malloc(numGraphs * sizeof(Z3_ast));

    for (int currentGraph = 0; currentGraph < numGraphs; currentGraph++) {
        int size = orderG(graphs[currentGraph]);
        Z3_ast* orFormula = (Z3_ast*) malloc(size * k * (k - 1));

        for (int node = 0; node < orderG(graphs[currentGraph]); node++) {
            for (int i = 0; i < k; i++) {
                int index = 0;
                for (int j = 0; j < k; j++) {
                    if (i == j) {
                        continue;
                    }

                    Z3_ast tmpFormula[] = {
                        Z3_mk_not(ctx, getNodeVariable(ctx, currentGraph, i, k, node)),
                        Z3_mk_not(ctx, getNodeVariable(ctx, currentGraph, j, k, node))
                    };

                    orFormula[node * size + k * i + j] = Z3_mk_or(
                        ctx,
                        2,
                        tmpFormula
                    );

                    index++;
                }
            }
        }

        andFormula[currentGraph] = Z3_mk_and(
            ctx,
            size * k * (k - 1),
            orFormula
        );
    }

    return Z3_mk_and(
        ctx,
        numGraphs,
        andFormula
    );
}

/**
 * @brief Les sommets occupant les positions forment bien un chemin.
 **/
static Z3_ast fifthPartFormula(Z3_context ctx, Graph* graphs, unsigned numGraphs, int k) {
    Z3_ast* andFormulaFinal = (Z3_ast *) malloc(numGraphs * sizeof(Z3_ast));
    for (int currentGraph = 0; currentGraph < numGraphs; currentGraph++) {

        Z3_ast* andFormula = (Z3_ast*) malloc(k - 1);
        for(int i = 0; i < k - 1; i++) {
            int size = orderG(graphs[currentGraph]);
            Z3_ast* orFormula = (Z3_ast*) malloc(sizeG(graphs[currentGraph]) * size);


            for (int firstNode = 0; firstNode < size; firstNode++) {
                Z3_ast* orEdgeFormula = (Z3_ast*) malloc(sizeG(graphs[currentGraph]));
                int index = 0;

                for (int secondNode = 0; secondNode < size; secondNode++) {
                    if (isEdge(graphs[currentGraph], firstNode, secondNode)) {
                        Z3_ast tmpFormula[] = {
                            Z3_mk_not(ctx, getNodeVariable(ctx, currentGraph, i, k, firstNode)),
                            getNodeVariable(ctx, currentGraph, i + 1, k, secondNode)
                        };

                        orEdgeFormula[index++] = Z3_mk_or(
                            ctx,
                            2,
                            tmpFormula
                        );
                    }
                }

                orFormula[firstNode] = Z3_mk_or(
                    ctx,
                    index,
                    orEdgeFormula
                );
            }

            andFormula[i] = Z3_mk_and(
                ctx,
                size,
                orFormula
            );
        }
        andFormulaFinal[currentGraph] = Z3_mk_and(
            ctx,
            numGraphs,
            andFormula
        );
    }

    return Z3_mk_and(
        ctx,
        numGraphs,
        andFormulaFinal
    );
}

Z3_ast graphsToPathFormula( Z3_context ctx, Graph *graphs,unsigned int numGraphs, int pathLength) {
    Z3_ast formulaParts[] = {
        firstPartFormula(ctx, graphs, numGraphs, pathLength),
        secondPartFormula(ctx, graphs, numGraphs, pathLength),
        thirdPartFormula(ctx, graphs, numGraphs, pathLength),
        fourthPartFormula(ctx, graphs, numGraphs, pathLength),
        fifthPartFormula(ctx, graphs, numGraphs, pathLength)
    };

    return Z3_mk_and(
        ctx,
        5,
        formulaParts
    );
}