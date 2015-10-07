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
#include <unistd.h>
#include <time.h>
typedef unsigned long long rmU64;
#else
#include <windows.h>
#include <wchar.h>
typedef unsigned __int64 rmU64;
#endif

#define LINEBUFLEN 100
#define NoTimeArg "-noTime"
#define NullsNotNewlinesArg "-0"

/**  Heap implementation  ****************************************************/

/* Thanks to https://gist.github.com/martinkunev/1365481
 * from which basic heap related code was originally taken */
struct filetimestamp {
  char* name;
  rmU64 modtime;
  int   id;
};
typedef struct filetimestamp* HeapElement;

struct heap {
  size_t       size;
  size_t       count;
  HeapElement* data;
};
static int instance_count = 0;

#define heap_front(h) (*(h)->data)
#define heap_term(h) (free((h)->data))
#define CMP(a, b) ((a->modtime) <= (b->modtime))

HeapElement
heap_newElement(rmU64 fileModTime, char* filepath)
{
  HeapElement elem;
  ++instance_count;
  elem = malloc(sizeof(struct filetimestamp));
  elem->id = instance_count;
  elem->modtime = fileModTime;
  elem->name = malloc((1+strlen(filepath))*sizeof(char));
  strcpy(elem->name, filepath);
  return elem;
}

void
heap_freeElement(HeapElement elem)
{
  if (!elem) return;
  if (elem->name) free(elem->name);
  free(elem);
}

void
heap_init(struct heap *h, int size)
{
  h->size  = size;
  h->count = 0;
  h->data  = malloc(sizeof(HeapElement) * size);
  if (!h->data) _exit(1);
}

HeapElement
heap_pop(struct heap *h)
{
  unsigned int index, swap, other;
  HeapElement poppedElem = NULL, temp = NULL;
  if (h->count==0) {
    return NULL;
  }
  poppedElem = h->data[0];

  temp = h->data[--h->count];
  for(index = 0; 1; index = swap) {
    swap = (index << 1) + 1;
    if (swap >= h->count) break;
    other = swap + 1;
    if ((other < h->count) && CMP(h->data[other], h->data[swap])) swap = other;
    if CMP(temp, h->data[swap]) break;
    h->data[index] = h->data[swap];
  }
  h->data[index] = temp;
  return poppedElem;
}

int
heap_push(struct heap *h, HeapElement value)
{
  unsigned int index, parent;
  if (h->count>0) {
    if (h->count < h->size) {
      /* go ahead */
    } else {
      HeapElement min_in_top = heap_front(h);
      if (CMP(min_in_top, value)) {
        heap_freeElement(heap_pop(h));
      } else {
        return 0;
      }
    }
  }
  for(index = h->count++; index; index = parent)
  {
    parent = (index - 1) >> 1;
    if CMP(h->data[parent], value) break;
    h->data[index] = h->data[parent];
  }
  h->data[index] = value;
  return 1;
}

/**  End heap implementation  ************************************************/

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
offertoheap(struct heap *h, char *filepath)
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

void processLines(struct heap *h, char eol)
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
  struct heap hh;
  int bPrintTime = 0;
  char timeStr[] = "year-MM-dd hh:mm:ss "; /* just to take care of size */
  char eol = '\n';
  HeapElement popped;
  if (!checkInputs(argc, argv, &N, &bPrintTime, &eol)) {
    return -1;
  }
  memset(timeStr, '\0', sizeof(timeStr));
  heap_init(&hh, N);
  processLines(&hh, eol);
  for (i=0; i<N*10; i++) {
    popped = heap_pop(&hh);
    if (!popped)
      break;
    if (bPrintTime) {
      fillTimeStr(timeStr, popped->modtime);
    }
    printf("%s%s%c", timeStr, popped->name, eol);
    heap_freeElement(popped);
  }
  heap_term(&hh);
  return 0;
}
