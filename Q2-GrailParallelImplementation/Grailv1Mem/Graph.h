#ifndef GRAPH_H
#define GRAPH_H
#include "Edge.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <Windows.h>
#include <time.h>
#include <ctime>
typedef struct graph* Graph;

Graph GRAPHinit(int N, const char *modality);
void GRAPHFree(Graph G);
void GRAPHinsert(Graph G, Edge e);
Graph GraphParallelRead(LPTSTR inputFile, Graph G, int NThread);
void GraphPrint(Graph G);
void RandomizedLabelling(Graph G,int d);
void printLabels(Graph G);
void printLabelsOnFile(Graph G, LPTSTR file);
void printRoots(Graph G, FILE* fp);
void QueryResolutionSetup(LPTSTR queryFile, Graph G, int d);
#endif //E01_GRAPH_H