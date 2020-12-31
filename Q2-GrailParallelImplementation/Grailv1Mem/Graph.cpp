#include "Graph.h"
#define MAX 32              //dimension of one row of a bitmap (is a DWORD variable)
#define BUFFER 100          //dimension of the buffer to store the row of the graph file
#define MINIBUFFER 100      //dimension of the buffer to store the node number
#define QUERYBUFFER 1     //dimension of the buffer to store queries of a specific thread 


//STRUCTS DEFINITION
typedef struct node* link;
struct node {
    int v;                           //destination node
    link next;                      //next element of the list
};


struct graph {
    int V;                                   //number of vertex
    int E;                                   //number of edges
    link* adj;                               //vector for adj list
    int** MatrAdj;                           //adjMatr
    link z;                                  //ending element of a list
    bool matr;                               //if true, we're using adj matrix, otherwise we're using adj list
    int batch_sz;                            //size of the input graph file's portion that each thread should read
    int* nChildren;                          //array that contains the number of children of a specific node
    LPTSTR file;                             //name of the file containing the graph
    struct label** LabelMatrix = NULL;       //matrix for labels
    int d;                                   //number of labels
    int dim;                                 //bitmap dimension
    DWORD* bitmap;                           //bitmap definition (to find roots)
};


struct label {                              //definition of the label
    DWORD low;
    DWORD high;
};

typedef struct reader_thread_arg {          //graph reading params for a thread
    DWORD index;
    LARGE_INTEGER offset;
}tP;

struct tread_t_label {                      //label generation params for a thread
    DWORD label_index;
    DWORD tot_label;
    DWORD* visited;
    DWORD r;
};


struct query_data {                         //single query resolution params for a thread
    int u;
    int v;
    int nLabels;
    DWORD* visited;
};

struct query_chunk {                        //multiple queries resolution params for a thread
    FILE* fp;
    Graph G;
    int id;
    int d;
};



//GLOBAL VARIABLES

HANDLE queryFileMutex, queryMutex, bitmapMutex, printMutex, percMutex;

/*
*   queryFileMutex : protects to access the query file
*   queryMutex : protects the global variables totNumQuery and totQueryTime
*   bitmapMutex : protects the bitmap
*   printMutex : protects the printf
*   percMutex : protects the stats global variables (progressbar, perc)
*/

Graph G;                                                    //internal global pointer to the Graph
int totNumQuery = 0;
int progressbar = 0;
int perc = 0;
double totQueryTime = 0;
FILE* fQueryOut = 0;
FILE* fLabelsOut = 0;


//GRAPH FUNCTIONS

void clearString(char* str, int N) {
    //to clear a string str of N chars
    int i;
    for (i = 0; i < N; i++) {
        str[i] = ' ';
    }
    }


void setBitMap(int index, DWORD* bitmapArray) {
    //to set a bit to 1 in a bitmap             
    DWORD word_offset = index / MAX;
    DWORD bit_offset = index % MAX;
    bitmapArray[word_offset] |= (1 << bit_offset);
}


BOOL checkBitMap(int index, DWORD* bitmapArray) {
    //to check if a bit in set to 1 in a bitmap         
    DWORD word_offset = index / MAX;
    DWORD bit_offset = index % MAX;
    if ((bitmapArray[word_offset] & (1 << bit_offset)) == 0)
        return FALSE;
    else
        return TRUE;
}

link NEW(int v, link next) {
    //to create a new edge to the vertex v and insert in the head of the list
    link p = (link)malloc(sizeof(*p));
    p->v = v;
    p->next = next;
    return p;
}

link REMOVE(link head, link next) {
    //to remove an element from the adj list
    link t = head;
    head = next;
    free(t);
    return head;
}


Graph GRAPHinit(int N, const char* modality) {
    //to initialize a Graph of N vertexes. If the modality is "matrix" or "list" it will be implemented as a (respectively) adj matrix or adj list
    int i,j;
    G = (Graph)malloc(sizeof(*G));
    G->E = 0;
    G->V = N;
    G->z = NEW(-1, NULL);
    G->dim = N / MAX + 1;
    G->bitmap = (DWORD*)malloc(G->dim * sizeof(DWORD));
    G->nChildren = (int*)malloc(N * sizeof(int));

    for (i = 0; i < G->dim; i++) {
        G->bitmap[i] = 0;
    }

    if (strcmp(modality, "list") == 0) {//initialize adj list
        G->matr = false;
        G->adj = (link*)malloc((N) * sizeof(struct node));
        for (i = 0; i < N; i++) {
            G->adj[i] = G->z;
            G->nChildren[i] = 0;
        }
    }
    else {
        if (strcmp(modality, "matrix") == 0) {//initialize adj matrix
            G->matr = true;
            G->MatrAdj = (int**)malloc(G->V * sizeof(int*));
            for (i = 0; i < G->V; i++) {
                G->nChildren[i] = 0;
                G->MatrAdj[i] = (int*)malloc(G->V * sizeof(int));
                for (j = 0; j < N; j++) {
                    G->MatrAdj[i][j] = 0;
                }
            }
        }
    }
    bitmapMutex = CreateMutex(NULL, FALSE, NULL);
   
    return G;
}

void GRAPHFree(Graph G) {
    int i, j,k=0;
    link l;
    if (G->matr) {
        for (i = 0; i < G->V; i++) {
            free(G->MatrAdj[i]);
        }
    }
    else {
        for (i = 0; i < G->V; i++) {
            k = 0;
            for (l = G->adj[i]; l != G->z; l = l->next) {
                k++;
            }
            for (j = 0; j < k; j++) {
                G->adj[i]=REMOVE(G->adj[i], G->adj[i]->next);
            }
        }
    }
    free(G->z);
    free(G);
    CloseHandle(bitmapMutex);
}

bool thereIsChild(link head, int child) {
    link l;
    for (l = head; l != NULL; l = l->next) {
        if (l->v == child) {
            return true;
        }
    }
    return false;
}

void GRAPHinsert(Graph G, Edge e) {
    //insert in the Graph
    int v = e.v, w = e.w, wt = e.wt;
    if (G->matr) {
        if (G->MatrAdj[v][w] == 0) {
            G->MatrAdj[v][w] = 1;
            G->E++;
            G->nChildren[v]++;
        }
    }
    else {
        if (!thereIsChild(G->adj[v], w)) {
            G->adj[v] = NEW(w, G->adj[v]);
                G->nChildren[v]++;
                G->E++;
        }
    }
}


DWORD WINAPI ThreadReader(LPVOID args) {
    //to read a portion of the file (worker thread)
    tP* data = (tP*)args;
    
    LARGE_INTEGER relative_offset, start_offset;		   //relative offset: relative distance with respect to the starting point of my block 
                                                           //start_offset: starting byte from which I should start to read 
 
    int flag = 1;                                          //to detect if I'm reading a node index (=1), or its children (=0)
    int j = 0, i = 0, k = 0,index;
    int read_sharp = 0;		                               //to detect if I've read the last sharp
                                                     



    char buffer[BUFFER];								   //single row of the graph input file
    char minibuffer[MINIBUFFER];						   //contains a parent node
    char child[MINIBUFFER];						           //contains a child node
    int node;
    FILE* fp = 0;
    relative_offset.QuadPart = 0;
    start_offset.QuadPart = 0;

    _wfopen_s(&fp, G->file, _T("r"));                   //opening the file and moving into my block
    fseek(fp, data->offset.QuadPart, SEEK_SET);			



    fgets(buffer, BUFFER, fp);						//read first row and
                                                    //  thread 0: delete the row containing the number of nodes
                                                    //  thread n: discard the first row until the # 
    relative_offset.QuadPart += strlen(buffer);

    //read until the end of the file or of my block (the end of my block is signaled by a reading of a # after my portion) 
    //in this way, every row of the file is covered

    while (fgets(buffer, BUFFER, fp) != NULL && ((relative_offset.QuadPart < G->batch_sz) || read_sharp == 0)) {
        j = 0;

        for (i = 0; i < BUFFER && buffer[i] != '\0'; i++) {		    //for each char in "buffer"
            if (iswdigit(buffer[i]) && flag == 1) {			        //      - if it is a number, it means that I'm reading a node index
                minibuffer[j] = buffer[i];						    //          add it to "minibuffer"
                j++;
            }
            else if (buffer[i] == (':')) {							    //  - otherwise, if it's ':', it means that I'm starting to read the children indexes
                start_offset.QuadPart = relative_offset.QuadPart + 1;	//      (save the current offset into start_offset)
                minibuffer[j] = '\0';                                   //      it also means that I've finished to read the node index, so I can close the minibuffer compilation
                flag = 0;                                               //      signal that I'm not reading a node index anymore
                j = 0;
                node = atoi(minibuffer);                                //      convert the minibuffer to an integer
                clearString(minibuffer, MINIBUFFER);                    //      clear the minibuffer

                WaitForSingleObject(percMutex, INFINITE);               //      Updating percentage...
                progressbar ++;
                if (((float)progressbar / (G->V) * 100) >= perc + 1) {
                    perc += 1;
                    int maxR = G->V / 10;
                    int r = printf("%d%%", perc);
                    for (int z = 0; z < r; z++) {
                        printf("\b");
                    }
                }
                ReleaseMutex(percMutex);
            }
            else if (buffer[i] == ('#') && flag == 0) {				    //  - otherwise, if it's '#', it means that a row (so a the children list) is finished
                flag = 1;                                               //    reset the flag: in this way in the next iteration I know that I should start read a new node
                if (relative_offset.QuadPart >= G->batch_sz)		    //    check: If I have finished my block size, I need to stop (set read_sharp=1)
                    read_sharp = 1;
                relative_offset.QuadPart++;                             //     increment my relative offset
            }
            else if (buffer[i] == (' ') && flag == 0 && k > 0) {        // - otherwise, if it's ' ', it means that I've just finished the reading of a child
                child[k] = '\0';                                        //    close the buffer "child"
                k = 0;                                                  
                index = atoi(child);                                    //    convert the buffer "child" into an integer

                clearString(child, MINIBUFFER);                         //    clear the buffer "child"
                GRAPHinsert(G, EDGEcreate(node, index));                //    insert the edge node->child in the graph
                WaitForSingleObject(bitmapMutex, INFINITE);
                setBitMap(index, G->bitmap);                            //    set the bitmap: signal that the children cannot be a root
                ReleaseMutex(bitmapMutex);
            }
            else if (iswdigit(buffer[i]) && flag == 0) {                // - otherwise, I'm reading a child index
                child[k] = buffer[i];                                   //   add the char to the buffer "child"
                k++;
            }

            relative_offset.QuadPart++;		                            // increment my relative offset

        }
    }
   
    fclose(fp);
    ExitThread(0);
}


Graph GraphParallelRead(LPTSTR inputFile, Graph G, int NThread) {
    //to parallel read the Graph G. You need to specify the file name, the graph structure and the number of threads.

    //setting up variables + mutex to compute percentage
    progressbar = 0;                                    
    perc = 0;
    percMutex = CreateMutex(NULL, FALSE, NULL);


    FILE* fIn;                                      // input file
    DWORD size;                                     // size of the file
    tP* threadPars;                                 // array of thread params
    HANDLE* threads;                                // array of thread handles
    DWORD* tIds;                                    // array of thread ids
    int i = 0;

    NThread = min(NThread, 8);                      //define the number of threads (max. 8, empirical reasons)
    
    //open the file and retrive its dimension
    _wfopen_s(&fIn, inputFile, _T("r"));            
    fseek(fIn, 0L, SEEK_END);
    size = ftell(fIn);	
    fclose(fIn);


    G->batch_sz = size / NThread;	                //define the block dimension that each thread should deal with
    G->file=inputFile;                              //set the global variable inputFile with the name of the Graph file

    //prepare threads....
    threads = (HANDLE*)malloc(NThread * sizeof(HANDLE));
    threadPars = (tP*)malloc(NThread * sizeof(tP));
    tIds = (DWORD*)malloc(NThread * sizeof(DWORD));

    printf("---> Reading the graph...");

    for (i = 0; i < NThread; i++) {
        tIds[i] = i;                                                        
        threadPars[i].index = i;                                            
        threadPars[i].offset.QuadPart = (LONGLONG)i * G->batch_sz;       //set the starting offset for the thread
        
        threads[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadReader, &threadPars[i], 0, &tIds[i]);
        if (threads[i] == NULL) {
            ExitProcess(1);
        }
    }

    //wait for the threads to finish...
    WaitForMultipleObjects(NThread, threads, TRUE, INFINITE);

    //close and free...
    for (i = 0; i < NThread; i++) {
        CloseHandle(threads[i]);
    }
    printf("Done!\n");
    free(threads);
    free(tIds);
    CloseHandle(percMutex);

}

void GraphPrint(Graph G) {
    //to print the graph
    int i,j;
    link t;
    if (!G->matr) {
        for (i = 0; i < G->V; i++) {
            printf("Node: %2d\tChildren: ", i);
            for (t = G->adj[i]; t != G->z; t = t->next) {
                printf("%d\t", t->v);
            }
            printf("\n");
        }
    }
    else {
        for (i = 0; i < G->V; i++) {
            printf("Node: %2d\t Children: ", i);
            for (j = 0; j < G->V; j++) {
                if (G->MatrAdj[i][j]==1) {
                    printf("%d\t",j);
                }
            }
            printf("\n");
        }
    }
    /*
    for (i = 0; i < G->V; i++) {
        printf("node %d has %d children\n", i, G->nChildren[i]);
    }*/
    printf("Total number of edges: %d\n\n", G->E);
}

void printLabels(Graph G) {
    //to print labels on consolle
    int i,j;
    if (G->LabelMatrix != NULL) {
        for (i = 0; i < G->V; i++) {
            printf("Labels for node %d:", i);
            for (j = 0; j < G->d; j++) {
                printf("\t[%d, %d]", G->LabelMatrix[i][j].low, G->LabelMatrix[i][j].high);
            }
            printf("\n");
        }
    }
}
void printLabelsOnFile(Graph G, LPTSTR file) {
    //to print labels on file
    int i, j;
    progressbar = 0;
    perc = 0;
    _wfopen_s(&fLabelsOut, file, _T("w"));
    wprintf_s(_T("---> Printing labels on file %s...\n"),file);
    printRoots(G, fLabelsOut);
    if (G->LabelMatrix != NULL) {
        for (i = 0; i < G->V; i++) {
            fprintf(fLabelsOut,"Labels for node %d:", i);
            for (j = 0; j < G->d; j++) {
                fprintf(fLabelsOut,"\t[%d, %d]", G->LabelMatrix[i][j].low, G->LabelMatrix[i][j].high);
            }
            fprintf(fLabelsOut,"\n");
        }
        progressbar++;
        if (((float)progressbar / (G->V) * 100) >= perc + 1) {
            perc += 1;
            int maxR = G->V / 10;
            int r = printf("%d%%", perc);
            for (int z = 0; z < r; z++) {
                printf("\b");
            }
        }
        fclose(fLabelsOut);
    }
    printf("Done!\n");
    progressbar = 0;
    perc = 0;
}
int getChild(Graph G, int node, int child_index) {
    //get the index of the 1st, 2nd, 3rd, 4th,... child of node in graph G
    int i, j, k = -1;
    link l;

    if (G->matr) {     
            for (j = 0; j < G->V; j++) {
                if (G->MatrAdj[node][j] == 1) {
                    k++;
                    if (k == child_index) {
                        return j;
                    }
                }
            }
            return -1;
    }
    else {
        for (i = 0, l = G->adj[node]; i < child_index && l->next != G->z; i++, l = l->next);
        if(i==child_index)
            return l->v;
        return -1;  
    }
}

void shuffle(int* v, int n, int seed) {
    //to shuffle the n elements of vector v 2 by 2
    srand((unsigned)time(NULL) + seed);
    int i = 0, tmp;
    for (i = n - 1; i >= 1; i--) {
        int j = rand() % (i + 1);
        tmp = v[i];
        v[i] = v[j];
        v[j] = tmp;
    }
}

int* Randomize(int n, int seed) {
    //to create a vector of n random elements
    int j;
    int* indexes = (int*)malloc(n * sizeof(int));  
    for (j = 0; j < n; j++) {							
        indexes[j] = j;
    }
    shuffle(indexes, n, seed);						
    return indexes;
}

int  RandomizedVisit(int node_index, struct tread_t_label* d) {
    //to randomly visit the graph startinf from node_index and reading d labels.
    //Random here means that we visit the children of node_index in a random order 
    int nChildren, j, curChild, minLower = G->V, tmp, lowlab;
    /*
        nChildren: #children of node_index
        curChild: index of a specific child
        minLower: to compute the left label -> contains the min of left labels of all children
        tmp: temporal varable to compute minLower
        lowlab: contains the left label for this node
    */

    if (!checkBitMap(node_index, d->visited)) {                             //if node_index is not already visited
        setBitMap(node_index, d->visited);                                  //  set it as visited
        nChildren = G->nChildren[node_index];                               //  retrieve its children number
        int* order = Randomize(nChildren, d->label_index + nChildren);      //  create a vector to random visit them

        for (j = 0; j < nChildren; j++) {                                   //for each children
            curChild = getChild(G, node_index, order[j]);                   // get its real index
            tmp = RandomizedVisit(curChild, d);                             //recurring.....
            if (tmp < minLower)                                             //compute min of children's left labels
                minLower = tmp;
        }
        free(order);                                                       

        //update stats.....
        WaitForSingleObject(percMutex, INFINITE);
        progressbar++;
        if (((float)progressbar / (G->V * d->tot_label) * 100) >= perc + 1) {
            perc += 1;
            int r = printf("%d%%", perc);
            for (int z = 0; z < r; z++) {
                printf("\b");
            }
        }
        ReleaseMutex(percMutex);

       
        lowlab = min(minLower, d->r);                                   //compute the final label for this node [min(min(Li),r),r]
        G->LabelMatrix[node_index][d->label_index].high = d->r;
        G->LabelMatrix[node_index][d->label_index].low = lowlab;
        d->r++;
        return lowlab;                                                  //return the left label of node_index
    }
    else {                                                              // otherwise, if the node is already visited
        lowlab = G->LabelMatrix[node_index][d->label_index].low;        //  ---> return the left label of node_index
        return lowlab;
    }
}


DWORD WINAPI singleIndexRandomizedLabelling(LPVOID params) {
   //to generate a visit order for the whole graph, and call the labelling visit function
    struct tread_t_label* d = (struct tread_t_label*)params;
    int i;
    int* order = Randomize(G->V, 0);

    for (i = 0; i < G->V; i++) {
        if (!checkBitMap(order[i], G->bitmap)) {            //if the node is a root, start the visit from it
             RandomizedVisit(order[i], d);
        }
     }
    free(order);
    ExitThread(0);
}

void RandomizedLabelling(Graph G,int d) {
    //global settings of the d threads that will execute the labelling
    progressbar = 0;
    perc = 0;
    int i, j;
    G->d = d;
    HANDLE* threadH = (HANDLE*)malloc(d * sizeof(HANDLE));
    DWORD* tId = (DWORD*)malloc(d * sizeof(HANDLE));
    percMutex = CreateMutex(NULL, FALSE, NULL);
    struct tread_t_label* params = (struct tread_t_label*)malloc(d * sizeof(struct tread_t_label));


    G->LabelMatrix = (struct label**)malloc(G->V * sizeof(struct label*));          //label matrix creation and initialization
    for (i = 0; i < G->V; i++) {
        G->LabelMatrix[i] = (struct label*)malloc(d * sizeof(struct label));
            for (j = 0; j < d; j++) {
                G->LabelMatrix[i][j].high = 0;
                G->LabelMatrix[i][j].low = 0;
            }
    }
    

    printf("---> Creating labels... ");
    for (i = 0; i < d; i++) {                                                       //threads initialization
        params[i].label_index = i;
        params[i].tot_label = d;
        params[i].visited = (DWORD*)malloc(G->dim * sizeof(DWORD));
        for (j = 0; j < G->dim; j++)
            params[i].visited[j] = 0;
        params[i].r = 1;
        threadH[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)singleIndexRandomizedLabelling, &params[i], 0, &tId[i]);
    }

    WaitForMultipleObjects(d, threadH, TRUE, INFINITE);

    for (i = 0; i < d; i++) {
        free(params[i].visited);
        CloseHandle(threadH[i]);
    }
    printf("Done!\n ");
    free(threadH);
    free(tId);
    free(params);
    progressbar = 0;
    CloseHandle(percMutex);
}

void printRoots(Graph G, FILE* fp) {
    //to print the roots
    int i,j=0;
   
        DWORD count = 0;
        for (i = 0; i < G->V; i++) {
            if (checkBitMap(i, G->bitmap) == FALSE)         //if a bit is zero in the bitmap, the corresponding node is a root
                count++;
        }
        if(fp==stdout){
            printf("---> TOT Roots:%d\n\n",count);
            return;
        }
        
        fprintf(fp,"#Roots:\t%d\nRoots indexes:\t", count);
    
    for (i = 0; i < G->V; i++) {
        if (!checkBitMap(i, G->bitmap)) {
            fprintf(fp, "%d ", i);
            j++; 
            if (j == 20) {
                j = 0;
            fprintf(fp, "\n\t\t");
            }
        }
       

    }
        fprintf(fp,"\n\n");
    
}

DWORD RecursiveReachable(struct query_data* data) {
    //to recursivly see if a node v is reachable from a node u
    struct query_data* next_data;
    int i, child, childrenCount = 0;
    int ret;
   
    if (data->u == data->v) {                           // success recursion exit condition
        return 1;
    }
    if (data->u < 0 || data->u >= G->V) {               //error,exit
        printf("u: %d, exiting \n", data->u);
        return 0;
    }

    BOOL visited = checkBitMap(data->u, data->visited); //check if a a node was already visited
    if (!visited)
        setBitMap(data->u, data->visited);
    else 
        return 0;
    
    for (i = 0; i < data->nLabels; i++) {

        // check for error absence in the labels
        if (G->LabelMatrix[data->v][i].low == G->V + 1 || G->LabelMatrix[data->u][i].low == G->V + 1) 
            continue;
        //check if u labels do contain v labels
        if (!(G->LabelMatrix[data->v][i].low >= G->LabelMatrix[data->u][i].low && G->LabelMatrix[data->v][i].high <= G->LabelMatrix[data->u][i].high)) {
            return 0;
        }

    }
    childrenCount = G->nChildren[data->u];                  //number of children of node u
    next_data = (struct query_data*)malloc(sizeof(struct query_data));
   
    for (i = 0; i < childrenCount; i++) {                   //take the i-th child of node u and recur
        child = getChild(G,data->u, i);                    
        next_data->u =child;
        next_data->nLabels = data->nLabels;
        next_data->v = data->v;
        next_data->visited = data->visited;
        ret=  RecursiveReachable(next_data);
        if (ret == 1) {                                     //if v is reachable, do not recur on other children
           break;
        }
       
    }
    free(next_data);
    return ret;                                            
}


BOOL Reachable(int u, int  v, int nLabels, Graph G1) {
    //to setup query params and call the recursive resolution
    G = G1;
    struct query_data *data;
    DWORD ret;

    if (u < 0 || v < 0 || u >= G->V || v >= G->V) {                                 //the query has sense only for nodes actually present in the graph
        return 0;
    }

    data = (struct query_data*)malloc(sizeof(struct query_data));
    data->u = u;
    data->v = v;
    data->nLabels = nLabels;
    data->visited = (DWORD*)malloc(G->dim * sizeof(DWORD));
    for (int i = 0; i < G->dim; i++)
        data->visited[i] = 0;
    

    ret=RecursiveReachable(data);
    free(data->visited);
    free(data);
    return ret;
}

void ResolveQueries(LPVOID params) {
    //to setup the single query resolution params for a thread
    int n = 0;                                   //number of queries resolved by this thread
    int i, j;
    clock_t begin_query, end_query;
    int buffer[QUERYBUFFER][2];                  //buffer to possibly store more source-dest couples 
    double querytime = 0;                        //time spent in query resolution by this thread
    bool end = false;                            //flag to check if it is the last query for this thread
    struct query_chunk* data = (struct query_chunk*)params;
    FILE* filep = data->fp;
    
   
       
    while (!end) {
        WaitForSingleObject(queryFileMutex, INFINITE);
        for (i = 0; i < QUERYBUFFER && !end; i++) {
            end = fscanf_s(filep, "%d %d", &buffer[i][0], &buffer[i][1]) !=2;
        }   
        ReleaseMutex(queryFileMutex);
        if (end) {
            i--;
        }
       
        for (j = 0; j < i; j++) {
             begin_query = clock();
             BOOL res = Reachable(buffer[j][0], buffer[j][1], data->d, data->G);
             end_query = clock();
             querytime += (double)(end_query - begin_query) / CLOCKS_PER_SEC;      
             WaitForSingleObject(printMutex, INFINITE);
             fprintf(fQueryOut,"%d , %d reachable: %d\n", buffer[j][0],buffer[j][1], res);
             ReleaseMutex(printMutex);
             n++;

             WaitForSingleObject(percMutex, INFINITE);               //      Updating percentage...
             progressbar++;
                 int r = printf("%5d", progressbar);
                 for (int z = 0; z < r; z++) {
                     printf("\b");
                 }
             ReleaseMutex(percMutex);
        }
    }
   
    WaitForSingleObject(queryMutex, INFINITE);              //update global values for time and number of queries
    totQueryTime += querytime;
    totNumQuery += n;
    ReleaseMutex(queryMutex);
    ExitThread(0);
}

int getNumThread(LPTSTR queryFile){
    FILE* fIn;
    int n = 0,i1,i2;
    _wfopen_s(&fIn, queryFile, _T("r"));
    if (fIn == NULL) {
        fprintf(stderr, ("Error opening query file!\n"));
        return -1;
    }
    while( fscanf_s(fIn,"%d %d",&i1,&i2)==2){
        n++;
    }
    if(n>=0 && n<=100){
        return 1;
    }
    else if(n>100 && n<=250){
        return 2;
    }
    else if(n>250 && n<=500){
        return 4;
    }
    else if(n>500 && n<=750){
        return 8;
    }
    else if(n>750){
        return 16;
    }
    return -1;
}

VOID QueryResolutionSetup(LPTSTR queryFile, Graph G, int d) {
    //to setup each thread for query resolution, and collect global results
    totNumQuery = 0;
    totQueryTime = 0;
    perc = 0;
    progressbar = 0;
    FILE* fIn;
    HANDLE* threadH;
    struct query_chunk* tChunks;
    DWORD* tId;
   
    int n_thread = getNumThread(queryFile);                 //use the correct number of threads, based on the number of queries
    clock_t begin_query = clock();                          //start of time computation for global time tracking
     _wfopen_s(&fQueryOut, _T("queryResults.txt"), _T("w"));
     _wfopen_s(&fIn, queryFile, _T("r"));                   //open the file to print the queries
    if (fIn == NULL || n_thread==-1 || fQueryOut==NULL) {
        fprintf(stderr, ("Error opening query file!\n"));
        return;
    }
    //printf("I'm using %d threads! \n", n_thread);
    threadH = (HANDLE*)malloc(n_thread * sizeof(HANDLE));  
    tChunks = (struct query_chunk*)malloc(n_thread * sizeof(struct query_chunk));
    tId = (DWORD*)malloc(n_thread * sizeof(DWORD));
    queryFileMutex = CreateMutex(NULL, FALSE, NULL);
    queryMutex = CreateMutex(NULL, FALSE, NULL);
    printMutex = CreateMutex(NULL, FALSE, NULL);
    percMutex = CreateMutex(NULL, FALSE, NULL);
    wprintf_s(_T("---> Number of solved queries..."));
    for (int i = 0; i < n_thread; i++) {
        tId[i] = i;
        tChunks[i].fp = fIn;
        tChunks[i].id = i;
        tChunks[i].G = G;
        tChunks[i].d = d;
        threadH[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ResolveQueries, &tChunks[i], 0, &tId[i]);
        if (threadH[i] == NULL)
            ExitProcess(1);
    }

    WaitForMultipleObjects(n_thread, threadH, TRUE, INFINITE);
    for (int i = 0; i < n_thread; i++)
        CloseHandle(threadH[i]);

    free(threadH);
    free(tId);
    fclose(fIn);
    fclose(fQueryOut);
    CloseHandle(queryFileMutex);
    CloseHandle(queryMutex);
    CloseHandle(printMutex);
    CloseHandle(percMutex);
    clock_t end_query = clock();                        //end of time computation for global time tracking
    wprintf_s(_T("%d Done!\n---> Query results stored in queryResults.txt.\n"),progressbar);
    fprintf(stdout, "---> Query resolving time: %f s\n", (double)(end_query - begin_query) / CLOCKS_PER_SEC);
    fprintf(stdout, ("     Number of queries: %d s\n     Query Total Time: %lf s\n     Query Avg Time: %f s\n\n"), totNumQuery, totQueryTime, totQueryTime / totNumQuery);
  
}










