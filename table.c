#include "compiler.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int ptr_bucket_init(PtrBucket *table, int cap){
  assert(cap != 0);
  table->cap = cap;
  table->len = 0;
  table->data = calloc(cap, sizeof(PtrBucketEntry));
  if(!table->data){
    fprintf(stderr, "couldn't initialize ptr_bucket \n");
    return -1;
  }
  return 0;
}

int ptr_bucket_quit(PtrBucket *table){
  table->cap = 0;
  table->len = 0;
  free(table->data);
  return 0;
}


int ptr_bucket_put(PtrBucket *table, uint64_t key, void *data) {
  if(table->len * 2 >= table->cap){
    int newcap = table->cap * 2;
    int status;
    PtrBucket newbucket;
    status = ptr_bucket_init(&newbucket, newcap);
    if(status < 0){
      fprintf(stderr, "couldn't put because resizing failed\n");
      return -1;
    }
    for(int i=0; i<table->cap; i++){
      PtrBucketEntry entry = table->data[i];
      if(entry.key){
        ptr_bucket_put(&newbucket, entry.key, entry.data);
      }
    }
    free(table->data);
    *table = newbucket;
  }

  uint64_t index = key % table->cap;
  PtrBucketEntry *entry = &table->data[index];
  const int maxTries = 50;
  int numTries = 0;
  while(entry->key != 0 && entry->key != key && numTries < maxTries){
    index = (index + 1) % table->cap;
    entry = &table->data[index];
  }
  if(numTries == maxTries){
    fprintf(stderr, "ptr_bucket put exceeded max tries %d\n", maxTries);
    return -1;
  }
  entry->key = key;
  entry->data = data;
  table->len++;
  return 0;
}

void *ptr_bucket_get(PtrBucket *table, uint64_t key) {
  uint64_t index = key % table->cap;
  PtrBucketEntry entry = table->data[index];
  const int maxTries = 50;
  int numTries = 0;
  while (entry.key != 0 && entry.key != key && numTries < maxTries) {
    index = (index + 1) % table->cap;
    entry = table->data[index];
    numTries++;
  }
  if (numTries == maxTries) {
    fprintf(stderr, "ptr_bucket put exceeded max tries %d\n", maxTries);
    return NULL;
  }
  if(entry.key == 0){return NULL;}
  return entry.data;
}

void ptr_bucket_print(PtrBucket *table, print_func print){
  for(int i=0; i<table->cap; i++){
    PtrBucketEntry entry = table->data[i];
    if(entry.key != 0){
      void *d = entry.data;
      print(d);
    }
  }
}

int ptr_bucket_init_from_bucket(PtrBucket *table, PtrBucket *already_existing){
  table->cap = already_existing->cap;
  table->data = calloc(already_existing->cap, sizeof(PtrBucketEntry));
  if(table->data < 0){
    fprintf(stderr, "couldn't allocate bucket from existing bucket\n");
    return -1;
  }
  memcpy(table->data, already_existing, sizeof(PtrBucketEntry) * already_existing->cap);
  table->len = already_existing->len;
  return 0;
}

int scope_init(Scope *scope){
  scope->parent = NULL;
  return ptr_bucket_init(&scope->vars, 10);
}

int scope_quit(Scope *scope){
  ptr_bucket_quit(&scope->vars);
  scope->parent = NULL;
  return 0;
}

void *scope_get(Scope *scope, uint64_t key) {
  void *result = ptr_bucket_get(&scope->vars, key);
  if(result == NULL){
    if(scope->parent == NULL){
      return NULL;
    }
    return scope_get(scope->parent, key);
  }
  return result;
}

int scope_put(Scope *scope, uint64_t key, void *data){
  return ptr_bucket_put(&scope->vars, key, data);
}

