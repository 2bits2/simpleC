#include "compiler.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <assert.h>


int arr_ituple_init(IntTupleArray *tuples){
  tuples->cap = 10;
  tuples->len = 0;
  tuples->data = malloc(sizeof(*tuples->data) * tuples->cap);
  if (!tuples->data) {
    fprintf(stderr, "couldn't initialize dep graph data with cap %d \n", tuples->cap);
    return -1;
  }
  return 0;
}

int arr_ituple_quit(IntTupleArray *graph){
  free(graph->data);
  graph->cap = 0;
  graph->len = 0;
  return 0;
}

int arr_ituple_add(IntTupleArray *graph, IntTuple entry){
  if(graph->len >= graph->cap){
    int newcap = graph->cap * 2;
    IntTuple *entries = realloc(graph->data, sizeof(IntTuple) * newcap);
    if(!entries){
      fprintf(stderr, "couldn't add tuple (%ld %ld) \n", entry.first, entry.second);
      return -1;
    }
    graph->data = entries;
    graph->cap = newcap;
  }
  graph->data[graph->len] = entry;
  graph->len++;
  return 0;
}


int depgraph_init(DepGraph *graph){
  graph->resolved_i = 0;
  return arr_ituple_init(&graph->dependencies);
}

int depgraph_quit(DepGraph *graph){
  return arr_ituple_quit(&graph->dependencies);
}

int depgraph_add(DepGraph *graph, IntTuple tuple){
  graph->resolved_i++;
  return arr_ituple_add(&graph->dependencies, tuple);
}

int depgraph_quasi_resolve(DepGraph *graph, int resolvedItem){
  // everything that depends on the resolved item
  // moves to the quasi resolved section
  int num_resolved = 0;
  for (int i = 0; i < graph->resolved_i; i++) {
    IntTuple tuple = graph->dependencies.data[i];
    while(tuple.second == resolvedItem && i < graph->resolved_i ){
      IntTuple tmp = graph->dependencies.data[graph->resolved_i-1];
      graph->dependencies.data[graph->resolved_i - 1] = tuple;
      graph->dependencies.data[i] = tmp;
      graph->resolved_i--;
      tuple = tmp;
      num_resolved++;
    }
  }
  return num_resolved;
}

void depgraph_remove_pending_duplicates(DepGraph *graph, IntTuple tuple){
  for (int i = graph->resolved_i; i < graph->dependencies.len; i++) {
    IntTuple t = graph->dependencies.data[i];
    while (t.first == tuple.first && i < graph->dependencies.len) {
      t = graph->dependencies.data[graph->dependencies.len - 1];
      graph->dependencies.data[i] = t;
      graph->dependencies.len--;
    }
  }
}

IntTuple depgraph_pop_quasi_resolved(DepGraph *graph){
  IntTuple err = {DEPGRAPH_EMPTY, DEPGRAPH_EMPTY};
  if(graph->dependencies.len <= 0){
    return err;
  }
  IntTuple tuple = graph->dependencies.data[graph->dependencies.len - 1];
  graph->dependencies.len--;
  return tuple;
}

int depgraph_item_is_fully_resolved(DepGraph *graph, IntTuple res) {
  bool is_fully_resolved = true;
  for (int i = 0; i < graph->resolved_i; i++) {
    if (graph->dependencies.data[i].first == res.first) {
      depgraph_remove_pending_duplicates(graph, res);
      is_fully_resolved = false;
      break;
    }
  }
  return is_fully_resolved;
}

IntTuple depgraph_pop_fully_resolved(DepGraph *graph) {
  // now we get one of the quasi resolved
  // elements and ensure that it
  // really doesn't depend on anything else
  IntTuple res;
  int is_resolved = false;
  do {
    res = depgraph_pop_quasi_resolved(graph);
    is_resolved = depgraph_item_is_fully_resolved(graph, res);
    depgraph_remove_pending_duplicates(graph, res);
  } while (graph->resolved_i <= graph->dependencies.len && !is_resolved);
  return res;
}

uint64_t depgraph_resolve(DepGraph *graph) {
  if (graph->resolved_i <= 0) {
    if (graph->dependencies.len <= 0) {
      fprintf(stderr, "already resolved dependency graph\n");
      return DEPGRAPH_EMPTY;
    }
    // there are still some
    // resolved dependencies pending
    // so get the last one
    IntTuple tuple = depgraph_pop_quasi_resolved(graph);
    depgraph_remove_pending_duplicates(graph, tuple);

    // now we can return
    // the pending dependency
    return tuple.first;
  }

  /* if (graph->resolved_i != 0 graph->dependencies.len == graph->resolved_i) { */
  /*   // error circular dependency */
  /*   return DEPGRAH_CIRCULAR_DEP; */
  /* } */

  IntTuple res = depgraph_pop_fully_resolved(graph);
  int num_resolved = depgraph_quasi_resolve(graph, res.first);

  return res.first;
}

void depgraph_print(DepGraph *graph) {
  printf("depgraph\n");
  printf("----- unresolved ------\n");
  for (int i = 0; i < graph->resolved_i; i++) {
    IntTuple tuple = graph->dependencies.data[i];
    printf("(%ld %ld)\n", tuple.first, tuple.second);
  }
  printf("---- resolved -----------\n");
  for (int i = graph->resolved_i; i < graph->dependencies.len; i++) {
    IntTuple tuple = graph->dependencies.data[i];
    printf("(%ld %ld)\n", tuple.first, tuple.second);
  }
}

void depgraph_prepare(DepGraph *graph) {
  depgraph_quasi_resolve(graph, 0);
}


void depgraph_test() {

  DepGraph graph;
  depgraph_init(&graph);

  IntTuple a = {.first = 'a', .second = 'b'};
  IntTuple b = {.first = 'b', .second = 'c'};
  IntTuple c = {.first = 'b', .second = 'd'};
  IntTuple d = {.first = 'b', .second = 'e'};
  IntTuple e = {.first = 'c', .second = 0};
  IntTuple f = {.first = 'e', .second = 0};
  IntTuple g = {.first = 'd', .second = 0};

  depgraph_add(&graph, a);
  depgraph_add(&graph, b);
  depgraph_add(&graph, b);
  depgraph_add(&graph, b);
  depgraph_add(&graph, b);
  depgraph_add(&graph, b);
  depgraph_add(&graph, c);
  depgraph_add(&graph, d);
  depgraph_add(&graph, e);
  depgraph_add(&graph, f);
  depgraph_add(&graph, g);

  depgraph_print(&graph);
  depgraph_prepare(&graph);

  int dep = 0;
  do {
    depgraph_print(&graph);
    /* depgraph_print(&graph); */
    dep = depgraph_resolve(&graph);
    printf("resolved dep %c \n", dep);
  } while (dep >= 0);

  depgraph_print(&graph);
  depgraph_quit(&graph);
}
