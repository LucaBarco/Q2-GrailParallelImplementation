
#ifndef EDGE_H
#define EDGE_H
typedef struct edge_s {
    int v;
    int w;
    int wt;
}Edge;

Edge EDGEcreate(int v, int w);
#endif //EDGE_H
