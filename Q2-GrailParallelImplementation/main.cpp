#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <tchar.h>
typedef enum {memory, file, end} options;
typedef enum { smallD, smallS, large, own } graphOptions;
struct customGraph {
	int v;
	int e;
	int q;
};
int main(int argc, char** argv) {
	/*
	if (argc < 3) {
		fprintf(stderr, ("Missing parameters!\nPass the file name in the format inputGraphFile n_labels queryFile\n"));
		return 1;
	}*/
	TCHAR name[100] = { _T("myGraph") };
	TCHAR graphInput[10000];
	TCHAR queryInput[10000];
	options mem_option;
	graphOptions graph_option;
	int choice, labels;
	struct customGraph g;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	TCHAR prg[1000];
	TCHAR nameExeGen[50] = { _T("./graphGenerator/graphGenerator.exe") };
	TCHAR nameExeMem[50] = { _T("./Grailv1Mem/Grailv1Mem.exe") };
	TCHAR nameExeFile[50] = { _T("./Grailv1File/Grailv1File.exe") };
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));
	printf("#################################################################################################################\n");
	printf("###################################### Q2 - GRAIL PARALLEL IMPLEMENTATION #######################################\n");
	printf("#################################################################################################################\n");
	printf("################################################# Developed by ##################################################\n");
	printf("#########################  Stefano Bergia   Giuseppina Impagnatiello   Luca Barco  ##############################\n");
	printf("#################################################################################################################\n\n\n");

	printf("We have developed two possible implementations of the GRAIL algorithm:\n\n \t- In the first one, the graph is entirely loaded in main memory, as an adjacency list.\n\t\t Some memory overloads could happen for big and/or dense graphs, depending on your machine\n\n\t-In the second one, the graph is stored in a binary file, allowing also very big and/or dense graphs.\n\t\tIt may take longer, because of the disk overhead.\n\n");
	do {
		swprintf_s(graphInput, _T("myGraph.gra"));
		swprintf_s(queryInput, _T("myGraph.que"));
		do {
			printf("Choose the Grail version: \n\n\t\t- 0: load the in-memory version \n\t\t- 1: load the in-file version\n\t\t- 2: exit\n\n----> Make your choice: ");
			scanf_s("%d", &mem_option);

			switch (mem_option) {
			case memory:
				printf("\nOk,let's use memory implementation\n");
				break;
			case file:
				printf("\nOk,let's use file based implementation\n");
				break;
			case end:
				return 0;
			default:
				printf("\nSorry, I cannot understand what you want to do. Retry\n");
				break;
			}
		} while (mem_option != memory && mem_option != file);

		do {
			printf("\nSelect your input graph file:\n");
			printf("\n\t0- Small dense benchmark");
			printf("\n\t1- Small sparse benchmark");
			printf("\n\t2- Large benchmark");
			printf("\n\t3- Generate your own file\n");
			printf("\n\n----> Make your choice: ");
			scanf_s("%d", &graph_option);

			switch (graph_option) {
			case smallD:
				printf("\nOk,let's use one of small dense real graphs\n");

				do {
					printf("\nSelect your input graph file:\n");
					printf("\n\t0- arXiv_sub_6000-1");
					printf("\n\t1- citeseer_sub_10720");
					printf("\n\t2- go_sub_6793");
					printf("\n\t3- pubmed_sub_9000-1");
					printf("\n\t4- yago_sub_6642\n");
					printf("\n\n----> Make your choice: ");
					scanf_s("%d", &choice);

					switch (choice) {
					case 0:
						swprintf_s(graphInput, _T("./benchmark/small_dense_real/arXiv_sub_6000-1.gra"));
						swprintf_s(queryInput, _T("./benchmark/small_dense_real/arXiv_sub_6000-1.que"));
						break;
					case 1:
						swprintf_s(graphInput, _T("./benchmark/small_dense_real/citeseer_sub_10720.gra"));
						swprintf_s(queryInput, _T("./benchmark/small_dense_real/citeseer_sub_10720.que"));
						break;
					case 2:
						swprintf_s(graphInput, _T("./benchmark/small_dense_real/go_sub_6793.gra"));
						swprintf_s(queryInput, _T("./benchmark/small_dense_real/go_sub_6793.que"));
						break;
					case 3:
						swprintf_s(graphInput, _T("./benchmark/small_dense_real/pubmed_sub_9000-1.gra"));
						swprintf_s(queryInput, _T("./benchmark/small_dense_real/pubmed_sub_9000-1.que"));
						break;
					case 4:
						swprintf_s(graphInput, _T("./benchmark/small_dense_real/yago_sub_6642.gra"));
						swprintf_s(queryInput, _T("./benchmark/small_dense_real/yago_sub_6642.que"));
						break;
					default:
						printf("\nSorry, I cannot understand what you want to do. Retry\n");
					}
				} while (choice < 0 && choice>3);
				break;

			case smallS:
				printf("\nOk,let's use one of small sparse real graphs\n");

				do {
					printf("\nSelect your input graph file:\n");
					printf("\n\t0- agrocyc_dag_uniq");
					printf("\n\t1- amaze_dag_uniq");
					printf("\n\t2- anthra_dag_uniq");
					printf("\n\t3- ecoo_dag_uniq");
					printf("\n\t4- human_dag_uniq");
					printf("\n\t5- kegg_dag_uniq");
					printf("\n\t6- mtbrv_dag_uniq");
					printf("\n\t7- nasa_dag_uniq");
					printf("\n\t8- vchocyc_dag_uniq");
					printf("\n\t9- xmark_dag_uniq\n");
					printf("\n\n----> Make your choice: ");
					scanf_s("%d", &choice);
					switch (choice) {
					case 0:
						swprintf_s(graphInput, _T("./benchmark/small_sparse_real/agrocyc_dag_uniq.gra"));
						swprintf_s(queryInput, _T("./benchmark/small_sparse_real/agrocyc_dag_uniq.que"));
						break;
					case 1:
						swprintf_s(graphInput, _T("./benchmark/small_sparse_real/amaze_dag_uniq.gra"));
						swprintf_s(queryInput, _T("./benchmark/small_sparse_real/amaze_dag_uniq.que"));
						break;
					case 2:
						swprintf_s(graphInput, _T("./benchmark/small_sparse_real/anthra_dag_uniq.gra"));
						swprintf_s(queryInput, _T("./benchmark/small_sparse_real/anthra_dag_uniq.que"));
						break;
					case 3:
						swprintf_s(graphInput, _T("./benchmark/small_sparse_real/ecoo_dag_uniq.gra"));
						swprintf_s(queryInput, _T("./benchmark/small_sparse_real/ecoo_dag_uniq.que"));
						break;
					case 4:
						swprintf_s(graphInput, _T("./benchmark/small_sparse_real/human_dag_uniq.gra"));
						swprintf_s(queryInput, _T("./benchmark/small_sparse_real/human_dag_uniq.que"));
						break;
					case 5:
						swprintf_s(graphInput, _T("./benchmark/small_sparse_real/kegg_dag_uniq.gra"));
						swprintf_s(queryInput, _T("./benchmark/small_sparse_real/kegg_dag_uniq.que"));
						break;
					case 6:
						swprintf_s(graphInput, _T("./benchmark/small_sparse_real/mtbrv_dag_uniq.gra"));
						swprintf_s(queryInput, _T("./benchmark/small_sparse_real/mtbrv_dag_uniq.que"));
						break;
					case 7:
						swprintf_s(graphInput, _T("./benchmark/small_sparse_real/nasa_dag_uniq.gra"));
						swprintf_s(queryInput, _T("./benchmark/small_sparse_real/nasa_dag_uniq.que"));
						break;
					case 8:
						swprintf_s(graphInput, _T("./benchmark/small_sparse_real/vchocyc_dag_uniq.gra"));
						swprintf_s(queryInput, _T("./benchmark/small_sparse_real/vchocyc_dag_uniq.que"));
						break;
					case 9:
						swprintf_s(graphInput, _T("./benchmark/small_sparse_real/xmark_dag_uniq.gra"));
						swprintf_s(queryInput, _T("./benchmark/small_sparse_real/xmark_dag_uniq.que"));
						break;
					default:
						printf("\nSorry, I cannot understand what you want to do. Retry\n");
					}
				} while (choice < 0 && choice>9);
				break;
			case large:
				do {
					printf("\nOk,let's use  one of large real graphs\n");
					printf("\nSelect your input graph file:\n");
					printf("\n\t0- unipretenc_22m.scc.gra");
					printf("\n\t1- cit-Patents.scc.gra");
					printf("\n\n----> Make your choice: ");
					scanf_s("%d", &choice);
					switch (choice) {

					case 0:
						swprintf_s(graphInput, _T("./benchmark/large_real/unipretenc_22m.scc.gra"));
						swprintf_s(queryInput, _T("./benchmark/large_real/unipretenc_22m.scc.que"));
						break;
					
						
					case 1:
						swprintf_s(graphInput, _T("./benchmark/large_real/cit-Patents.scc.gra"));
						swprintf_s(queryInput, _T("./benchmark/large_real/cit-Patents.scc.que"));
						break;
						
					
					default:
						printf("\nSorry, I cannot understand what you want to do. Retry\n");
					}
				} while (choice < 0 && choice>2);

				break;

			case own:
				printf("\nOk,let's create a new graph\n");
				do {
					printf("\n---> Insert number of vertexes: ");
					scanf_s("%d", &g.v);
					if (g.v < 0) {
						printf("Please, insert a positive value\n");
					}
				} while (g.v < 0);

				do {
					printf("\n---> Insert maximum number of edges: ");
					scanf_s("%d", &g.e);
					if (g.e < 0) {
						printf("Please, insert a positive value");
					}
				} while (g.e < 0);

				do {
					printf("\n---> Insert number of queries: ");
					scanf_s("%d", &g.q);
					if (g.q < 0) {
						printf("Please, insert a positive value\n");
					}
				} while (g.q < 0);


				swprintf_s(prg, _T("%s %d %d %d %s"), nameExeGen, g.v, g.e, g.q, name);

				CreateProcess(NULL, prg, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
				// Wait until child process exits.
				WaitForSingleObject(pi.hProcess, INFINITE);

				// Close process and thread handles. 
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);

				break;

			default:
				printf("\nSorry, I cannot understand what you want to do. Retry\n");
				break;
			}
		} while (graph_option != smallD && graph_option != smallS && graph_option != large && graph_option != own);

		do {
			printf("\nSelect the number of labels (suggested: 3 or 5)\n");
			scanf_s("%d", &labels);
			if (labels < 0) {
				printf("Please, insert a positive value\n");
			}
		} while (labels < 0);

		if (mem_option == memory)
			swprintf_s(prg, _T("%s %s %d %s"), nameExeMem, graphInput, labels, queryInput);
		else
			swprintf_s(prg, _T("%s %s %d %s"), nameExeFile, graphInput, labels, queryInput);
		
		wprintf_s(_T("%s\n"), prg);
		
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&pi, sizeof(pi));


		CreateProcess(NULL, prg, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
		// Wait until child process exits.
		WaitForSingleObject(pi.hProcess, INFINITE);

		// Close process and thread handles. 
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);


	}while (mem_option!=end);
}