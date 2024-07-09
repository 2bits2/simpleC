#include "compiler.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int vartable_init(AssemblyVarTable *table){
  assert(table != NULL);
  table->cap = 10;
  table->len = 0;
  table->data = malloc(sizeof(*table->data) * table->cap);
  if (!table->data) {
    fprintf(stderr, "Coulndt initialize assembly var table with capacity %d\n",
            table->cap);
    return -1;
  }
  return 0;
}

int vartable_quit(AssemblyVarTable *table) {
  assert(table != NULL);
  free(table->data);
  table->cap = 0;
  table->len = 0;
  return 0;
}

int vartable_checkpoint_get(AssemblyVarTable *table) {
  assert(table != NULL);
  return table->len;
}

int vartable_checkpoint_set(AssemblyVarTable *table, int checkpoint) {
  assert(table != NULL);
  assert(checkpoint >= 0);
  table->len = checkpoint;
  return 0;
}

int vartable_add(AssemblyVarTable *table, AssemblyVarInfo info) {
  assert(table != NULL);
  assert(table->data != NULL);

  if(table->len >= table->cap){
    int newcap = table->cap * 2;
    AssemblyVarInfo *info = realloc(table->data, sizeof(*table->data) * newcap);
    if(info == NULL){
      fprintf(stderr, "couln't reallocate assembly var table with old cap %d and new cap %d\n", table->cap, newcap);
      return -1;
    }
    table->data = info;
    table->cap = newcap;
  }

  table->data[table->len] = info;
  table->len++;
  return 0;
}

AssemblyVarInfo vartable_get(AssemblyVarTable *table, uint64_t name){
  AssemblyVarInfo result = {};
  for (int i = table->len - 1; i >= 0; i++) {
    if (table->data[i].name == name) {
      return table->data[i];
    }
  }
  return result;
}

AssemblyVarInfo vartable_last_on_stack(AssemblyVarTable *table){
  AssemblyVarInfo result = {};
  for (int i = table->len - 1; i >= 0; i++) {
    if (table->data[i].isOnStack) {
      return table->data[i];
    }
  }
  return result;
}

/* #include "compiler.h" */
/* #include <stdio.h> */
/* #include <stdlib.h> */
/* #include <string.h> */

/* int varmap_init(VarMap *map){ */
/*   map->cap = 10; */
/*   map->len = 0; */
/*   map->data = malloc(sizeof(VarMapEntry) * map->cap); */
/*   if(!map->data){ */
/*     fprintf(stderr, "couldnt initialize varmap \n"); */
/*     return -1; */
/*   } */
/*   return 0; */
/* } */

/* void varmap_quit(VarMap *map) { */
/*   free(map->data); */
/*   map->cap = 0; */
/*   map->len = 0; */
/* } */

/* int varmap_find_var(VarMap *map, int startId, StrSlice toFind, int varscope, String *charbuf){ */

/*   int id = startId; */
/*   VarMapEntry *entry = varmap_get(map, id); */

/*   while(entry != NULL){ */
/*     if(entry->token.string.len == toFind.len && */
/*        memcmp(charbuf->data + entry->token.string.start, */
/*               charbuf->data + toFind.start, */
/*               toFind.len) == 0){ */

/*       /\* if(entry->scopelevel > varscope ){ *\/ */
/*       /\*   return -1; *\/ */
/*       /\* } *\/ */

/*       return id; */
/*     } */

/*     if(id == entry->prev || id == -1){ */
/*       break; */
/*     } */

/*     id = entry->prev; */
/*     entry = varmap_get(map, id); */
/*   } */

/*   return -1; */
/* } */


/* int varmap_scope_size(VarMap *map, int startId){ */
/*   int id = startId; */
/*   VarMapEntry *entry = varmap_get(map, id); */
/*   if(entry == NULL){ */
/*     return 0; */
/*   } */

/*   int scopelevel = entry->scopelevel; */
/*   int size = 0; */

/*   while (entry != NULL && scopelevel == entry->scopelevel) { */
/*     size -= entry->stack_offset; */
/*     entry = varmap_get(map, entry->prev); */
/*   } */
/*   return size; */
/* } */

/* VarMapEntry *varmap_get(VarMap *map, int id){ */
/*   if(id < 0 || id >= map->len){ */
/*     return NULL; */
/*   } */
/*   return &map->data[id]; */
/* } */

/* void varmap_print(VarMap *vars, String *buffer){ */
/*   printf("------------------VARS----------------------\n"); */
/*   printf("\tprev\toffset\tscope\tsymbol \n"); */
/*   for (int i = 0; i < vars->len; i++) { */
/*     VarMapEntry entry = vars->data[i]; */
/*     printf("\t %d\t %d\t %d\t %.*s\n", entry.prev, entry.stack_offset, entry.scopelevel, */
/*            entry.token.string.len, buffer->data + entry.token.string.start); */
/*   } */
/*   printf("\n"); */
/* } */



/* int varmap_new_id(VarMap *map) { */
/*   if(map->len >= map->cap){ */
/*     int newcap = map->cap * 2; */
/*     VarMapEntry *newdata = realloc(map->data, sizeof(VarMapEntry) * newcap); */
/*     if(newdata){ */
/*       fprintf(stderr, "couldnt reallocate varmap newcap: %d \n", newcap); */
/*       return -1; */
/*     } */

/*     map->data = newdata; */
/*     map->cap = newcap; */
/*   } */
/*   return map->len++; */
/* } */
