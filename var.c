#include "compiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int varmap_init(VarMap *map){
  map->cap = 10;
  map->len = 0;
  map->data = malloc(sizeof(VarMapEntry) * map->cap);
  if(!map->data){
    fprintf(stderr, "couldnt initialize varmap \n");
    return -1;
  }
  return 0;
}

void varmap_quit(VarMap *map) {
  free(map->data);
  map->cap = 0;
  map->len = 0;
}

int varmap_find_var(VarMap *map, int startId, StrSlice toFind, int varscope, String *charbuf){

  int id = startId;
  VarMapEntry *entry = varmap_get(map, id);

  while(entry != NULL){
    if(entry->view.len == toFind.len &&
       memcmp(charbuf->data + entry->view.start,
              charbuf->data + toFind.start,
              toFind.len) == 0){

      /* if(entry->scopelevel > varscope ){ */
      /*   return -1; */
      /* } */

      return id;
    }

    if(id == entry->prev || id == -1){
      break;
    }

    id = entry->prev;
    entry = varmap_get(map, id);
  }

  return -1;
}


int varmap_scope_size(VarMap *map, int startId){
  int id = startId;
  VarMapEntry *entry = varmap_get(map, id);
  if(entry == NULL){
    return 0;
  }

  int scopelevel = entry->scopelevel;
  int size = 0;

  while (entry != NULL && scopelevel == entry->scopelevel) {
    size -= entry->stack_offset;
    entry = varmap_get(map, entry->prev);
  }
  return size;
}

VarMapEntry *varmap_get(VarMap *map, int id){
  if(id < 0 || id >= map->len){
    return NULL;
  }
  return &map->data[id];
}

void varmap_print(VarMap *vars, String *buffer){
  printf("------------------VARS----------------------\n");
  printf("\tprev\toffset\tscope\tsymbol \n");
  for (int i = 0; i < vars->len; i++) {
    VarMapEntry entry = vars->data[i];
    printf("\t %d\t %d\t %d\t %.*s\n", entry.prev, entry.stack_offset, entry.scopelevel,
           entry.view.len, buffer->data + entry.view.start);
  }
  printf("\n");
}



int varmap_new_id(VarMap *map) {
  if(map->len >= map->cap){
    int newcap = map->cap * 2;
    VarMapEntry *newdata = realloc(map->data, sizeof(VarMapEntry) * newcap);
    if(newdata){
      fprintf(stderr, "couldnt reallocate varmap newcap: %d \n", newcap);
      return -1;
    }

    map->data = newdata;
    map->cap = newcap;
  }
  return map->len++;
}
