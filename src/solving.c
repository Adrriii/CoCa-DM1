#include "Solving.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define MAX_NAME_SIZE 50
#define DOT_DIRECTORY "sol/"
extern bool DEBUG;

extern void printd(const char* message);


// TODO : enlever toute la duplication de code !

Z3_ast getNodeVariable(Z3_context ctx, int number, int position, int k, int node) {
    char name[MAX_NAME_SIZE];
    snprintf(name, MAX_NAME_SIZE, "x(%d, %d, %d, %d)", number, position, k, node);

    return mk_bool_var(ctx, name);
}


/**
 * @brief Get the Source Node object
 *
 * @param graph graph in wich looking for source node
 * @return unsigned the index of the source node
 */
static unsigned getSourceNode(Graph graph) {
    int node;
    for (node = 0; node < orderG(graph) && !isSource(graph, node); node++);

    return node;
}


/**
 * @brief Get the Target Node object
 *
 * @param graph graph in wich looking for target node
 * @return unsigned the index of the target node
 */
static unsigned getTargetNode(Graph graph) {
    int node;
    for (node = 0; node < orderG(graph) && !isTarget(graph, node); node++);

    return node;
}


/**
 * @brief Le chemin de chaque graphe i commence par s_i et finit par t_k.
 **/
static Z3_ast firstPartFormula(Z3_context ctx, Graph* graphs, unsigned numGraphs, int k) {
    printd("First formula !");
    Z3_ast sourceTargetAndFormula[numGraphs];

    for (int currentGraph = 0; currentGraph < numGraphs; currentGraph++) {
        Z3_ast tmpFormula[] = {
            getNodeVariable(ctx, currentGraph, 0, k, getSourceNode(graphs[currentGraph])),
            getNodeVariable(ctx, currentGraph, k, k, getTargetNode(graphs[currentGraph]))
        };

        sourceTargetAndFormula[currentGraph] = Z3_mk_and(ctx,2,tmpFormula);
    }

    // Return graph big and formula
    return Z3_mk_and(ctx,numGraphs,sourceTargetAndFormula);
}


/**
 * @brief Chaque position du chemin est occupée par au moins 1 sommet.
 **/
static Z3_ast secondPartFormula(Z3_context ctx, Graph* graphs, unsigned numGraphs, int k) {
    printd("Second formula !");
    Z3_ast andFormula[numGraphs];

    for (int currentGraph = 0; currentGraph < numGraphs; currentGraph++) {
        int size = orderG(graphs[currentGraph]);
        Z3_ast orPositionFormula[k + 1];

        //TODO Parler dans le rapport de l'opti 1 | k - 1 qui fail ?
        for (int position = 0; position <= k; position++) {
            Z3_ast singleton[size];

            for (int node = 0; node < size; node++) {
                singleton[node] = getNodeVariable(ctx, currentGraph, position, k, node);
            }

            orPositionFormula[position] = Z3_mk_or(ctx, size,singleton);
        }

        andFormula[currentGraph] = Z3_mk_and(ctx,k + 1,orPositionFormula);
    }

    // Return graph big and formula
    return Z3_mk_and(ctx,numGraphs,andFormula);
}


/**
 * @brief Chaque position 0 < j < k est occupée par au maximum un sommet.
 **/
static Z3_ast thirdPartFormula(Z3_context ctx, Graph* graphs, unsigned numGraphs, int k) {
    printd("Third formula !");
    Z3_ast andFormula[numGraphs];

    for (int currentGraph = 0; currentGraph < numGraphs; currentGraph++) {
        int size = orderG(graphs[currentGraph]);
        Z3_ast positionAndFormula[k + 1];

        for (int position = 0; position <= k; position++) {
            Z3_ast firstNodeAndFormula[size];

            for(int firstNode = 0; firstNode < size; firstNode++) {
                int index = 0;
                Z3_ast orFormula[size];

                for(int secondNode = 0; secondNode < size; secondNode++) {
                    if (firstNode == secondNode) {
                        continue;
                    }

                    Z3_ast tmpFormula[] = {
                        Z3_mk_not(ctx, getNodeVariable(ctx, currentGraph, position, k, firstNode)),
                        Z3_mk_not(ctx, getNodeVariable(ctx, currentGraph, position, k, secondNode))
                    };

                    orFormula[index] = Z3_mk_or(ctx,2,tmpFormula);
                    index ++;
                }

                firstNodeAndFormula[firstNode] = Z3_mk_and(ctx,index,orFormula);
            }

            positionAndFormula[position] = Z3_mk_and(ctx,size,firstNodeAndFormula);
        }

        andFormula[currentGraph] = Z3_mk_and(ctx,k + 1,positionAndFormula);
    }

    return Z3_mk_and(ctx,numGraphs,andFormula);
}


/**
 * @brief Chaque sommet occupe soit une position unique, soit aucune position.
 **/
static Z3_ast fourthPartFormula(Z3_context ctx, Graph* graphs, unsigned numGraphs, int k) {
    printd("Fourth formula !");
    Z3_ast graphAndFormula[numGraphs];

    for (int currentGraph = 0; currentGraph < numGraphs; currentGraph++) {
        int size = orderG(graphs[currentGraph]);
        Z3_ast nodeAndFormula[size];

        for (int node = 0; node < size; node++) {
            Z3_ast positionAndFormula[k + 1];

            for (int i = 0; i <= k; i++) {
                int index = 0;
                Z3_ast orFormula[k + 1];

                for (int j = 0; j <= k; j++) {
                    if (i == j) {
                        continue;
                    }

                    Z3_ast tmpFormula[] = {
                        Z3_mk_not(ctx, getNodeVariable(ctx, currentGraph, i, k, node)),
                        Z3_mk_not(ctx, getNodeVariable(ctx, currentGraph, j, k, node))
                    };

                    orFormula[index] = Z3_mk_or(ctx,2,tmpFormula);
                    index++;
                }

                positionAndFormula[i] = Z3_mk_and(ctx, index, orFormula);
            }

            nodeAndFormula[node] = Z3_mk_and(ctx,k + 1,positionAndFormula);

        }

        graphAndFormula[currentGraph] = Z3_mk_and(ctx, size, nodeAndFormula);
    }

    return Z3_mk_and(ctx,numGraphs,graphAndFormula);
}


/**
 * @brief Les sommets occupant les positions forment bien un chemin.
 **/
static Z3_ast fifthPartFormula(Z3_context ctx, Graph* graphs, unsigned numGraphs, int k) {
    printd("Fifth formula !");
    int size;
    Z3_ast andFormulaFinal[numGraphs];

    for (int currentGraph = 0; currentGraph < numGraphs; currentGraph++) {
        size = orderG(graphs[currentGraph]);
        Z3_ast andFormula[k + 1];

        for(int i = 0; i <= k; i++) {
            Z3_ast orFormula[sizeG(graphs[currentGraph])];

            for (int firstNode = 0; firstNode < size; firstNode++) {
                Z3_ast orEdgeFormula[sizeG(graphs[currentGraph])];
                int index = 0;

                for (int secondNode = 0; secondNode < size; secondNode++) {
                    if (!isEdge(graphs[currentGraph], firstNode, secondNode)) {
                        Z3_ast tmpFormula[] = {
                            Z3_mk_not(ctx, getNodeVariable(ctx, currentGraph, i, k, firstNode)),
                            Z3_mk_not(ctx, getNodeVariable(ctx, currentGraph, i + 1, k, secondNode))
                        };

                        orEdgeFormula[index] = Z3_mk_or(ctx,2,tmpFormula);
                        index++;
                    }
                }

                orFormula[firstNode] = Z3_mk_and(ctx,index,orEdgeFormula);
            }

            andFormula[i] = Z3_mk_and(ctx,size,orFormula);
        }
        andFormulaFinal[currentGraph] = Z3_mk_and(ctx,k,andFormula);
    }

    return Z3_mk_and(ctx,numGraphs,andFormulaFinal);
}


Z3_ast graphsToPathFormula( Z3_context ctx, Graph *graphs,unsigned int numGraphs, int pathLength) {
    printd("Welcome in graphsToPathFormula !");

    Z3_ast formulaParts[] = {
        firstPartFormula(ctx, graphs, numGraphs, pathLength),
        secondPartFormula(ctx, graphs, numGraphs, pathLength),
        thirdPartFormula(ctx, graphs, numGraphs, pathLength),
        fourthPartFormula(ctx, graphs, numGraphs, pathLength),
        fifthPartFormula(ctx, graphs, numGraphs, pathLength)
    };



    return Z3_mk_and(ctx,5,formulaParts);
}


/**
 * @brief Find the value for k_max
 *
 * @param graphs array of graphs to compute with
 * @param numGraphs size of graphs array
 * @return int the value of k_max
 */
int kMaxValue(Graph* graphs, unsigned numGraphs) {
    if (numGraphs == 0) return -1;

    int minSize = orderG(graphs[0]);
    int tmpSize;

    for (int i = 1; i < numGraphs; i++) {
        if ((tmpSize = orderG(graphs[i])) < minSize) {
            minSize = tmpSize;
        }
    }

    return minSize - 1;
}


Z3_ast graphsToFullFormula(Z3_context ctx, Graph *graphs,unsigned int numGraphs) {
    int kMax = kMaxValue(graphs, numGraphs);

    Z3_ast orFormula[kMax];

    // De part la simplicité des chemins, il ne peut y avoir k = 0
    for (int i = 1; i < kMax; i++) {
        orFormula[i - 1] = graphsToPathFormula(
            ctx,
            graphs,
            numGraphs,
            i
        );
    }

    return Z3_mk_or(ctx, kMax - 1, orFormula);
}


void printPathsFromModel(Z3_context ctx, Z3_model model, Graph *graphs, int numGraph, int pathLength) {
    int size, value;

    for (int i = 0; i < numGraph; i++) {
        size = orderG(graphs[i]);
        printf("Path in graph %d\n", i);

        for (int pos = 0; pos <= pathLength; pos++) {
            for (int node = 0; node < size; node++) {

                Z3_ast currentVar = getNodeVariable(ctx, i, pos, pathLength, node);
                value = valueOfVarInModel(ctx,model,currentVar);

                if (value) {
                    printf("%d: pos %d: %s%s", i, pos, getNodeName(graphs[i], node), (pos == pathLength)? "\n":" -> ");
                }
            }
        }
    }
}


/**
 * @brief Looking in model if a position in a graph is occuped
 * 
 * @param ctx Z3 context
 * @param graphs array of graphs
 * @param graphIndex index of current graph
 * @param position position to check in model
 * @param k value of current path length
 * @param model Z3 model
 * @return true if a node is at the given position
 * @return false if not
 */
static bool isPositionOccuped(Z3_context ctx, Graph *graphs, int graphIndex, int position, int k, Z3_model model) {
    int size = orderG(graphs[graphIndex]);

    for (int node = 0; node < size; node++) {
        Z3_ast currentVar = getNodeVariable(ctx, graphIndex, position, k, node);

        if (valueOfVarInModel(ctx, model, currentVar)) {
            return true;
        }
    }

    return false;
}


int getSolutionLengthFromModel(Z3_context ctx, Z3_model model, Graph *graphs) {
    int nbGraphs = -1;

    const char* baseString = Z3_model_to_string(ctx, model);
    char string[strlen(baseString)];

    strncpy(string, baseString, strlen(baseString));
    char delim[] = "\n";
    char *ptr = strtok(string, delim);

    // Looking in the model for size of graphs. Needed for kMaxValue
    while(ptr) {
        int i, position, k, node;
        char value[5];
        sscanf(ptr, "x(%d, %d, %d, %d) -> %s", &i, &position, &k, &node, value);

        if (i > nbGraphs) nbGraphs = i;
        ptr = strtok(NULL, delim);
    }

    int kMax = kMaxValue(graphs, nbGraphs + 1);

    for (int k = 0; k <= kMax; k++) {
        bool found = true;

        for (int i = 0; i <= nbGraphs; i++) {
            for (int position = 0; position <= k; position++) {
                if (!isPositionOccuped(ctx, graphs, i, position, k, model)) {
                    found = false;
                    break;
                }
            }

            if (!found) break;
        }

        if (found) {
            return k;
        }
    }

    // Error ?
    return -1;
}


void createDotFromModel(Z3_context ctx, Z3_model model, Graph *graphs, int numGraph, int pathLength, char* name) {
    int value;

    char realName[100];

    snprintf(realName, 100, "%s%s.dot", DOT_DIRECTORY, name);

    system("mkdir "DOT_DIRECTORY" 2> /dev/null"); // Hide error when sol/ already exists
    FILE* graph = fopen(realName, "w");
    

    sscanf(name, "%s.dot", name);
    for (int i = 0; i < strlen(name); i++) {
        if (name[i] == '-') name[i] = '_';
    }

    fprintf(graph, "digraph %s{\n", name);

    for (int i = 0; i < numGraph; i++) {
        for (int node = 0; node < orderG(graphs[i]); node++) {

            if (isSource(graphs[i], node)) {
                fprintf(graph, "_%d_%s [initial=1,color=green][style=filled,fillcolor=lightblue];\n", i, getNodeName(graphs[i], node));
                continue;
            }

            if (isTarget(graphs[i], node)) {
                fprintf(graph, "_%d_%s [final=1,color=red][style=filled,fillcolor=lightblue];\n", i, getNodeName(graphs[i], node));
                continue;
            }
            value = false;
            for (int pos = 0; pos <= pathLength; pos++) {
                Z3_ast currentVar = getNodeVariable(ctx, i, pos, pathLength, node);
                if (valueOfVarInModel(ctx,model,currentVar)) {
                    value = true;
                    break;
                }
            }

            if (value) {
                fprintf(graph, "_%d_%s [style=filled,fillcolor=lightblue];\n", i, getNodeName(graphs[i], node));
            } else {
                fprintf(graph, "_%d_%s ;\n", i, getNodeName(graphs[i], node));
            }
        }

        for (int firstNode = 0; firstNode < orderG(graphs[i]); firstNode++) {
            for (int secondNode = 0; secondNode < orderG(graphs[i]); secondNode++) {
                if (isEdge(graphs[i], firstNode, secondNode)) {
                    value = false;

                    // Don't take the return
                    if (!(isSource(graphs[i], secondNode) && isTarget(graphs[i], firstNode))) {
                        for (int pos = 0; pos <= pathLength; pos++) {
                            if (valueOfVarInModel(ctx, model, getNodeVariable(ctx, i, pos, pathLength, firstNode))) {
                                if (valueOfVarInModel(ctx,model,getNodeVariable(ctx, i, pos + 1, pathLength, secondNode))) {
                                    value = true;
                                }
                            }
                        }
                    }

                    if (value) {
                        fprintf(graph, "_%d_%s -> _%d_%s [color=blue];\n", i, getNodeName(graphs[i], firstNode), i, getNodeName(graphs[i], secondNode));
                    } else {
                        fprintf(graph, "_%d_%s -> _%d_%s ;\n", i, getNodeName(graphs[i], firstNode), i, getNodeName(graphs[i], secondNode));

                    }
                }
            }
        }

    }

    fprintf(graph, "}\n");

    fclose(graph);
}
