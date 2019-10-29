#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <Graph.h>
#include <Parsing.h>

#include "Solving.h"

/**
 * Remove ?
 **/ 
void welcome() {
    printf("\e[31m\n\
         .e$.\n\
        d$\" .d\n\
      e$P   $%%\n\
     $$F\n\
   .$$\"   .$\"3   .$\"\"  .$P $$\n\
  .$$F   d$  4  d$ d$ z$\" J$%%\n\
  $$P   $$ \".$z$$  ^ z$\" .$P\n\
 $$$   d$F  J $$F   z$$  $$ .\n\
4$$F   $$  4\" $$   z$$  $$\".\"\n\
$$$   4$$.d\" 4$$ .$3$$.$$$e%%\n\
$$$    $$*    $$$\" ^$$\"'$$\n\
$$$\n\
'$$c          .e$$$$$$$$e\n\
 \"$$b.   .e$*\"     \"$$$$$$$\n\
   \"*$$$*\"\n\
   \e[0m \n\tDevoir maison Complexité et Calculabilité\n\n\n");
}

// TODO : desactivate for return
bool DEBUG = false;

bool VERBOSE            = false;
bool FORMULA_DISPLAY    = false;
bool BY_DEPTH           = false;
bool DISPLAY_FULL_PATH  = false;
bool WRITE_DOT          = false;


/**
 * @brief Print message if DEBUG flag is set.
 * @param message the message to print.
 **/
void printd(const char* message) {
    if (DEBUG) {
        printf("\n\e[31mDEBUG Message -> \e[0m%s\n", message);
    }
}

void debugFormula(Graph* graphs, unsigned numGraphs) {
    Z3_context ctx = makeContext();

    Z3_ast result = graphsToFullFormula(ctx, graphs, numGraphs);

    //printf("\n\nResult : \n\n%s\n", Z3_ast_to_string(ctx, result));

    Z3_lbool isSat = isFormulaSat(ctx,result);

        switch (isSat)
        {
        case Z3_L_FALSE:
            printf("Formula is not satisfiable.\n");
            break;

        case Z3_L_UNDEF:
                printf("We don't know if formula is satisfiable.\n");
            break;

        case Z3_L_TRUE:
                printf("Formula is satisfiable.\n");
                break;
        }

    Z3_del_context(ctx);
}

void debugFormulaKMAX(Graph* graphs, unsigned numGraphs, int kMax) {
    Z3_context ctx = makeContext();
    Z3_ast result;

    for (int k = 1; k < kMax; k++) {
        result = graphsToPathFormula(ctx, graphs, numGraphs, k);
        Z3_lbool isSat = isFormulaSat(ctx, result);
        switch (isSat)
        {
        case Z3_L_FALSE:
            printf("Formula is not satisfiable for k = %d.\n", k);
            break;

        case Z3_L_UNDEF:
            printf("We don't know if formula is satisfiable for k = %d.\n", k);
            break;

        case Z3_L_TRUE:
            printf("There is a simple valid path of length %d in all graphs\n", k);
            break;
        }
    }

    Z3_del_context(ctx);
}

Graph* loadGraphs(int argc, char**argv, int optind) {
    Graph* graphs = (Graph *) malloc((argc - optind) * sizeof(Graph));

    printd("Loading graphs : ");
    for (int i = optind; i < argc; i++) {
        printd(argv[i]);
        graphs[i - optind] = getGraphFromFile(argv[i]);
    }

    return graphs;
}

/**
 * @brief usage function to show how program works.
 **/
void usage(FILE* where) {
    fprintf(where,    
        "Use: equalPath [options] files...\n \
        Each file should contain a graph in dot format.\n \
        The program decides if there exists a length n such that each input graph has a valid simple path of length n.\n \
        Can display the result both on the command line or in dot format.\n \
        Options: \n \
        -h         Displays this help\n \
        -v         Activate verbose mode (displays parsed graphs)\n \
        -F         Displays the formula computed (obviously not in this version, but you should really display it in your code)\n \
        -s         Tests separately all formula by depth [if not present: uses the global formula]\n \
        -d         Only if -s is present. Explore the length in decreasing order. [if not present: in increasing order]\n \
        -a         Only if -s is present. Computes a result for every length instead of stopping at the first positive result (default behaviour)\n \
        -t         Displays the paths found on the terminal [if not present, only displays the existence of the path].\n \
        -f         Writes the result with colors in a .dot file. See next option for the name. These files will be produced in the folder 'sol'.\n \
        -o NAME    Writes the output in \"NAME-lLENGTH.dot\" where LENGTH is the length of the solution. Writes several files in this format if both -s and -a are present. [if not present: \"result-lLENGTH.dot\"]\n"
    );
}

int main(int argc, char **argv) {
    if (argc == 1) {
        usage(stderr);
        exit(EXIT_FAILURE);
    }
    welcome();

    int opt;
    char *filename = NULL;

    while ((opt = getopt(argc, argv, "DhvFsdatfo:")) != -1) {
        switch (opt)
        {
        case 'D':
        printd("Debug enable.");
            DEBUG = true;
            break;
        case 'h':
    printd("- h option selected.");
            usage(stdin);
            break;
            
        case 'v':
    printd("-v option selected.");
            VERBOSE = true;
            break;

        case 'F':
    printd("-F option selected.");
            FORMULA_DISPLAY = true;
            break;

        case 's':
    printd("-s option selected.");
            BY_DEPTH = true;
            break;
        
        // TODO better argument handler
        case 'd':
    printd("-d option selected.");
            if (!BY_DEPTH) {
                usage(stderr);
                fprintf(stderr, "-d require -s to be present (if present put it before).\n");
                exit(EXIT_FAILURE);
            }
            break;

        // TODO better argument handler
        case 'a':
    printd("-a option selected.");
            if (!BY_DEPTH) {
                usage(stderr);
                fprintf(stderr, "-a require -s to be present (if present put it before).\n");
                exit(EXIT_FAILURE);
            }
            break;

        case 't':
    printd("-t option selected.");
            DISPLAY_FULL_PATH = true;
            break;
        
        case 'f':
    printd("-f option selected");
            WRITE_DOT = true;
            break;

        case 'o':
    printd("-o option selected with name : ");
    printd(optarg);
            filename = optarg;
            break;

        default:
            usage(stderr);
            exit(EXIT_FAILURE);
            break;
        }
    }

    Graph* graphs = loadGraphs(argc, argv, optind);
    int numberGraphs = argc - optind;


    /**
     *  It's time to debug..
     **/

    debugFormulaKMAX(graphs, numberGraphs, 11);


    // Free graphs
    for (int i = 0; i < numberGraphs; i++) {
        deleteGraph(graphs[i]);
    }

    free(graphs);

    return EXIT_SUCCESS;
}