#include "Graph.h"
#include <tchar.h>
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>

int _tmain(int argc, LPTSTR argv[]) {
	/*FILE NAMES ARE PASSED VIA COMMAND LINE IN THE FORMAT inputGraphFile n_labels inputQueryFile*/

	int nNodes;
	double threshold;
	Graph G;
	if (argc < 4) {
		fprintf(stderr, ("Missing parameters!\nPass the args in the format inputGraphFile n_labels queryFile\n"));
		return 1;
	}
	LPTSTR inputFile=argv[1];
	LPTSTR queryFile = argv[3];
	
	FILE* fIn;
	_wfopen_s(&fIn, inputFile, _T("r"));
	if (fIn == NULL) {
		fprintf(stderr, ("Error opening input file!\n"));
	}

	int LABELS = min(5,atoi((const char*)argv[2]));

	FILE* fQue;
	_wfopen_s(&fQue, queryFile, _T("r"));
	if (fQue == NULL) {
		fprintf(stderr, ("Error opening query file!\n"));
	}

	clock_t begin_list, end_list;
	double querytime = 0;
	
	fscanf_s(fIn, ("%ld\n"), &nNodes);

	threshold = ((double)nNodes * (nNodes - 1) )* 0.45;
	
	char modality[10] = { "list" };
	
	G = GRAPHinit(nNodes, modality);


	//printf("-----------------------------------------------------------------\n");
	printf("------------------------ GRAPH READING --------------------------\n\n");
	//printf("-----------------------------------------------------------------\n");

	begin_list = clock();
	GraphParallelRead(inputFile, G, 5);
	end_list = clock();
	querytime = (double)(end_list - begin_list) / CLOCKS_PER_SEC;
	printf("\n---> Reading graph time: %f s\n\n", querytime);
	//printf("-----------------------------------------------------------------\n");
	printf("---------------------------   ROOTS   ---------------------------\n\n");
	//printf("-----------------------------------------------------------------\n");
	printRoots(G, stdout);
	
	//printf("-----------------------------------------------------------------\n");
	printf("---------------------------  LABELS   ---------------------------\n\n");
	//printf("-----------------------------------------------------------------\n");

	begin_list = clock(); 
	RandomizedLabelling(G,LABELS);

	end_list = clock();
	querytime = (double)(end_list - begin_list) / CLOCKS_PER_SEC;
	printf("\n---> Creation labels time: %f s\n\n", querytime);
	TCHAR fileName[100] = { _T("Labels.txt") };
	printLabelsOnFile(G, fileName);
	//printf("-----------------------------------------------------------------\n");
	printf("---------------------------  QUERIES  ---------------------------\n\n");
	//printf("-----------------------------------------------------------------\n");
	
	
	
	QueryResolutionSetup(queryFile, G, LABELS);
	
	GRAPHFree(G);

	return 0;
}