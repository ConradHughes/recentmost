/* `heap.h'
 *
 * Heap interface for recentmost.
 */

#ifndef _HEAP_H
#define _HEAP_H

#ifndef _WIN32
typedef unsigned long long rmU64;
#else
typedef unsigned __int64 rmU64;
#endif

struct filetimestamp {
  char* name;
  rmU64 modtime;
  int   id;
};
typedef struct filetimestamp* HeapElement;

struct _heap;
typedef struct _heap* Heap;

extern Heap        heap_alloc(int size);
extern void        heap_free(Heap h);

extern HeapElement heap_newElement(rmU64 fileModTime, char* filepath);
extern void        heap_freeElement(HeapElement elem);

extern int         heap_push(Heap h, HeapElement value);
extern HeapElement heap_pop(Heap h);

#endif /* not _HEAP_H */
