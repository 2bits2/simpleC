#include "compiler.h"
#include <stdlib.h>
#include <string.h>

int filemanager_init(FileManager *manager);
void filemanager_quit(FileManager *manager);

int filemanager_get_id(FileManager *manager, const String *filename);
int filemanager_load_file(FileManager *manager, const String *filename);


int filemanager_init(FileManager *manager){
  manager->cap = 10;
  manager->filenames = malloc(sizeof(*manager->filenames) * manager->cap);
  manager->content = malloc(sizeof(*manager->content) * manager->cap);
  manager->len = 0;
  if(!manager->filenames){
    fprintf(stderr, "couldnt initialize filemanager - storing filenames with cap %d failed\n", manager->cap);
    exit(1);
    return -1;
  }
  if(!manager->content){
    fprintf(stderr, "couldnt initialize filemanager strings with cap %d\n", manager->cap);
    exit(2);
    return -2;
  }
  return 0;
}

void filemanager_quit(FileManager *manager){
  for(int i=0; i<manager->len; i++){
    str_quit(&manager->content[i]);
    str_quit(&manager->filenames[i]);
  }
  free(manager->content);
  free(manager->filenames);
  manager->cap = 0;
  manager->len = 0;
}

int filemanager_get_id(FileManager *manager, const String *filename){
  int already_loaded = -1;
  for (int i = 0; i < manager->len; i++) {
    if (manager->filenames[i].len == filename->len &&
        strncmp(filename->data, manager->filenames[i].data, filename->len) == 0) {
      already_loaded = i;
    }
  }
  return already_loaded;
}

String filemanager_get_content(FileManager *manager, int file_id){
  if(file_id < 0 || file_id >= manager->len){
    String result = {};
    return result;
  }
  return manager->content[file_id];
}

String filemanager_get_filename(FileManager *manager, int file_id) {
  if (file_id < 0 || file_id >= manager->len) {
    String result = {};
    return result;
  }
  return manager->filenames[file_id];
}

int filemanager_load_file(FileManager *manager, const String *filename) {
  if (manager->len >= manager->cap) {
    int newcap = manager->cap;
    String *newcontent =
        realloc(manager->content, sizeof(*manager->content) * newcap);
    if (!newcontent) {
      fprintf(stderr,
              "couldnt load file contents %.*s in filemanager "
              "with new filemanager capacity %d\n",
              filename->len, filename->data, newcap);
      exit(3);
      return -1;
    }

    String *newfilenames =
        realloc(manager->content, sizeof(*manager->filenames) * newcap);
    if(!newfilenames){
      fprintf(stderr,
              "couldnt load file names %.*s in "
              "filemanager with new filemanager capacity %d\n",
              filename->len, filename->data, newcap);
      exit(4);
      return -2;
    }

    manager->content = newcontent;
    manager->filenames = newfilenames;
    manager->cap = newcap;
  }

  int newid = manager->len++;

  String *newcontent = &manager->content[newid];
  String *newfilename = &manager->filenames[newid];

  str_init(newfilename, 80);
  str_push(newfilename, filename->data, filename->len);

  if (filename->data[filename->len - 1] != '\0') {
    str_push(newfilename, "\0", 1);
    newfilename->len--;
  }

  str_init(newcontent, 100);

  FILE *file = fopen(newfilename->data, "r");
  if(!file){
    fprintf(stderr, "couldnt open file %.*s \n", newfilename->len, newfilename->data);
    return -1;
  }

  str_file_read(newcontent, file);
  fclose(file);
  return newid;
}
