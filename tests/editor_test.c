#define main editorMain
#include "../main.c"
#undef main

#include <assert.h>

static void resetEditor(void) {
  editorFindCallback("", '\x1b');
  for (int i = 0; i < E.numrows; i++) {
    editorFreeRow(&E.row[i]);
  }
  free(E.row);
  free(E.filename);
  memset(&E, 0, sizeof(E));
  E.screenrows = 20;
  E.screencols = 80;
}

static void addRow(char *text) {
  editorInsertRow(E.numrows, text, strlen(text));
}

static void assertRowIndexes(void) {
  for (int i = 0; i < E.numrows; i++) {
    assert(E.row[i].idx == i);
  }
}

static void assertHighlight(int row, int start, int end, int highlight) {
  for (int i = start; i < end; i++) {
    assert(E.row[row].hl[i] == highlight);
  }
}

// Feed real key input through the prompt without drawing into the test output.
static void withInput(const char *keys, void (*action)(void)) {
  int input[2];
  assert(pipe(input) == 0);
  size_t len = strlen(keys);
  assert(write(input[1], keys, len) == (ssize_t)len);
  assert(close(input[1]) == 0);
  int saved_stdin = dup(STDIN_FILENO);
  int saved_stdout = dup(STDOUT_FILENO);
  FILE *output = tmpfile();
  assert(saved_stdin != -1 && saved_stdout != -1 && output);
  assert(dup2(input[0], STDIN_FILENO) != -1);
  assert(close(input[0]) == 0);
  assert(dup2(fileno(output), STDOUT_FILENO) != -1);
  action();
  assert(dup2(saved_stdin, STDIN_FILENO) != -1);
  assert(dup2(saved_stdout, STDOUT_FILENO) != -1);
  assert(close(saved_stdin) == 0);
  assert(close(saved_stdout) == 0);
  assert(fclose(output) == 0);
}

static void testNewlinesAndDeletion(void) {
  resetEditor();
  addRow("ABCDEF");
  addRow("uvwxyz");
  E.cy = 1;
  E.cx = 2;
  editorInsertNewLine();
  assert(E.numrows == 3 && E.cy == 2 && E.cx == 0);
  assert(strcmp(E.row[1].chars, "uv") == 0);
  assert(strcmp(E.row[2].chars, "wxyz") == 0);
  assertRowIndexes();

  editorDelChar();
  assert(E.numrows == 2 && E.cy == 1 && E.cx == 2);
  assert(strcmp(E.row[1].chars, "uvwxyz") == 0);
  E.cx = 0;
  editorInsertNewLine();
  assert(E.row[1].size == 0 && E.cy == 2);
  E.cy = E.numrows;
  editorInsertNewLine();
  assert(E.numrows == 4 && E.cy == 4);
  assertRowIndexes();
}

static void testTabPositionsAndSearch(void) {
  resetEditor();
  addRow("a\tb\tc");
  assert(strcmp(E.row[0].render, "a       b       c") == 0);
  assert(editorRowRxToCx(&E.row[0], 8) == 2);
  assert(editorRowRxToCx(&E.row[0], 16) == 4);
  for (int cx = 0; cx <= E.row[0].size; cx++) {
    int rx = editorRowCxToRx(&E.row[0], cx);
    assert(editorRowRxToCx(&E.row[0], rx) == cx);
  }
  editorFindCallback("c", 'c');
  assert(E.cx == 4);

  resetEditor();
  addRow("match zero");
  addRow("match one");
  addRow("match two");
  editorFindCallback("match", 'm');
  assert(E.cy == 0);
  editorFindCallback("match", ARROW_UP);
  assert(E.cy == 2 && E.numrows == 3);
  editorFindCallback("match", ARROW_DOWN);
  assert(E.cy == 0 && E.numrows == 3);
  editorFindCallback("", BACKSPACE);
  assertHighlight(0, 0, 5, HL_NORMAL);
  editorFindCallback("missing", 'g');
  assert(E.numrows == 3);
  assertRowIndexes();
}

static void testPromptCancellation(void) {
  resetEditor();
  E.syntax = &HLDB[0];
  addRow("int match;");
  addRow("second line");
  addRow("third line");
  E.cx = 2;
  E.cy = 2;
  E.rowoff = 1;
  E.coloff = 1;
  withInput("match\x1b", editorFind);
  assert(E.cx == 2 && E.cy == 2 && E.rowoff == 1 && E.coloff == 1);
  assertHighlight(0, 0, 3, HL_KEYWORD2);
  assertHighlight(0, 4, 9, HL_NORMAL);
  withInput("match\r", editorFind);
  assert(E.cx == 4 && E.cy == 0);
  withInput("\x1b", editorSave);
  assert(E.filename == NULL);
  assert(strcmp(E.statusmsg, "Save aborted") == 0);
}

static void testMultilineComments(void) {
  resetEditor();
  E.syntax = &HLDB[0];
  addRow("/*");
  addRow("");
  addRow("int x;");
  addRow("*/ int y;");
  assert(E.row[0].hl_open_comment && E.row[1].hl_open_comment);
  assertHighlight(2, 0, 6, HL_MLCOMMENT);
  assertHighlight(3, 0, 2, HL_MLCOMMENT);
  assertHighlight(3, 3, 6, HL_KEYWORD2);
  assert(!E.row[3].hl_open_comment);

  editorRowDelChar(&E.row[0], 1);
  assert(!E.row[1].hl_open_comment);
  assertHighlight(2, 0, 3, HL_KEYWORD2);
  editorRowInsertChar(&E.row[0], 1, '*');
  assertHighlight(2, 0, 6, HL_MLCOMMENT);
  editorInsertRow(2, "*/", 2);
  assertHighlight(3, 0, 3, HL_KEYWORD2);
  editorDelRow(2);
  assertHighlight(2, 0, 6, HL_MLCOMMENT);
  // Removing the opening delimiter must update all affected following rows.
  editorDelRow(0);
  assertHighlight(1, 0, 3, HL_KEYWORD2);
  assertRowIndexes();

  resetEditor();
  E.syntax = &HLDB[0];
  addRow("/* // */ int x;");
  assertHighlight(0, 0, 8, HL_MLCOMMENT);
  assertHighlight(0, 9, 12, HL_KEYWORD2);
  E.cx = 6;
  editorInsertNewLine();
  assert(E.row[0].hl_open_comment);
  assertHighlight(1, 0, 2, HL_MLCOMMENT);
  assertHighlight(1, 3, 6, HL_KEYWORD2);
  editorDelChar();
  assertHighlight(0, 9, 12, HL_KEYWORD2);
  assertRowIndexes();
}

static void testSyntaxSelectionAndColors(void) {
  resetEditor();
  E.filename = strdup("test.c.c");
  editorSelectSyntaxHighlight();
  assert(E.syntax == &HLDB[0]);
  addRow("int x = 42; // comment");
  assertHighlight(0, 0, 3, HL_KEYWORD2);
  assertHighlight(0, 8, 10, HL_NUMBER);
  assertHighlight(0, 12, E.row[0].rsize, HL_COMMENT);
  addRow("\"/* // */\"");
  assertHighlight(1, 0, E.row[1].rsize, HL_STRING);
  assert(!E.row[1].hl_open_comment);
  addRow("\"a\\\"b\"");
  assertHighlight(2, 0, E.row[2].rsize, HL_STRING);

  E.screenrows = 1;
  struct abuf ab = ABUF_INIT;
  editorDrawRows(&ab);
  const char suffix[] = "\x1b[39m\x1b[K\r\n";
  assert(ab.len >= (int)sizeof(suffix) - 1);
  assert(memcmp(ab.b + ab.len - (sizeof(suffix) - 1), suffix,
                sizeof(suffix) - 1) == 0);
  abFree(&ab);

  free(E.filename);
  E.filename = strdup("test.txt");
  editorSelectSyntaxHighlight();
  assert(E.syntax == NULL);
  assertHighlight(0, 0, E.row[0].rsize, HL_NORMAL);
  // Short and empty rows can be visible while horizontally scrolled.
  addRow("");
  E.coloff = 200;
  E.screenrows = E.numrows;
  ab = (struct abuf)ABUF_INIT;
  editorDrawRows(&ab);
  abFree(&ab);
}

static void testFileRoundTrip(void) {
  resetEditor();
  char filename[] = "/tmp/kilo-regression-XXXXXX";
  int fd = mkstemp(filename);
  assert(fd != -1);
  assert(close(fd) == 0);
  E.filename = strdup(filename);
  addRow("first");
  addRow("");
  addRow("a\tb");
  editorSave();
  assert(E.dirty == 0);
  resetEditor();
  editorOpen(filename);
  assert(E.numrows == 3 && E.dirty == 0);
  assert(strcmp(E.row[0].chars, "first") == 0);
  assert(E.row[1].size == 0);
  assert(strcmp(E.row[2].chars, "a\tb") == 0);
  assertRowIndexes();
  assert(unlink(filename) == 0);
}

int main(void) {
  testNewlinesAndDeletion();
  testTabPositionsAndSearch();
  testPromptCancellation();
  testMultilineComments();
  testSyntaxSelectionAndColors();
  testFileRoundTrip();
  resetEditor();
  puts("All 6 editor regression groups passed.");
  return 0;
}
