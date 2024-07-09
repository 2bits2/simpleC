#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include "compiler.h"

int offset_align(int offset, int multiple) {
  if (offset % multiple == 0) {
    return offset;
  }
  return ((offset / multiple) + 1) * multiple;
}

int natural_alignment_size(int *offsets, int startoffset, int *sizes, int len) {
  for (int i = 0; i < len; i++) {
    int size = sizes[i];
    startoffset = offset_align(startoffset, size);
    offsets[i] = startoffset;
    startoffset += size;
  }
  return startoffset;
}


int main(int argc, char *argv[]) {

  int status = 0;

  if (argc != 2) {
    fprintf(stderr, "wrong number of arguments. need a c file as input\n");
    exit(1);
  }

  FileManager files;
  status = filemanager_init(&files);
  if (status < 0) {
    fprintf(stderr, "couldn't initialize file manager\n");
    return status;
  }

  Parser parser;
  parser_init(&parser);

  String input_file_name    = {
    .data = argv[1],
    .cap = 0,
    .len = strlen(argv[1])
  };

  int    input_file_id      = filemanager_load_file(&files, &input_file_name);
  String input_file_content = filemanager_get_content(&files, input_file_id);
  char   *filename = "myassembly.s";
  String out_file_name = {.data= filename, .cap = 0, .len=strlen(filename)};

  parser_parse(&parser, input_file_content, input_file_id, &files);
  parser_dump_assembly(&parser, stdout);

  FILE *file = fopen(filename, "w");
  if(!file){
    fprintf(stderr, "couldnt open %s for writing\n", filename);
    exit(2);
  }

  parser_dump_assembly(&parser, file);
  fclose(file);
  filemanager_quit(&files);

  StrSlice t = {.len = 3, .start = 20};
  uint64_t result = str_slice_to_uint64(t);
  StrSlice s = str_slice_from_uint64(result);

  return 0;
}
