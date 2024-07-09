#include "compiler.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>


////////////// string /////////////////////////////////
String str_from_slice(String str, StrSlice slice){
  str.data += slice.start;
  str.len = slice.len;
  str.cap = 0;
  return str;
}

bool str_equals(String str, String str2){
  return str.len == str2.len && strncmp(str.data, str2.data, str.len) == 0;
}


void str_file_print(String buf, FILE *file){
  fprintf(file, "%.*s", buf.len, buf.data);
}

int str_init(String *buf, unsigned cap){
  buf->len = 0;
  buf->cap = cap;
  buf->data = malloc(sizeof(char) * buf->cap);
  if(!buf->data){
    return -1;
  }
  return 0;
}

int str_file_read(String *buf, FILE *file) {
  int numread = 0;
  while( (numread = fread(buf->data + buf->len, sizeof(char), buf->cap - buf->len, file)) != 0){

    int newcap = buf->cap * 2;
    char *newdata = realloc(buf->data, newcap * 2);
    if(newdata == NULL){
      fprintf(stderr, "couldn't reallocate string memory\n");
      return -1;
    }
    buf->data = newdata;
    buf->cap = newcap;
    buf->len += numread;
  }
  return 0;
}

void str_quit(String *buf){
  free(buf->data);
  buf->len = 0;
  buf->cap = 0;
}

unsigned str_size(String *buf){
  return buf->len;
}

int str_push(String *buf, const char *input, int inputlen){
  if(buf->len + inputlen >= buf->cap){
    int newcap = buf->cap * 2 + inputlen;
    char *newdata = realloc(buf->data, newcap * 2);
    if (newdata == NULL) {
      fprintf(stderr, "couldn't reallocate string memory\n");
      return -1;
    }
    buf->data = newdata;
    buf->cap = newcap;
  }

  for(int i=0; i<inputlen; i++){
    buf->data[buf->len + i] = input[i];
  }
  buf->len += inputlen;
  return 0;
}

uint64_t str_hash(String *buf) {
  uint64_t hash = 0;
  for (int i = 0; i < buf->len; i++) {
    char c = buf->data[i];
    hash += c;
    hash += (hash << 10);
    hash ^= (hash >> 6);
  }
  hash += (hash << 3);
  hash ^= (hash >> 11);
  hash += (hash << 15);
  return hash;
}



uint64_t str_slice_to_uint64(StrSlice slice) {
  uint64_t result = slice.start;
  result = result << 32;
  result |= slice.len;
  return result;
}

StrSlice str_slice_from_uint64(uint64_t slice) {
  uint64_t mask = 1;
  mask = (mask << 33) - 1;
  StrSlice result = {};
  result.len = slice & mask;
  result.start = (slice >> 32);
  return result;
}



//////////// string interning //////////////////////////////

int str_interner_init(StringInterner *interner, int cap) {
  interner->cap = cap;
  interner->count = 0;
  interner->meta = malloc(sizeof(*interner->meta) * interner->cap);
  if (!interner->meta) {
    fprintf(stderr, "couldn't initialize stringinterner meta data (cap:%d)\n",
            interner->cap);
    return -1;
  }
  for(int i=0; i<interner->cap; i++){
    interner->meta[i] = STR_INTERNER_SLOT_EMPTY;
  }
  interner->entries = malloc(sizeof(*interner->entries) * interner->cap);
  if (!interner->entries) {
    fprintf(stderr, "couldn't initialize stringinterner entries\n");
    free(interner->meta);
    return -1;
  }
  return str_init(&interner->string, 1024);
}

unsigned hash_index(uint64_t hash){ return hash >> 7; }
unsigned hash_meta(uint64_t hash) { return hash & ((1 << 7) - 1); }


StrSlice str_interner_set(StringInterner *interner, String str) {
  uint64_t hash  = str_hash(&str);
  uint8_t  meta  = hash_meta(hash);
  unsigned index = hash_index(hash) % interner->cap;
  int      attempts = 0;
  int      maxAttempts = 40;
  while(attempts < maxAttempts){
    uint8_t current_meta = interner->meta[index];
    if (current_meta == meta){
      StringInternerEntry *entry = &interner->entries[index];
      String istr = str_from_slice(interner->string, entry->slice);
      if (str_equals(istr, str)) {
        return entry->slice;
      }
    } else if (current_meta == STR_INTERNER_SLOT_EMPTY) {

      StringInternerEntry *entry = &interner->entries[index];
      int start = interner->string.len;
      int len = str.len;
      str_push(&interner->string, str.data, str.len);
      entry->slice.start = start;
      entry->slice.len = len;
      entry->hash = hash;
      interner->meta[index] = meta;
      interner->count++;
      return entry->slice;
    }
    attempts += 1;
    index = (index + 1) % interner->cap;
  }
  fprintf(stderr, "string interner put slot exceeded %d attemps\n", attempts);
  StrSlice result = {};
  return result;
}


int str_interner_resize(StringInterner *interner) {
  int status = 0;
  StringInterner newinterner;
  int newcap = interner->cap * 2;
  printf("resizing interner newcap %d \n", newcap);
  status = str_interner_init(&newinterner, newcap);
  if (status < 0) {
    fprintf(stderr, "couldnt resize string interner\n");
    return -2;
  }
  str_quit(&newinterner.string);
  int maxAttempts = 30;
  for (int i = 0; i < interner->cap; i++) {
    if (interner->meta[i] != STR_INTERNER_SLOT_EMPTY) {
      StringInternerEntry entry = interner->entries[i];
      uint64_t hash = entry.hash;
      uint8_t  meta  = hash_meta(hash);
      unsigned index = hash_index(hash) % newcap;

      int numAttempts = 0;
      while(newinterner.meta[index] != STR_INTERNER_SLOT_EMPTY && numAttempts < maxAttempts){
        index = (index + 1) % newcap;
        numAttempts++;
      }
      if(numAttempts >= maxAttempts){
        fprintf(stderr, "resize numAttempts %d exceeded maxAttemtps %d\n", numAttempts, maxAttempts);
      }
      newinterner.meta[index] = meta;
      newinterner.entries[index] = entry;
    }
  }
  free(interner->meta);
  free(interner->entries);
  interner->cap = newcap;
  interner->entries = newinterner.entries;
  interner->meta = newinterner.meta;
  return 0;
}


String str_interner_get(StringInterner *interner, StrSlice slice){
  return str_from_slice(interner->string, slice);
}

StrSlice str_interner_put(StringInterner *interner, String str) {
  if (interner->count + 10 >= interner->cap) {
    str_interner_resize(interner);
  }
  return str_interner_set(interner, str);
}

void str_interner_print(StringInterner *interner) {
  for (int i = 0; i < interner->cap; i++) {
    uint8_t meta = interner->meta[i];
    if (meta == STR_INTERNER_SLOT_EMPTY) {
      printf("%d\t.. \n", i);
    } else {
      StringInternerEntry entry = interner->entries[i];
      printf("%d\tmeta:%d hash:%lu start:%d len:%d %.*s \n", i, meta,
             entry.hash, entry.slice.start, entry.slice.len,
             entry.slice.len, interner->string.data + entry.slice.start
             );
    }
  }
  printf("--interner buffer ---\n");
  str_file_print(interner->string, stdout);
  printf("\n");
}

int str_interner_quit(StringInterner *interner) {
  free(interner->meta);
  free(interner->entries);
  str_quit(&interner->string);
  interner->cap = 0;
  interner->count = 0;
  return 0;
}
