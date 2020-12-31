#include <tchar.h>
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctime>
#include "psapi.h"

#define BUFFER 100			//maximum dimension for a read buffer
#define MINIBUFFER 10		//maximum dimension for a node string
#define NLABEL 5			//maximum number of label
#define QUERYBUFFER 20		//number of query each thread store in the buffer

/*
	THIS PROGRAM READS A GRAPH FILE IN THE FORMAT
		NODE: CHILDREN#
	AND A QYERY LIST IN THE FORMAT
		U V
	AND CREATES:
	1- A NEW BINARY FILE WITH THE OFFSET OF THE ROW STARTS WITH (row i contains the offset of node i) AND THE RELATIVE LABELS
	2- A TXT FILE WITH THE REACHABILITY QUERIES SOLVED
*/



//comments for the functions can be founf above the function itself
DWORD checkIndexFile(DWORD nodes, int nlabel, BOOL showedge);
DWORD WINAPI parallelRead(LPVOID lpParam);
struct output_record* readIndex(int node_index, HANDLE lk);
void writeIndex(struct output_record* new_node, HANDLE lk);
void printRoots(FILE* fp);
void setBitMap(int index, DWORD* bitmapArray);
BOOL checkBitMap(int index, DWORD* bitmapArray);
void RandomizedLabelling(int d);
DWORD WINAPI singleIndexRandomizedLabelling(LPVOID params);
int  RandomizedVisit(int node_index, struct tread_t_label* d);
DWORD getBits(BOOL ones);
int getChildrenLock(int node_index, int child_index, int my_pos, int d, HANDLE index_file);
int getChildren(int node_index, int child_index, HANDLE hIn);
void lockIndex(int node_index, int my_pos, int d);
void unlockIndex(int node_index, int my_pos);
int searchSem(DWORD my_id, int node_index, int len);
void shuffle(int* v, int n, int seed);
int* Randomize(int n, int seed);
BOOL Reachable(int u, int  v, int nLabels);
BOOL RecursiveReachable(struct reach* data);
VOID generateIndex(int n_thread);
void ResolveQueries(LPVOID params);
VOID QueryResolutionSetup(LPTSTR queryFile, int n_thread);


//used in the generateIndex, tells each thread which portion of the file to read 
struct tread_t {
	DWORD index;
	LARGE_INTEGER offset;
	FILE* fp;
};

//contains the parameters fot threads that operate in randomizedLabelling
struct tread_t_label {
	DWORD label_index;
	DWORD tot_label;
	DWORD* visited;
	DWORD r;
	HANDLE index_file;
};

//represents a label
struct label {
	DWORD low;
	DWORD high;
};

//represent a record in the index file
struct output_record {
	DWORD node_index;
	LARGE_INTEGER offset;
	DWORD size;
	DWORD nChildren;
	struct label labels[NLABEL];
};

//used to synchronize threads in randomizedlabelling
struct file_sync {
	DWORD node_index;
	HANDLE sem;
	DWORD queue_count;
};

//used to solve reachability queries
struct reach {
	struct output_record* u;
	struct output_record* v;
	DWORD nLabels;
	DWORD* visited;
};


//stings wit the file names and file pointers
LPTSTR  graphFile, queryFile;
FILE* queryOutput;
FILE* labelFp;
HANDLE hOut, hIndex;
TCHAR indexFile[30];
TCHAR queryOutFile[30];
TCHAR outputLog[30];

//mutextes
HANDLE bitmapMutex;					// used in parallelRead to access roots bitmap
HANDLE indexMutex;					// used in paralleread to acces file sunchronously
HANDLE graphMutex;					// used in getChildrenLock graph file sunchronously
HANDLE percMutex;					// used in RandomizedVisit value for the progress bar
HANDLE bufferMutex;					// used in ResolveQueries to access the query file
HANDLE queryStatsMutex;				// used in ResolveQueriesto to update the number of solved queries
HANDLE sync_mutex;					// used in lockindex and unlockindex to access struct sync sunchronously

struct file_sync* sync;						// used to synchronize threads generating the label
int rootBitmapDim;							// size of the  bitMap containin the roots
int labelProgressbar = 0, LabelPerc = 0;	// used for label's progressbar
int indexProgress = 0,indexPerc = 0;		//used for index progressbar
int logProgress = 0, logPerc = 0;		//used for index progressbar
int queryCount = 0;							//used for reachability queries progress bar

int batch_sz;						//used in generateIndexto determine the size of the batch to read for each thread
int queryTimeCount;
double queryTimeSum;				//used to compute query execution time statistics
double nL;							//contains the number of labels

DWORD nNodes;						//contains the number of node for the graph
DWORD* rootsBitmap;						//is a bitmap with one bit for each node that indicates if that node is a root or no


int _tmain(int argc, LPTSTR argv[]) {

	FILE* fp;
	int nLabels = 0;				//number of labels for each node
	int nRoots = 0;					//number of roots
	int tmp = 0;

	int i = 0;
	if (argc < 3) {
		fprintf(stderr, ("Missing parameters!\nPass the file name in the format inputGraphFile outputIndexFile\n"));
		return 0;
	}

	graphFile = argv[1];			//name for the file containing the graph
	nLabels = _wtoi(argv[2]);		//number of label to use (MAX 5)
	queryFile = argv[3];			//name for the txt file that contains the query


	wcscpy_s(indexFile,graphFile);			//name of the binary file that will contain the index
	tmp = wcslen(indexFile);
	indexFile[tmp] = '.';
	indexFile[tmp+1] = 'i';
	indexFile[tmp+2] = 'd';
	indexFile[tmp+3] = '\0';

	wcscpy_s(queryOutFile, _T("queryResults.txt")); 			//name for the file that will contain the query results
	wcscpy_s(outputLog, _T("Labels.txt")); 						//name for the file that will contain the output log
	nL = nLabels;

	_wfopen_s(&labelFp, outputLog, _T("w"));
	bitmapMutex = CreateMutex(NULL, FALSE, NULL);
	indexMutex = CreateMutex(NULL, FALSE, NULL);
	nNodes = 0;

	//printf("We're working for you..... ");
	//printf("-----------------------------------------------------------------\n");
	printf("--------------------- INDEX FILE GENERATION -----------------------\n\n");
	//printf("-----------------------------------------------------------------\n");
	printf("---> Reading the graph ad generating the index file...");
	clock_t begin_index = clock();
	generateIndex(2);					//scan a first time the graph file to generate the index file with sizes and offsets, as well as calculating roots
	clock_t end_index = clock();
	CloseHandle(bitmapMutex);
	CloseHandle(indexMutex);
	fprintf(stdout, "Index File Created\n");
	printRoots(labelFp);					//print the roots
	nRoots = getBits(FALSE);
	fprintf(stdout, ("---> Index Creation Time: %f s\n\n"), (double)(end_index - begin_index) / CLOCKS_PER_SEC);

	//printf("-----------------------------------------------------------------\n");
	printf("---------------------------   ROOTS   ---------------------------\n\n");
	//printf("-----------------------------------------------------------------\n");
	printf("---> Number of Roots: %d\n\n", nRoots);	//print the number of roots
	
	//printf("-----------------------------------------------------------------\n");
	printf("---------------------------  LABELS   ---------------------------\n\n");
	//printf("-----------------------------------------------------------------\n");
	 printf("---> Creating labels... ");
	percMutex = CreateMutex(NULL, FALSE, NULL);
	clock_t begin_labelling = clock();
	RandomizedLabelling(nLabels);		//generate the labels
	clock_t end_labelling = clock();
	printf("Done!\n");
	CloseHandle(percMutex);
	clock_t begin_query, end_query;

	 wprintf_s(_T("---> Printing labels on file %s..."),outputLog);
	checkIndexFile(nNodes, nLabels, FALSE);	//print the index file with the labels
	printf("Done!\n");

	fclose(labelFp);
	fprintf(stdout, ("---> Label Creation Time: %fs\n\n"), (double)(end_labelling - begin_labelling) / CLOCKS_PER_SEC);
	//printf("-----------------------------------------------------------------\n");
	printf("---------------------------  QUERIES  ---------------------------\n\n");
	//printf("-----------------------------------------------------------------\n");
	fprintf(stdout, "\n---> Executing reachability queries ...\n");
	graphMutex = CreateMutex(NULL, FALSE, NULL);
	_wfopen_s(&queryOutput, queryOutFile, _T("w"));
	begin_query = clock();
	QueryResolutionSetup(queryFile, 2);				//solve reachability queries
	end_query = clock();
	fprintf(stdout, ("---> Actual time: %fs\n"), (double)(end_query - begin_query) / CLOCKS_PER_SEC);
	fclose(queryOutput);
	CloseHandle(graphMutex);

	//CONFRONTO TRA THREAD NON CANCELLARE
	/*printf("1 thread\n");
	begin_query = clock();
	QueryResolutionSetup(queryFile, 1);
	end_query = clock();
	fprintf(stdout, ("Actual time: %f\n"), (double)(end_query - begin_query) / CLOCKS_PER_SEC);
	begin_query = clock();
	printf("2\n");
	QueryResolutionSetup(queryFile, 2);
	end_query = clock();
	fprintf(stdout, ("Actual time: %f\n"), (double)(end_query - begin_query) / CLOCKS_PER_SEC);
	begin_query = clock();
	printf("3\n");
	QueryResolutionSetup(queryFile, 3);
	end_query = clock();
	fprintf(stdout, ("Actual time: %f\n"), (double)(end_query - begin_query) / CLOCKS_PER_SEC);
	begin_query = clock();
	printf("4\n");
	QueryResolutionSetup(queryFile, 4);
	end_query = clock();
	fprintf(stdout, ("Actual time: %f\n"), (double)(end_query - begin_query) / CLOCKS_PER_SEC);

	begin_query = clock();
	printf("5\n");
	QueryResolutionSetup(queryFile, 5);
	end_query = clock();
	fprintf(stdout, ("Actual time: %f\n"), (double)(end_query - begin_query) / CLOCKS_PER_SEC);

	begin_query = clock();
	printf("6\n");
	QueryResolutionSetup(queryFile, 6);
	end_query = clock();
	fprintf(stdout, ("Actual time: %f\n"), (double)(end_query - begin_query) / CLOCKS_PER_SEC);

	begin_query = clock();
	printf("7\n");
	QueryResolutionSetup(queryFile, 7);
	end_query = clock();
	fprintf(stdout, ("Actual time: %f\n"), (double)(end_query - begin_query) / CLOCKS_PER_SEC);

	begin_query = clock();
	printf("8\n");
	QueryResolutionSetup(queryFile, 8);
	end_query = clock();
	fprintf(stdout, ("Actual time: %f\n"), (double)(end_query - begin_query) / CLOCKS_PER_SEC);*/

	return 0;
}


//prints the index file with the labels (params: number of nodes, number of labels, flag to show also the children of each node)
DWORD checkIndexFile(DWORD nodes, int nLabel, BOOL showedge) {


	HANDLE hIn = CreateFile(indexFile, GENERIC_READ, 0, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	FILE* fCheck;
	_wfopen_s(&fCheck, graphFile, _T("r"));

	char* buffer = NULL;			//buffer to read from file
	struct output_record node;		//record read from the index file
	int i, k = 0,r,z;

	FILE* fp;
	fp = labelFp;

	if (hIn == INVALID_HANDLE_VALUE) {
		fprintf(stderr, ("Error opening indexes file!\n"));
		return -1;
	}
	fprintf(fp, "\n");
	for (i = 0; i < nodes; i++) {	//read node by node
		ReadFile(hIn, &node, sizeof(struct output_record), NULL, NULL);
		fprintf(fp, ("%d:\tOffset: %llu  Size:%d  Children:%d\t"), node.node_index, node.offset, node.size, node.nChildren);
		for (k = 0; k < nLabel; k++) {
			fprintf(fp, ("[%d,%d]"), node.labels[k].low, node.labels[k].high);
		}
		fprintf(fp, "\n");
		labelProgressbar++;
		if (LabelPerc != (labelProgressbar / nNodes)) {
			LabelPerc = (labelProgressbar / nNodes);
			r = printf("%d%%", LabelPerc);
			for (z = 0; z < r; z++)
				printf("\b");
		}
	

		//if showedhe is TRUE i use the offset and the size contained in the record i just read to read the children of the node in the grapf file
		if (showedge) {
			if (node.size > 0) {
				buffer = (char*)malloc((node.size + 1) * sizeof(char));
				_fseeki64(fCheck, node.offset.QuadPart + 1, SEEK_SET);

				//buffer allocation to read edges
				if (fread(buffer, sizeof(char), node.size, fCheck) > 0) {	//read the current node with the overlapped structure
					buffer[node.size] = ('\0');								//set string terminator
					fprintf(fp, ("\t\tEdges: %s\n"), buffer);
				}
				free(buffer);
			}
			else
				fprintf(fp, ("\t\tEdges:\n"));
		}

	}
	CloseHandle(hIn);
	fclose(fCheck);
	return 0;
}

//scan the graph file to compute and offset and a size fo each node in order to create the index file
//params: number of thread that will read the file
VOID generateIndex(int n_thread) {
	FILE* fIn;
	DWORD sz;
	HANDLE* threadH;				//thread handles
	struct tread_t* tOffsets;		//parameters to pass to the vector
	DWORD* tId;						//thread ids
	int i = 0;

	_wfopen_s(&fIn, graphFile, _T("r"));
	hOut = CreateFile(indexFile, GENERIC_WRITE, 0, NULL,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (fIn == NULL || hOut == INVALID_HANDLE_VALUE ) {
		fprintf(stderr, ("Error opening files!\n"));
		return;
	}

	fscanf_s(fIn, ("%ld\n"), &nNodes);										//i read the number of nodes

	rootBitmapDim = nNodes / sizeof(DWORD) + 1;								//compute  the size of the bitmap
	rootsBitmap = (DWORD*)malloc(rootBitmapDim * sizeof(DWORD));			//allocate the bitmap
	fseek(fIn, 0L, SEEK_END);
	sz = ftell(fIn);														//retrieve File dimension
	batch_sz = sz / n_thread;												//dimension of the batch to read for each thread
	fseek(fIn, 0, SEEK_SET);												//place file pointer in the beginning of the file

	threadH = (HANDLE*)malloc(n_thread * sizeof(HANDLE));					//Threads initialization
	tOffsets = (struct tread_t*)malloc(n_thread * sizeof(struct tread_t));	
	tId = (DWORD*)malloc(n_thread * sizeof(DWORD));

	for (i = 0; i < n_thread; i++) {
		tId[i] = i;
		tOffsets[i].index = i;
		tOffsets[i].offset.QuadPart = (LONGLONG)i * batch_sz;				//set the offset in the file from th thread must start to read
		tOffsets[i].fp = fIn;
		threadH[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)parallelRead, &tOffsets[i], 0, &tId[i]);
		if (threadH[i] == NULL)
			ExitProcess(1);
	}
	WaitForMultipleObjects(n_thread, threadH, TRUE, INFINITE);
	for (i = 0; i < n_thread; i++)
		CloseHandle(threadH[i]);

	fclose(fIn);
	free(threadH);
	free(tId);
	CloseHandle(hOut);
}

//reads a batch of the graph file and generates the corresponfig indexes
DWORD WINAPI parallelRead(LPVOID lpParam) {

	int offset = 0;
	struct tread_t* data = (struct tread_t*)lpParam;
	LARGE_INTEGER relative_offset, start_offset;		//relative offset: distance from the beginning of the block 
														//start_offset:offset for thr beginning of the edge portion for a node
	relative_offset.QuadPart = 0;
	start_offset.QuadPart = 0;

	int flag = 1;										//used to individuate the node index in a row
	int read_sharp = 0;									//used to signal when the '#' at the end of the node is read
	int j = 0, i = 0, k = 0, z = 0, r = 0, index;		

	struct output_record out;							//struct to save index record
	for (k = 0; k < NLABEL; k++) {						//setup starting vaue for the labels
		out.labels[k].low = nNodes + 1;
		out.labels[k].high = 0;
	}
	out.nChildren = 0;

	for (k = 0; k < rootBitmapDim; k++)					//initialize the bitmap (0 is root, 1 is not root)
		rootsBitmap[k] = 0;
	k = 0;

	char buffer[BUFFER];								//row read from the graph file
	char minibuffer[MINIBUFFER];						//index of the node
	char  child[MINIBUFFER];							//index of the child

	FILE* fp = data->fp;
	LARGE_INTEGER FilePos;
	OVERLAPPED ov = { 0,0,0,0,NULL };

	WaitForSingleObject(indexMutex, INFINITE);			//mutual exclusion to access file pointer
	fseek(fp, data->offset.QuadPart, SEEK_SET);			//i move the file pointer to the portion associated with the current thread
	fgets(buffer, BUFFER, fp);							//read and dispose of the first row (it will be read by the previous thred and it contains the number of nodes for thread zero
	offset = ftell(fp);									//i save the offset
	ReleaseMutex(indexMutex);

	relative_offset.QuadPart += strlen(buffer);			//i save the number of char read

	for (z = 0; z < MINIBUFFER; z++)					//reset the child buffer
		child[z] = '\0';

	WaitForSingleObject(indexMutex, INFINITE);					//mutual exclusion in the file
	fseek(fp, offset, SEEK_SET);								//place the file pointer in the right offset
		//loop until the file is over or i have read the first '#' (read_sharp) past the batch_size
	while (fgets(buffer, BUFFER, fp) != NULL && ((relative_offset.QuadPart < batch_sz) || read_sharp == 0)) {
		offset = ftell(fp);										//save the new offset after reading the row
		ReleaseMutex(indexMutex);
		j = 0;
		for (i = 0; i < BUFFER && i < strlen(buffer); i++) {		//cycle over each char of the buffer
			if (iswdigit(buffer[i]) && flag == 1) {					//check if i'm reading a char from the node index
				minibuffer[j] = buffer[i];							//add the char in the minibuffer
				j++;
			}
			else if (buffer[i] == (':')) {								//portion reserved to edges
				start_offset.QuadPart = relative_offset.QuadPart + 1;	//save the offset
				minibuffer[j] = '\0';
				flag = 0;												//reset flag to read childs
				j = 0;
			}
			else if (buffer[i] == ('#') && flag == 0) {										//'#' reached
				out.node_index = atoi(minibuffer);											//convert minibuffer in integer
				out.offset.QuadPart = data->offset.QuadPart + (start_offset.QuadPart);		//set the starting offset
				out.size = (relative_offset.QuadPart - start_offset.QuadPart);				//set edges size

				FilePos.QuadPart = out.node_index * (LONGLONG)sizeof(struct output_record);	//set overlap structure to write on the correct portion of the index file
				ov.Offset = FilePos.LowPart;
				ov.OffsetHigh = FilePos.HighPart;
				WriteFile(hOut, &out, sizeof(struct output_record), NULL, &ov);				//write the record
				out.nChildren = 0;
				flag = 1;
				if (relative_offset.QuadPart >= batch_sz)		//seif i'm past the batch size and i have read the '#' i must terminate the cycle
					read_sharp = 1;
				relative_offset.QuadPart++;						//invrement realtive offset to account for '\n'

				WaitForSingleObject(bitmapMutex, INFINITE);
				indexProgress++;								//update progressbar
				ReleaseMutex(bitmapMutex);
			}
			else if (iswdigit(buffer[i]) && flag == 0) {		//save child buffer
				child[k] = buffer[i];
				k++;
			}
			else if (buffer[i] == (' ') && flag == 0 && k > 0) {

				child[k] = '\0';
				k = 0;
				index = atoi(child);							//save the child index

				out.nChildren++;

				WaitForSingleObject(bitmapMutex, INFINITE);								//print progressbar
				if (indexPerc != ((index - index) + indexProgress) * 100 / nNodes) {
				int indexPerc = ((index - index) + indexProgress) * 100 / nNodes;
				r = printf("%d%%", indexPerc);
				for (z = 0; z < r; z++)
					printf("\b");
				}
				setBitMap(index, rootsBitmap);											//update roots bitmap (the child i found cannot be a root)
				ReleaseMutex(bitmapMutex);


			}

			relative_offset.QuadPart++;		//incremento offset relativo

		}
		WaitForSingleObject(indexMutex, INFINITE);
		fseek(fp, offset, SEEK_SET);		//set thr correct offset
	}
	ReleaseMutex(indexMutex);
	ExitThread(0);
}


//printf the roots for the graph
void printRoots(FILE* fp) {
	int i,j=0;
	for (i = 0; i <= nNodes; i++) {
		if (!checkBitMap(i , rootsBitmap)) {
			fprintf(fp, "%d ", i );
			j++;
			if (j == 20) {
				j = 0;
				fprintf(fp, "\n");
			}
		}
		
	}
}

//returns the nomber of bits equa to the parameters in the bitmap (with 0 number of roots)
DWORD getBits(BOOL ones) {
	int i;
	DWORD count = 0;
	for (i = 0; i < nNodes; i++) {
		if (checkBitMap(i, rootsBitmap) == ones)
			count++;
	}
	return count;
}

//used in the lockindex, it returns the index (relative to the struct sync) of the thread that is working on the node "node_index"
//it return -1 if that node is free or it is occupied by the thread itself (my_id)
int searchSem(DWORD my_id, int node_index, int len) {

	int i = 0;
	for (i = 0; i < len; i++) {
		if (i != my_id) {
			if (sync[i].node_index == node_index)
				return i;
		}
	}
	return -1;
}

//it locks a portion of the index file corresponding to the node: node_index
void lockIndex(int node_index, int my_pos, int d) {

	int pos;
	do {
		WaitForSingleObject(sync_mutex, INFINITE);				//get exclusive access to the ssync vector
		pos = searchSem(my_pos, node_index, d);					//check if the node is already locked
		if (pos < 0) {											//if the node is free
			sync[my_pos].node_index = node_index;				// the current thread locks it
			ReleaseMutex(sync_mutex);
			break;												//break the cycle
		}
		else {
			sync[pos].queue_count++;							//if the node is locked i update the number of threads waiting on it
			ReleaseMutex(sync_mutex);							
			WaitForSingleObject(sync[pos].sem, INFINITE);		//i wait on the semaphore of the thread lockig the node
		}
	} while (TRUE);												//stay in this function as long as the thread doesn't get the lock
}

//release a previously locked index
void unlockIndex(int node_index, int my_pos) {

	WaitForSingleObject(sync_mutex, INFINITE);
	sync[my_pos].node_index = -1;										//release the node from the structure
	ReleaseSemaphore(sync[my_pos].sem, sync[my_pos].queue_count, NULL);	//release semaphore for an amount equal to the thread waiting on the semaphore
	sync[my_pos].queue_count = 0;										//reset the waiting count
	ReleaseMutex(sync_mutex);

}

//reads a record from the index file using the overlapped data structure
struct output_record* readIndex(int node_index, HANDLE lk) {
	HANDLE hIn;
	if (lk != NULL)
		hIn = lk;
	else {
		do {
			hIn = CreateFile(indexFile, GENERIC_READ, 0, NULL,
				OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		} while (hIn == INVALID_HANDLE_VALUE || hIn == NULL);
	}

	LARGE_INTEGER FilePos, FileReserved;
	OVERLAPPED ov = { 0,0,0,0,NULL };

	struct output_record* node = (struct output_record*)malloc(sizeof(struct output_record));

	FilePos.QuadPart = node_index * (LONGLONG)sizeof(struct output_record);	//imposto struct overlapped per andare a leggere nella porzione corretta del file di indice
	ov.Offset = FilePos.LowPart;
	ov.OffsetHigh = FilePos.HighPart;
	FileReserved.QuadPart = sizeof(struct output_record);

	ReadFile(hIn, node, sizeof(struct output_record), NULL, &ov);

	if (lk == NULL)
		CloseHandle(hIn);

	return node;
}

//writes a record from the index file using the overlapped data structure
void writeIndex(struct output_record* new_node, HANDLE lk) {
	HANDLE hIn;
	if (lk != NULL)
		hIn = lk;
	else
		hIn = CreateFile(indexFile, GENERIC_WRITE, 0, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	LARGE_INTEGER FilePos, FileReserved;
	OVERLAPPED ov = { 0,0,0,0,NULL };

	FilePos.QuadPart = new_node->node_index * (LONGLONG)sizeof(struct output_record);	//imposto struct overlapped per andare a scrivere nella porzione corretta del file di indice
	ov.Offset = FilePos.LowPart;
	ov.OffsetHigh = FilePos.HighPart;
	FileReserved.QuadPart = sizeof(struct output_record);

	WriteFile(hIn, new_node, sizeof(struct output_record), NULL, &ov);

	if (lk == NULL)
		CloseHandle(hIn);

}


//reads the chilfren "child_index" of the node "node_index" in a synchronous way calling lockindex 
//if child_index is equal to -1 returns the child number
int getChildrenLock(int node_index, int child_index, int my_pos, int d, HANDLE index_file) {
	FILE* fCheck;

	struct output_record* node = NULL;
	char* buffer;
	char* token;
	char* next_token;
	int i = 0;

	lockIndex(node_index, my_pos, d);
	node = readIndex(node_index, index_file);
	unlockIndex(node_index, my_pos);

	if (child_index == -1)
		return node->nChildren;

	if (node->size > 0) {
		buffer = (char*)malloc((node->size + 1) * sizeof(char));
		WaitForSingleObject(graphMutex, INFINITE);
		_wfopen_s(&fCheck, graphFile, _T("r"));
		_fseeki64(fCheck, node->offset.QuadPart + 1, SEEK_SET);
		if (fread(buffer, sizeof(char), node->size, fCheck) > 0) {	//read the current node with the overlapped structure
			buffer[node->size] = ('\0');							//set string terminator
		}

		fclose(fCheck);
		ReleaseMutex(graphMutex);

		token = strtok_s(buffer, " ", &(next_token));
		while (i < child_index && token != NULL) {
			i++;
			token = strtok_s(NULL, " ", &(next_token));
		}

		DWORD child_value = atoi(token);
		free(buffer);
		free(node);
		return child_value;

	}
	free(node);
	return -1;
}

//same as getchildrenlock but without te call to lock and unlock
int getChildren(int node_index, int child_index, HANDLE hIn) {
	FILE* fCheck;
	_wfopen_s(&fCheck, graphFile, _T("r"));
	struct output_record* node;
	char* buffer;
	char* token;
	char* next_token;
	int i = 0;

	int n = 0;
	node = readIndex(node_index, hIn);

	if (child_index == -1)
		return node->nChildren;

	if (node->size > 0) {
		buffer = (char*)malloc((node->size + 1) * sizeof(char));
		_fseeki64(fCheck, node->offset.QuadPart + 1, SEEK_SET);
		//buffer allocation to read 
		if (fread(buffer, sizeof(char), node->size, fCheck) > 0) {	//read the current node with the overlapped structure
			buffer[node->size] = ('\0');							//set string terminator
		}

		token = strtok_s(buffer, " ", &(next_token));
		while (i < child_index && token != NULL) {
			i++;
			token = strtok_s(NULL, " ", &(next_token));
		}
		DWORD child_value = atoi(token);
		free(buffer);
		free(node);
		fclose(fCheck);
		return child_value;

	}
	free(node);
	fclose(fCheck);
	return -1;
}

//sets a bit in the bitmap to 1
void setBitMap(int index, DWORD* bitmapArray) {
	DWORD word_offset = index / 32;
	DWORD bit_offset = index % 32;
	bitmapArray[word_offset] |= (1 << bit_offset);
}

//returns the value of the index-th bit in the bitmal 
BOOL checkBitMap(int index, DWORD* bitmapArray) {
	DWORD word_offset = index / 32;
	DWORD bit_offset = index % 32;
	if ((bitmapArray[word_offset] & (1 << bit_offset)) == 0)
		return FALSE;
	else
		return TRUE;
}

//it assign d label to each node according to GRAIL algorithm
void RandomizedLabelling(int d) {
	int i, j;
	HANDLE* threadH = (HANDLE*)malloc(d * sizeof(HANDLE));
	DWORD* tId = (DWORD*)malloc(d * sizeof(HANDLE));
	struct tread_t_label* params = (struct tread_t_label*)malloc(d * sizeof(struct tread_t_label));
	sync = (struct file_sync*)malloc(d * sizeof(struct file_sync));

	HANDLE hIn = CreateFile(indexFile,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	sync_mutex = CreateMutex(NULL, FALSE, NULL);
	for (i = 0; i < d; i++) {
		sync[i].node_index = -1;
		sync[i].sem = CreateSemaphore(NULL, 0, d, NULL);
		sync[i].queue_count = 0;
	}

	//for each label i start a thread that assign the i-th label for each node
	for (i = 0; i < d; i++) {
		params[i].label_index = i;
		params[i].tot_label = d;
		params[i].visited = (DWORD*)malloc(rootBitmapDim * sizeof(DWORD));			//visited bitmap for each thread
		params[i].index_file = hIn;
		for (j = 0; j < rootBitmapDim; j++)
			params[i].visited[j] = 0;
		params[i].r = 1;	// see r value in the GRAIL pseudocode
		threadH[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)singleIndexRandomizedLabelling, &params[i], 0, &tId[i]);
	}

	WaitForMultipleObjects(d, threadH, TRUE, INFINITE);

	for (i = 0; i < d; i++) {
		free(params[i].visited);
		CloseHandle(threadH[i]);
	}

	CloseHandle(hIn);
	free(sync);
	free(threadH);
	free(tId);
	free(params);
	CloseHandle(sync_mutex);
}


//calls Randomized Visit on each root of the graph in a random order
DWORD WINAPI singleIndexRandomizedLabelling(LPVOID params) {
	struct tread_t_label* d = (struct tread_t_label*)params;
	int i;
	int* order = Randomize(nNodes, 0);		//generates a vector of nNodes elements in a random order
	for (i = 0; i < nNodes; i++) {
		if (!checkBitMap(order[i], rootsBitmap)) {
			RandomizedVisit(order[i], d);
		}
	}
	free(order);
	ExitThread(0);
}

//recursive visit on the graph to assign the label according to GRAIL alghoritm
int  RandomizedVisit(int node_index, struct tread_t_label* d) {
	int nChildren, curChild, minLower = nNodes, tmp, lowlab,j,r,z;
	struct output_record* index_obj;
	HANDLE file;

	//check if the node was already visited
	if (!checkBitMap(node_index, d->visited)) {

		setBitMap(node_index, d->visited);																	//if not visited set it as visited
		nChildren = getChildrenLock(node_index, -1, d->label_index, d->tot_label, d->index_file);			// get the number of children for this node
		int* order = Randomize(nChildren, d->label_index + nChildren);										//generate a random order to visit the children

		for (j = 0; j < nChildren; j++) {																	//cycle over al children following random order
			curChild = getChildrenLock(node_index, order[j], d->label_index, d->tot_label, d->index_file);	//get current children
			if (curChild >= 0 && curChild < nNodes) {
				tmp = RandomizedVisit(curChild, d);															//call RandomizedVisit on the children
				if (tmp < minLower)																			//store the minimum of the lower label for for the current node's children
					minLower = tmp;
			}
			else
				printf("\nThread %d errore %d,curChild:%d\n", d->label_index, node_index, curChild);
		}
		free(order);
		lockIndex(node_index, d->label_index, d->tot_label);			//lock the index
		index_obj = readIndex(node_index, d->index_file);				//read current index to save modification already done by other threads

		WaitForSingleObject(percMutex, INFINITE);						//update progress bar
		labelProgressbar++;
		if (((float)labelProgressbar / (nNodes * d->tot_label) * 100) >= LabelPerc + 1) {
			LabelPerc += 1;
			r = printf("%d%%", LabelPerc);
			for (z = 0; z < r; z++)
				printf("\b");
		}
		ReleaseMutex(percMutex);
		
		lowlab = min(minLower, d->r);					//compute lower label	
		index_obj->labels[d->label_index].low = lowlab;	//set lower label
		index_obj->labels[d->label_index].high = d->r;	//set higher label
		writeIndex(index_obj, d->index_file);			//write index on the index file
		unlockIndex(node_index, d->label_index);		//unlock node 
		d->r++;											//update value of r
		free(index_obj);
		return lowlab;
	}
	else {		//if i already visited the node  i read its value from the index to return the left label
		lockIndex(node_index, d->label_index, d->tot_label);
		index_obj = readIndex(node_index, d->index_file);
		unlockIndex(node_index, d->label_index);
		lowlab = index_obj->labels[d->label_index].low;
		free(index_obj);
		return lowlab;
	}
}

//given a vector v it randomizes it's element by two by two swapping
void shuffle(int* v, int n, int seed) {					

	srand((unsigned)time(NULL) + seed);
	int i = 0, tmp;
	for (i = n - 1; i >= 1; i--) {
		int j = rand() % (i + 1);
		tmp = v[i];
		v[i] = v[j];
		v[j] = tmp;
	}
}

//allocates a vecor of increasing integer (0 to n) and shuffles it
int* Randomize(int n, int seed) {
	int j;
	int* indexes = (int*)malloc(n * sizeof(int));  //alloca un vettore di n elementi
	for (j = 0; j < n; j++) {							//assegna ad ogni elemento il valore del proprio indice
		indexes[j] = j;
	}
	shuffle(indexes, n, seed);							//mescolo il vettore (per ottenere un ordine di accesso ai figli casuale)
	return indexes;
}

//return true if a query u->v is reachable calling recursiveRechable
BOOL Reachable(int u, int  v, int nLabels) {
	struct reach data;
	int i;
	if (u >= nNodes || u < 0 || v >= nNodes || v < 0)
		return FALSE;
	data.u = readIndex(u, hIndex);	//read starting and ending index from the index file
	data.v = readIndex(v, hIndex);
	data.nLabels = nLabels;
	data.visited = (DWORD*)malloc(rootBitmapDim * sizeof(DWORD));	//allocate a bitmap to store visited nodes
	for (i = 0; i < rootBitmapDim; i++)
		data.visited[i] = 0;
	int ret =RecursiveReachable(&data);			//start recursion to look for the reachability
	
	free(data.u);
	free(data.v);
	free(data.visited);
	return ret;
}

//return true if a query u->v is reachable using nLabels to speed up the process
BOOL RecursiveReachable(struct reach * data) {
	struct reach* next_data;
	int i, child_index, childCount = 0, ret=0;

	//if u is equal to v the the original query is reachable
	if (data->u->node_index == data->v->node_index) {
		return 1;
	}

	//check if the node was already visite if it wasn't set it as visited otherwise retun as not reachable
	BOOL visited = checkBitMap(data->u->node_index, data->visited);
	if (!visited)
		setBitMap(data->u->node_index, data->visited);
	else
		return 0;
	
	for (i = 0; i < data->nLabels; i++) {
		//error checking 
		if (data->v->labels[i].low == nNodes + 1 || data->u->labels[i].low == nNodes + 1)
			continue;
		//check the labels to se if the query is not reachable without doing the full DFS
		if (!(data->v->labels[i].low >= data->u->labels[i].low && data->v->labels[i].high <= data->u->labels[i].high)) {
			return 0;
		}
	}
	
	childCount = data->u->nChildren;								//get the number of children
	next_data = (struct reach*)malloc(sizeof(struct reach));		//allocate data to pass to next level of recursion

	for (i = 0; i < childCount; i++) {								//cycle over all children
		child_index = getChildren(data->u->node_index, i, hIndex);	//read the current child
		
		next_data->u = readIndex(child_index, hIndex);				//read it's index

		next_data->nLabels = data->nLabels;							//set struct to pass to next level
		next_data->v = data->v;
		next_data->visited = data->visited;

		ret= RecursiveReachable(next_data);							//call recursiveReachable on (child of u)->v
		free(next_data->u);
		if (ret == 1)												//if it is reachable i stop the recursion
			break;
		
	}	
	free(next_data);

	return ret;
}

//opens the query file and generates n_thread to read it and check reachability queries
VOID QueryResolutionSetup(LPTSTR queryFile,int n_thread) {
	FILE* fIn;
	HANDLE* threadH;
	DWORD* tId;
	int i = 0;
	_wfopen_s(&fIn, queryFile, _T("r"));

	if (fIn == NULL) {
		fprintf(stderr, ("Error opening query file!\n"));
		return;
	}

	//initialize values for the progress bar
	queryTimeSum = 0;
	queryTimeCount = 0;
	queryStatsMutex = CreateMutex(0, FALSE, 0);
	bufferMutex = CreateMutex(NULL, FALSE, NULL);

	threadH = (HANDLE*)malloc(n_thread * sizeof(HANDLE));  //Threads initialization
	tId = (DWORD*)malloc(n_thread * sizeof(DWORD));

	//launches n_threads
	for (i = 0; i < n_thread; i++) {
		tId[i] = i;
		threadH[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ResolveQueries, fIn, 0, &tId[i]);
		if (threadH[i] == NULL)
			ExitProcess(1);
	}
	WaitForMultipleObjects(n_thread, threadH, TRUE, INFINITE);
	for (i = 0; i < n_thread; i++)
		CloseHandle(threadH[i]);

	CloseHandle(queryStatsMutex);
	CloseHandle(bufferMutex);
	free(threadH);
	free(tId);
	fclose(fIn);

	fprintf(stdout, ("---> Number of queries: %d\n---> Query Total Time: %fs\n---> Query Avg Time: %fs\n\n"), queryTimeCount, queryTimeSum, queryTimeSum / queryTimeCount);
}

//reads the query files ina synchronous way and calls Reachable over it, it also computes statistics
void ResolveQueries(LPVOID params) {
	FILE* fp = (FILE*)params;
	int i1, i2,read=1,i,j,n=0, r,  z;
	float q = 0;
	clock_t begin_query, end_query;
	BOOL res = FALSE;

	int buffer[QUERYBUFFER][2];		//matrix used to store QUERYBUFFER reachability queries, used to improove synchronization between threads

	//cycle until there are queries to read
	while (read>0) {

		WaitForSingleObject(bufferMutex, INFINITE);			//access the file pointer in mutual exclusion
		for (i = 0; i < QUERYBUFFER && read > 0; i++) {		//fill up the buffe
			read = fscanf_s(fp, "%d %d", &i1, &i2);
			if (read > 0) {
				buffer[i][0] = i1;
				buffer[i][1] = i2;
			}
			else
				i--;
		}
		ReleaseMutex(bufferMutex);							//release the mutex

		for(j=0;j<i;j++){
			begin_query = clock();
			res = Reachable(buffer[j][0], buffer[j][1], nL);				//call reachable on each query in the buffer
			end_query = clock();

			fprintf(queryOutput,"%d,%d,%d\n", buffer[j][0], buffer[j][1], res);
			n++;


			WaitForSingleObject(queryStatsMutex, INFINITE);			//update the progress bar
			queryCount++;
			r = printf("---> Executed Queries: %d", queryCount);
			for (z = 0; z < r; z++)
				printf("\b");
			ReleaseMutex(queryStatsMutex);

			q += (double)(end_query - begin_query) / CLOCKS_PER_SEC;
			
		}
	}
	WaitForSingleObject(queryStatsMutex, INFINITE);
	queryTimeSum += q;								//update statistics
	queryTimeCount += n;
	ReleaseMutex(queryStatsMutex);
	ExitThread(0);

}