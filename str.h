#ifndef MY_STR_H
#define MY_STR_H

//////// string /////////////////
typedef struct String {
  char *data;
  unsigned len;
  unsigned cap;
} String;

typedef struct {
  uint32_t start;
  uint32_t len;
} StrSlice;

int      str_init(String *buf, unsigned cap);
void     str_quit(String *buf);
int      str_push(String *buf, const char *input, int inputlen);
unsigned str_size(String *buf);
void     str_file_print(String buf, FILE *file);
int      str_file_read(String *buf, FILE *file);
String   str_from_slice(String str, StrSlice slice);
bool     str_equals(String str, String str2);
uint64_t str_hash(String *buf);

uint64_t str_slice_to_uint64(StrSlice slice);
StrSlice str_slice_from_uint64(uint64_t slice);

// string interner ///////
// this is there to refer to whole strings
// with a slice that is garuanteed to be unique

typedef enum StringInternerMeta {
  STR_INTERNER_SLOT_EMPTY = 0b10000000,
} StringInternerMeta;

typedef struct StringInternerEntry {
  StrSlice slice;
  uint64_t hash;
} StringInternerEntry;

typedef struct StringInterner {
  StringInternerEntry *entries;
  uint8_t *meta;
  unsigned cap;
  unsigned count;
  String string;
} StringInterner;

int      str_interner_init(StringInterner *interner, int cap);
int      str_interner_quit(StringInterner *interner);
String   str_interner_get(StringInterner  *interner, StrSlice slice);
StrSlice str_interner_put(StringInterner  *interner, String str);
void     str_interner_print(StringInterner *interner);

#endif
