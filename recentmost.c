/* `recentmost.c'
 *
 * Pick N most-recently-modified files from list on stdin.
 *
 * Origin: https://github.com/shadkam/recentmost
 * This fork: https://github.com/ConradHughes/recentmost
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#ifndef _WIN32
#include <time.h>
#else
#include <wchar.h>
#endif

#include "heap.h"

#define LINEBUFLEN 100
#define NoTimeArg "-noTime"
#define NullsNotNewlinesArg "-0"

int
getFileModTime(const char* filepath, rmU64* out_FileModTime)
{
#ifdef _WIN32
  FILETIME ft;
  HANDLE fh;
  fh = CreateFile(filepath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);

  if (fh == INVALID_HANDLE_VALUE) {
    fprintf(stderr, "error: could not CreateFile(%s)\n", filepath);
    return 1;
  }
  if (GetFileTime(fh, NULL, NULL, &ft) == 0) {
    fprintf(stderr, "error: could not GetFileTime(%s)\n", filepath);
    return 2;
  }
  *out_FileModTime = ft.dwHighDateTime;
  *out_FileModTime = (*out_FileModTime)<<32;
  *out_FileModTime |= ft.dwLowDateTime;
#else
  struct stat st;
  if (stat(filepath, &st)) {
    fprintf(stderr, "error: filepath '%s' not found", filepath);
    return 1;
  }
  *out_FileModTime = st.st_mtime;
#endif
  return 0;
}

void
offertoheap(Heap h, char *filepath)
{
  HeapElement elem = NULL;
  rmU64 fileModTime = 0;
  if (0!=getFileModTime(filepath, &fileModTime)) {
    /* error - but msg already shown */
    /* fprintf(stderr, "error: could not find or get mod time for filepath '%s'\n", filepath); */
  } else {
    elem = heap_newElement(fileModTime, filepath);
    if (!heap_push(h, elem)) {
      heap_freeElement(elem);
      elem = NULL;
    }
  }
}

void
processLines(Heap h, char eol)
{
  int c = EOF;
  size_t linelen = 0;
  char* filepath = NULL;
  size_t allocated = LINEBUFLEN+1;
  char *linebuf = (char*)malloc(sizeof(char)*allocated);
  while (1) {
    c = fgetc(stdin);
    if (c == EOF) {
      break;
    }
    if (linelen >= allocated) {
      allocated += LINEBUFLEN;
      linebuf = realloc(linebuf, sizeof(char) * allocated);
    }
    if (c == eol) {
      linebuf[linelen] = '\0';
      offertoheap(h, linebuf);
      memset(linebuf, '\0', sizeof(char)*allocated);
      linelen = 0;
      continue;
    }
    linebuf[linelen++] = (unsigned char)c;
  }
  free(filepath);
}

void
usage(char* progname)
{
  fprintf(stderr, "Usage:%s <filecount> [%s (print last modified time)]\n", progname, NoTimeArg);
  fprintf(stderr, "    [%s (\\0-separated inputs and outputs)]\n", NullsNotNewlinesArg);
}

int
checkInputs(int argc, char** argv, int* N, int* bPrintTime, char *eol)
{
  if (argc<2) {
    usage(argv[0]);
    return 0;
  }
  *N = atoi(argv[1]);
  if (*N==0) {
    fprintf(stderr, "Error: need numeric non zero value (%s) for filecount\n", argv[1]);
    usage(argv[0]);
    return 0;
  }
  *bPrintTime = 1;
  if (argc>2 && 0==strncmp(NoTimeArg, argv[2], sizeof(NoTimeArg))) {
    *bPrintTime = 0;
  }
  *eol = '\n';
  if (argc > 2 && ! strcmp(NullsNotNewlinesArg, argv[2])) {
    *eol = '\0';
    *bPrintTime = 0;
  }
  return 1;
}

void
fillTimeStr(char* timeStr, rmU64 time)
{
#ifdef _WIN32
  FILETIME ft;
  SYSTEMTIME st;
  ft.dwHighDateTime =  time>>32;
  ft.dwLowDateTime  =  time&0xFFFF;
  FileTimeToLocalFileTime( &ft, &ft );
  FileTimeToSystemTime   ( &ft, &st );
#else
  struct tm *tm1;
  struct stat st;
  memset(&st, '0', sizeof(st));
  st.st_mtime = time;
  tm1 = localtime(&st.st_mtime);
#endif
  sprintf(timeStr, "%.4d-%.2d-%.2d %.2d:%.2d:%.2d%s",
#ifdef _WIN32
      st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond
#else
      1900+tm1->tm_year, 1+tm1->tm_mon, tm1->tm_mday, tm1->tm_hour,
      tm1->tm_min, tm1->tm_sec
#endif
      , " ");
}

int
main(int argc, char** argv)
{
  int i;
  int N; 
  Heap hh;
  int bPrintTime = 0;
  char timeStr[] = "year-MM-dd hh:mm:ss "; /* just to take care of size */
  char eol = '\n';
  HeapElement popped;
  if (!checkInputs(argc, argv, &N, &bPrintTime, &eol)) {
    return -1;
  }
  memset(timeStr, '\0', sizeof(timeStr));
  hh = heap_alloc(N);
  processLines(hh, eol);
  for (i=0; i<N*10; i++) {
    popped = heap_pop(hh);
    if (!popped)
      break;
    if (bPrintTime) {
      fillTimeStr(timeStr, popped->modtime);
    }
    printf("%s%s%c", timeStr, popped->name, eol);
    heap_freeElement(popped);
  }
  heap_free(hh);
  return 0;
}
