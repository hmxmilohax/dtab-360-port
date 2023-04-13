// by onyxite + gpt, public domain

#include <vector>
#include <string>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <stdint.h>


struct Tree;

struct Chunk {
  enum Tag {
    INT = 0x0,
    FLOAT = 0x1,
    VAR = 0x2,
    SYM = 0x5,
    UNHANDLED = 0x6,
    IFDEF = 0x7,
    ELSE = 0x8,
    ENDIF = 0x9,
    PARENS = 0x10,
    BRACES = 0x11,
    STRING = 0x12,
    BRACKETS = 0x13,
    DEFINE = 0x20,
    INCLUDE = 0x21,
    MERGE = 0x22,
    IFNDEF = 0x23,
    AUTORUN = 0x24,
    UNDEF = 0x25,
  } tag;
  union {
    int32_t i;
    float f;
    struct {
      char* data;
      size_t size;
    } str;
    Tree* subtree;
  };

  Chunk() : tag(INT), i(0) {} // c++03: Add a constructor to initialize the union
};

struct Tree {
  uint32_t nodeID;
  std::vector<Chunk> treeChunks;
};

struct DTA {
  uint8_t byteZero;
  Tree topTree;
};

// gpt wrote this (and edited to c++03)
void renumberTree(uint32_t& newID, Tree& tree) {
  tree.nodeID = newID++;
  for (std::vector<Chunk>::iterator it = tree.treeChunks.begin(); it != tree.treeChunks.end(); ++it) {
    Chunk& chunk = *it;
    if (chunk.tag == Chunk::PARENS || chunk.tag == Chunk::BRACES || chunk.tag == Chunk::BRACKETS) {
      renumberTree(newID, *chunk.subtree);
    }
  }
}

// gpt wrote this
void renumberFrom(uint32_t w, DTA& dta) {
  uint32_t newID = w;
  renumberTree(newID, dta.topTree);
}

// gpt wrote this
void skipWhitespaceAndComments(char* str, size_t len, size_t* cursor) {
  // Check if the input parameters are valid
  if (str == NULL || cursor == NULL) {
    return;
  }

  bool inComment = false;

  // Loop through the string starting from the cursor position
  for (; *cursor < len; ++(*cursor)) {
    char currentChar = str[*cursor];

    if (inComment) {
      // If we're in a comment and we find a newline, exit the comment
      if (currentChar == '\n') {
        inComment = false;
      }
    } else {
      // If the current character starts a comment, enter the comment
      if ((currentChar == ';' || (currentChar == '/' && *cursor + 1 < len && str[*cursor + 1] == '/'))) {
        inComment = true;
      }
      // If the current character is not a whitespace or comment start, stop advancing the cursor
      else if (!isspace((unsigned char)currentChar)) {
        break;
      }
    }
  }
}

// gpt wrote this
bool isBracket(char c) {
  return c == '[' || c == ']' || c == '(' || c == ')' || c == '{' || c == '}';
}

// gpt wrote this
void parseSymbol(char* str, size_t len, size_t* cursor) {
  // Check if the input parameters are valid
  if (str == NULL || cursor == NULL) {
    return;
  }

  // Loop through the string starting from the cursor position
  for (; *cursor < len; ++(*cursor)) {
    char currentChar = str[*cursor];

    // If the current character is a whitespace, a bracket, or a null terminator, stop advancing the cursor
    if (isspace((unsigned char)currentChar) || isBracket(currentChar) || currentChar == '\0') {
      break;
    }
  }
}

void parseSingleQuotedSymbol(char* str, size_t len, size_t* cursor) {
  for (; *cursor < len; ++(*cursor)) {
    char currentChar = str[*cursor];
    if (currentChar == '\'' || currentChar == '\0') {
      break;
    }
  }
}

void parseDoubleQuotedString(char* str, size_t len, size_t* cursor) {
  for (; *cursor < len; ++(*cursor)) {
    char currentChar = str[*cursor];
    if (currentChar == '"' || currentChar == '\0') {
      break;
    }
  }
}

// gpt wrote this
bool parseInteger(char* str, size_t len, int32_t* out) {
  if (len == 0) {
    return false;
  }

  int32_t result = 0;
  bool negative = false;
  size_t start = 0;

  if (str[0] == '-') {
    negative = true;
    start = 1;
  } else if (str[0] == '+') {
    start = 1;
  }

  if (start == len) {
    return false;
  }

  for (size_t i = start; i < len; ++i) {
    char c = str[i];
    if (c < '0' || c > '9') {
      return false;
    }

    result = result * 10 + (c - '0');
  }

  if (negative) {
    result = -result;
  }

  *out = result;
  return true;
}

// gpt wrote this
bool parseFloat(char* str, size_t len, float* out) {
    if (len == 0) {
        return false;
    }

    bool negative = false;
    size_t start = 0;

    if (str[0] == '-') {
        negative = true;
        start = 1;
    } else if (str[0] == '+') {
        start = 1;
    }

    if (start == len) {
        return false;
    }

    float result = 0.0;
    size_t i = start;
    for (; i < len && str[i] != '.'; ++i) {
        char c = str[i];
        if (c < '0' || c > '9') {
            return false;
        }
        result = result * 10 + (c - '0');
    }

    if (i == len || str[i] != '.') {
        return false;
    }
    ++i;

    float fraction = 1.0;
    for (; i < len && str[i] != 'e' && str[i] != 'E'; ++i) {
        char c = str[i];
        if (c < '0' || c > '9') {
            return false;
        }
        fraction /= 10;
        result += fraction * (c - '0');
    }

    if (i < len && (str[i] == 'e' || str[i] == 'E')) {
        ++i;
        int32_t exponent = 0;
        if (!parseInteger(str + i, len - i, &exponent)) {
            return false;
        }
        result *= pow(10.0f, exponent);
    }

    if (negative) {
        result = -result;
    }

    *out = result;
    return true;
}

void parseTree(char* str, size_t len, size_t* cursor, Tree* out);

bool parseChunk(char* str, size_t len, size_t* cursor, Chunk* out) {
  // first skip to next real char
  skipWhitespaceAndComments(str, len, cursor);
  // then see if we can parse a chunk
  if (*cursor >= len) {
    return false; // end of string
  }
  char c = str[*cursor];
  if (c == '\'') {

    (*cursor)++;
    size_t cursorStart = *cursor;
    parseSingleQuotedSymbol(str, len, cursor);
    size_t cursorEnd = *cursor;
    (*cursor)++;
    out->tag = Chunk::SYM;
    out->str.data = str + cursorStart;
    out->str.size = cursorEnd - cursorStart;
    return true;

  } else if (c == '"') {

    (*cursor)++;
    size_t cursorStart = *cursor;
    parseDoubleQuotedString(str, len, cursor);
    size_t cursorEnd = *cursor;
    (*cursor)++;
    out->tag = Chunk::STRING;
    out->str.data = str + cursorStart;
    out->str.size = cursorEnd - cursorStart;
    return true;

  } else if (c == '(') {

    (*cursor)++;
    out->tag = Chunk::PARENS;
    out->subtree = new Tree;
    parseTree(str, len, cursor, out->subtree);
    if (*cursor >= len || str[*cursor] != ')') {
      printf("parse error!!!\n");
    }
    (*cursor)++;
    return true;

  } else if (c == '[') {

    (*cursor)++;
    out->tag = Chunk::BRACKETS;
    out->subtree = new Tree;
    parseTree(str, len, cursor, out->subtree);
    if (*cursor >= len || str[*cursor] != ']') {
      printf("parse error!!!\n");
    }
    (*cursor)++;
    return true;

  } else if (c == '{') {

    (*cursor)++;
    out->tag = Chunk::BRACES;
    out->subtree = new Tree;
    parseTree(str, len, cursor, out->subtree);
    if (*cursor >= len || str[*cursor] != '}') {
      printf("parse error!!!\n");
    }
    (*cursor)++;
    return true;

  } else if (c == '$') {

    (*cursor)++;
    size_t cursorStart = *cursor;
    parseSymbol(str, len, cursor);
    size_t cursorEnd = *cursor;
    out->tag = Chunk::VAR;
    out->str.data = str + cursorStart;
    out->str.size = cursorEnd - cursorStart;
    return true;

  } else if (c == '#') {

    (*cursor)++;
    size_t cursorStart = *cursor;
    parseSymbol(str, len, cursor);
    size_t cursorEnd = *cursor;
    char* commandStart = str + cursorStart;
    size_t commandLength = cursorEnd - cursorStart;

    if (commandLength == 5 && strncmp(commandStart, "ifdef", 5) == 0) {
      Chunk param;
      parseChunk(str, len, cursor, &param);
      out->tag = Chunk::IFDEF;
      out->str = param.str;
    } else if (commandLength == 6 && strncmp(commandStart, "define", 6) == 0) {
      Chunk param;
      parseChunk(str, len, cursor, &param);
      out->tag = Chunk::DEFINE;
      out->str = param.str;
    } else if (commandLength == 7 && strncmp(commandStart, "include", 7) == 0) {
      Chunk param;
      parseChunk(str, len, cursor, &param);
      out->tag = Chunk::INCLUDE;
      out->str = param.str;
    } else if (commandLength == 5 && strncmp(commandStart, "merge", 5) == 0) {
      Chunk param;
      parseChunk(str, len, cursor, &param);
      out->tag = Chunk::MERGE;
      out->str = param.str;
    } else if (commandLength == 6 && strncmp(commandStart, "ifndef", 6) == 0) {
      Chunk param;
      parseChunk(str, len, cursor, &param);
      out->tag = Chunk::IFNDEF;
      out->str = param.str;
    } else if (commandLength == 5 && strncmp(commandStart, "undef", 5) == 0) {
      Chunk param;
      parseChunk(str, len, cursor, &param);
      out->tag = Chunk::UNDEF;
      out->str = param.str;
    } else if (commandLength == 4 && strncmp(commandStart, "else", 4) == 0) {
      out->tag = Chunk::ELSE;
    } else if (commandLength == 5 && strncmp(commandStart, "endif", 5) == 0) {
      out->tag = Chunk::ENDIF;
    } else if (commandLength == 7 && strncmp(commandStart, "autorun", 7) == 0) {
      out->tag = Chunk::AUTORUN;
    }
    return true;

  } else if (c == ')' || c == ']' || c == '}') {

    // end of some tree
    return false;

  } else {

    size_t cursorStart = *cursor;
    parseSymbol(str, len, cursor);
    size_t cursorEnd = *cursor;
    char* symbolStart = str + cursorStart;
    size_t symbolLength = cursorEnd - cursorStart;

    if (parseInteger(symbolStart, symbolLength, &out->i)) {
      out->tag = Chunk::INT;
    } else if (parseFloat(symbolStart, symbolLength, &out->f)) {
      out->tag = Chunk::FLOAT;
    } else if (symbolLength == 14 && strncmp(symbolStart, "kDataUnhandled", 14) == 0) {
      out->tag = Chunk::UNHANDLED;
    } else {
      out->tag = Chunk::SYM;
      out->str.data = symbolStart;
      out->str.size = symbolLength;
    }
    return true;

  }
}

void parseTree(char* str, size_t len, size_t* cursor, Tree* out) {
  out->nodeID = 0;
  while (true) {
    Chunk newChunk;
    if (parseChunk(str, len, cursor, &newChunk)) {
      out->treeChunks.push_back(newChunk);
    } else {
      return;
    }
  }
}

void debugPrintTree(Tree* tree);

void debugPrintChunk(Chunk chunk) {
  switch (chunk.tag) {
  case Chunk::SYM:
    printf("'%.*s'", (int) chunk.str.size, chunk.str.data);
    break;
  case Chunk::STRING:
    printf("\"%.*s\"", (int) chunk.str.size, chunk.str.data);
    break;
  case Chunk::VAR:
    printf("$%.*s", (int) chunk.str.size, chunk.str.data);
    break;
  case Chunk::PARENS:
    printf("(");
    debugPrintTree(chunk.subtree);
    printf(")");
    break;
  case Chunk::BRACES:
    printf("{");
    debugPrintTree(chunk.subtree);
    printf("}");
    break;
  case Chunk::BRACKETS:
    printf("[");
    debugPrintTree(chunk.subtree);
    printf("]");
    break;
  case Chunk::INT:
    printf("%d", (int) chunk.i);
    break;
  case Chunk::FLOAT:
    printf("%f", (double) chunk.f);
    break;
  case Chunk::IFDEF:
    printf("#ifdef '%.*s'\n", (int) chunk.str.size, chunk.str.data);
    break;
  case Chunk::DEFINE:
    printf("#define '%.*s'\n", (int) chunk.str.size, chunk.str.data);
    break;
  case Chunk::INCLUDE:
    printf("#include '%.*s'\n", (int) chunk.str.size, chunk.str.data);
    break;
  case Chunk::MERGE:
    printf("#merge '%.*s'\n", (int) chunk.str.size, chunk.str.data);
    break;
  case Chunk::IFNDEF:
    printf("#ifndef '%.*s'\n", (int) chunk.str.size, chunk.str.data);
    break;
  case Chunk::UNDEF:
    printf("#undef '%.*s'\n", (int) chunk.str.size, chunk.str.data);
    break;
  case Chunk::ELSE:
    printf("#else\n");
    break;
  case Chunk::ENDIF:
    printf("#endif\n");
    break;
  case Chunk::AUTORUN:
    printf("#autorun\n");
    break;
  case Chunk::UNHANDLED:
    printf("kDataUnhandled");
    break;
  default:
    printf("UNKNOWN");
    break;
  }
}

void debugPrintTree(Tree* tree) {
  size_t i = 0;
  size_t len = tree->treeChunks.size();
  for (std::vector<Chunk>::iterator it = tree->treeChunks.begin(); it != tree->treeChunks.end(); ++it) {
    Chunk& chunk = *it;
    debugPrintChunk(chunk);
    if (i + 1 < len) {
      printf(" ");
    }
    i++;
  }
}

void writeU16LE(FILE* out, uint16_t n) {
  uint8_t bytes[2];
  bytes[0] = n;
  bytes[1] = n >> 8;
  fwrite(&bytes[0], 1, 2, out);
}

void writeU32LE(FILE *out, uint32_t n) {
  uint8_t bytes[4];
  bytes[0] = n;
  bytes[1] = n >> 8;
  bytes[2] = n >> 16;
  bytes[3] = n >> 24;
  fwrite(&bytes[0], 1, 4, out);
}

// gpt wrote this
bool isLittleEndian() {
  uint32_t test = 1;
  return *((uint8_t *) &test) == 1;
}

// gpt wrote this
void writeFloatLE(FILE *out, float n) {
  uint32_t floatAsInt;
  memcpy(&floatAsInt, &n, sizeof(uint32_t));

  if (isLittleEndian()) {
    fputc((floatAsInt >> 0) & 0xFF, out);
    fputc((floatAsInt >> 8) & 0xFF, out);
    fputc((floatAsInt >> 16) & 0xFF, out);
    fputc((floatAsInt >> 24) & 0xFF, out);
  } else {
    fputc((floatAsInt >> 24) & 0xFF, out);
    fputc((floatAsInt >> 16) & 0xFF, out);
    fputc((floatAsInt >> 8) & 0xFF, out);
    fputc((floatAsInt >> 0) & 0xFF, out);
  }
}

void writeDoubleQuotedString(FILE *out, char* data, size_t size) {
  int numBackslashQ = 0;
  for (size_t i = 0; i < size - 1; i++) {
    if (data[i] == '\\' && data[i + 1] == 'q') {
      numBackslashQ++;
    }
  }
  // each \q will become 1 char, so subtract how many \q there are from the size
  writeU32LE(out, (uint32_t) (size - numBackslashQ));

  for (size_t i = 0; i < size; i++) {
    uint8_t ch;
    if (data[i] == '\\' && i != size - 1 && data[i + 1] == 'q') {
      ch = '"';
      i++;
    } else {
      ch = data[i];
    }
    fwrite(&ch, 1, 1, out);
  }

}

void writeDTBTree(FILE* out, Tree& tree);

void writeDTBChunk(FILE* out, Chunk& chunk) {
  writeU32LE(out, (uint32_t) chunk.tag);
  switch (chunk.tag) {
  case Chunk::INT:
    writeU32LE(out, (uint32_t) chunk.i);
    break;
  case Chunk::FLOAT:
    writeFloatLE(out, chunk.f);
    break;
  case Chunk::SYM:
  case Chunk::VAR:
  case Chunk::IFDEF:
  case Chunk::DEFINE:
  case Chunk::INCLUDE:
  case Chunk::MERGE:
  case Chunk::IFNDEF:
  case Chunk::UNDEF:
    writeU32LE(out, (uint32_t) chunk.str.size);
    fwrite(chunk.str.data, 1, chunk.str.size, out);
    break;
  case Chunk::STRING:
    writeDoubleQuotedString(out, chunk.str.data, chunk.str.size);
    break;
  case Chunk::PARENS:
  case Chunk::BRACES:
  case Chunk::BRACKETS:
    writeDTBTree(out, *chunk.subtree);
    break;
  case Chunk::UNHANDLED:
  case Chunk::ELSE:
  case Chunk::ENDIF:
  case Chunk::AUTORUN:
    writeU32LE(out, 0);
    break;
  }
}

void writeDTBTree(FILE* out, Tree& tree) {
  writeU16LE(out, (uint16_t) tree.treeChunks.size());
  writeU32LE(out, tree.nodeID);
  for (std::vector<Chunk>::iterator it = tree.treeChunks.begin(); it != tree.treeChunks.end(); ++it) {
    Chunk& chunk = *it;
    writeDTBChunk(out, chunk);
  }
}

void writeDTB(FILE* out, DTA& dta) {
  fwrite(&dta.byteZero, 1, 1, out);
  writeDTBTree(out, dta.topTree);
}

// gpt wrote the file load part
int FileLoad(int argc, char const *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s in.dta out.dtb\n", argv[0]);
    return 1;
  }

  const char *file_path = argv[1];
  FILE *file = fopen(file_path, "rb");
  if (file == NULL) {
    perror("Error opening file");
    return 2;
  }

  // Determine the file size
  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  // Allocate a buffer to hold the file content
  char *buffer = (char*) malloc(file_size + 1);
  if (buffer == NULL) {
    fprintf(stderr, "Error allocating memory\n");
    fclose(file);
    return 3;
  }

  // Read the file content into the buffer
  size_t bytes_read = fread(buffer, 1, file_size, file);
  fclose(file);

  if ((int) bytes_read != (int) file_size) {
    fprintf(stderr, "Error reading file\n");
    free(buffer);
    return 4;
  }

  // Null-terminate the buffer to handle it as a string
  buffer[file_size] = '\0';

  DTA dta;
  dta.byteZero = 0;
  size_t cursor = 0;
  parseTree(buffer, file_size, &cursor, &dta.topTree);

  debugPrintTree(&dta.topTree);
  printf("\n");

  bool success = false;
  FILE *file_out = fopen(argv[2], "wb");
  if (file_out == NULL) {
    perror("Error opening output file");
  } else {
    renumberFrom(1, dta);
    writeDTB(file_out, dta);
  }

  free(buffer);
  return (success ? 0 : 1);
}
