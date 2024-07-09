#ifndef MY_DEP_H
#define MY_DEP_H

//////// dependency solver ///////
/// the sizes of structs shall be determined
/// that rely on other struct members so a dependency graph
/// is created to resolve the sizes in topological order
typedef struct IntTuple {
  uint64_t first;
  uint64_t second;
} IntTuple;

typedef struct IntTupleArray {
  IntTuple *data;
  int len;
  int cap;
} IntTupleArray;

#define DEPGRAPH_EMPTY (UINT64_MAX)

typedef struct DepGraph {
  IntTupleArray dependencies;
  int resolved_i;
} DepGraph;

void depgraph_test();
int  depgraph_init(DepGraph *graph);
int  depgraph_quit(DepGraph *graph);
void depgraph_print(DepGraph *graph);

// adds a dependency to specific id
// the second item of the tuple is 0 if it is just
// a node with zero dependencies
int depgraph_add(DepGraph *graph, IntTuple tuple);

// needs to be called before resolving
void depgraph_prepare(DepGraph *graph);

// returns an id that can be resolved now
// -1 if there is nothing to be resolved anymore
uint64_t depgraph_resolve(DepGraph *graph);

#endif
