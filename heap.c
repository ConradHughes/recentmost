/* `heap.c'
 *
 * Heap implementation for recentmost.
 *
 * Origin: https://gist.github.com/martinkunev/1365481
 */

#include <string.h>
#include <stdlib.h>
#ifndef _WIN32
#include <unistd.h>
#else
#include <windows.h>
#endif

#include "heap.h"

struct _heap {
  size_t       size;
  size_t       count;
  HeapElement* data;
};

static int instance_count = 0;

#define CMP(a, b) ((a->modtime) <= (b->modtime))

Heap
heap_alloc(int size)
{
  Heap h = malloc(sizeof(*h));

  h->size  = size;
  h->count = 0;
  h->data  = malloc(sizeof(HeapElement) * size);
  if(!h->data) _exit(1);

  return h;
}

void
heap_free(Heap h)
{
  free(h->data);
  free(h);
}

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

HeapElement
heap_pop(Heap h)
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
heap_push(Heap h, HeapElement value)
{
  unsigned int index, parent;
  if (h->count>0) {
    if (h->count < h->size) {
      /* go ahead */
    } else {
      HeapElement min_in_top = *(h->data);
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
