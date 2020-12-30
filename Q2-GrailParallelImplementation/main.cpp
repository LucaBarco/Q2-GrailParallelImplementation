#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <tchar.h>
typedef enum {memory, file} options;
typedef enum { smallD, smallS, large, own } graphOptions;

int main(int argc, char** argv) {
	/*
	if (argc < 3) {
		fprintf(stderr, ("Missing parameters!\nPass the file name in the format inputGraphFile n_labels queryFile\n"));
		return 1;
	}*/
	char* graphInput=NULL;
	options mem_option;
	graphOptions graph_option;
	int choice,labels;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	TCHAR prg[1000] = { _T("querGenerator.exe 10 10 10 query.que") };
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));
	printf("##########################################################################################################################################################\n");
	printf("########################################################## Q2 - GRAIL PARALLEL IMPLEMENTATION ############################################################\n");
	printf("##########################################################################################################################################################\n");
	printf("##################################################################### Developed by #######################################################################\n");
	printf("#############################################  Stefano Bergia   Giuseppina Impagnatiello   Luca Barco  ###################################################\n");
	printf("##########################################################################################################################################################\n\n\n");

	printf("We have developed two possible implementations of the GRAIL algorithm:\n\n \t- In the first one, the graph is entirely loaded in main memory, as an adjacency list.\n\t\t Some memory overloads could happen for big and/or dense graphs, depending on your machine\n\n\t-In the second one, the graph is stored in a binary file, allowing also very big and/or dense graphs.\n\t\tIt may take longer, because of the disk overhead.\n\n");
	do {
		printf("Make your choice: \n\t\t- 0: load the in-memory version \n\t\t- 1: load the in-file version\n");
		scanf_s("%d", &mem_option);

		switch (mem_option) {
		case memory:
			printf("\nOk,let's use memory implementation\n");
			break;
		case file:
			printf("\nOk,let's use file based implementation\n");
			break;
		default:
			printf("\nSorry, I cannot understand what you want to do. Retry\n");
			break;
		}
	} while (mem_option != memory && mem_option != file);

	do{
		printf("\nSelect your input graph file:");
		printf("\n\t0- Small dense benchmark");
		printf("\n\t1- Small sparse benchmark");
		printf("\n\t2- Large benchmark");
		printf("\n\t3- Generate your own file\n");
		scanf_s("%d", &graph_option);

		switch (graph_option) {
			case smallD:
				printf("\nOk,let's use one of small dense real graphs\n");

				do {
					printf("\nSelect your input graph file:");
					printf("\n\t0- arXiv_sub_6000-1.gr");
					printf("\n\t1- citeseer_sub_10720");
					printf("\n\t2- go_sub_6793");
					printf("\n\t3- pubmed_sub_9000-1");
					printf("\n\t4- yago_sub_6642\n");
					scanf_s("%d", &choice);
				
					switch (choice) {
					case 0:
						
						strcpy_s(graphInput,sizeof(graphInput),"arXiv_sub_6000 - 1.gr");
						break;
					case 1:
						strcpy_s(graphInput, sizeof(graphInput),"citeseer_sub_10720");
						break;
					case 2:
						strcpy_s(graphInput, sizeof(graphInput),"go_sub_6793");
						break;
					case 3:
						strcpy_s(graphInput, sizeof(graphInput),"pubmed_sub_9000-1");
						break;
					case 4:
						strcpy_s(graphInput, sizeof(graphInput),"yago_sub_664");
						break;
					default:
						printf("\nSorry, I cannot understand what you want to do. Retry\n");
					}
				} while (choice < 0 && choice>3);
				break;

			case smallS:
				printf("\nOk,let's use one of small sparse real graphs\n");

				do {
					printf("\nSelect your input graph file:");
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
					scanf_s("%d", &choice);
					switch (choice) {
					case 0:
						strcpy_s(graphInput, sizeof(graphInput),"agrocyc_dag_uniq");
						break;
					case 1:
						strcpy_s(graphInput, sizeof(graphInput),"amaze_dag_uniq");
						break;
					case 2:
						strcpy_s(graphInput, sizeof(graphInput),"anthra_dag_uniq");
						break;
					case 3:
						strcpy_s(graphInput, sizeof(graphInput),"ecoo_dag_uniq");
						break;
					case 4:
						strcpy_s(graphInput, sizeof(graphInput),"human_dag_uniq");
						break;
					case 5:
						strcpy_s(graphInput, sizeof(graphInput),"kegg_dag_uniq");
						break;
					case 6:
						strcpy_s(graphInput, sizeof(graphInput),"mtbrv_dag_uniq");
						break;
					case 7:
						strcpy_s(graphInput, sizeof(graphInput),"nasa_dag_uniq");
						break;
					case 8:
						strcpy_s(graphInput, sizeof(graphInput),"vchocyc_dag_uniq");
						break;
					case 9:
						strcpy_s(graphInput, sizeof(graphInput),"xmark_dag_uniq");
						break;
					default:
						printf("\nSorry, I cannot understand what you want to do. Retry\n");
					}
				} while (choice < 0 && choice>9);
				break;
			case large:
				do {
					printf("\nSelect your input graph file:");
					printf("\n\t0- cit-Patents.scc.gra");
					printf("\n\t1- unipretenc_22m.scc.gra");
					scanf_s("%d", &choice);
					switch (choice) {
					case 0:
						strcpy_s(graphInput, sizeof(graphInput),"cit-Patents.scc.gra");
						break;
					case 1:
						strcpy_s(graphInput, sizeof(graphInput),"unipretenc_22m.scc.gra");
						break;
					default:
						printf("\nSorry, I cannot understand what you want to do. Retry\n");
					}
				} while (choice < 0 && choice>2);
				printf("\nOk,let's use  one of large real graphs\n");
				break;

			case own:
				printf("\nOk,let's create a new graph\n");
				//TODO: chiedere numero vertici, archi, query e nome file
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

	printf("\nSelect the number of labels suggested: 3 or 5)\n");
	scanf_s("%d", &labels);




}