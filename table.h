#ifndef MY_PTR_MAP_H
#define MY_PTR_MAP_H

typedef void (*print_func)(void *d);

typedef struct PtrBucketEntry {
  void *data;
  uint64_t key;
} PtrBucketEntry;

typedef struct PtrBucket {
  PtrBucketEntry *data;
  int cap;
  int len;
} PtrBucket;

int ptr_bucket_init(PtrBucket *table, int cap);
int ptr_bucket_quit(PtrBucket *table);
int ptr_bucket_init_from_bucket(PtrBucket *table, PtrBucket *already_existing);
void *ptr_bucket_get(PtrBucket *table, uint64_t key);
int  ptr_bucket_put(PtrBucket *table, uint64_t key, void *data);
void ptr_bucket_print(PtrBucket *table, print_func pr);

#endif
