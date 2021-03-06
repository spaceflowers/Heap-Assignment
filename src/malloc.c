/*
   Thomas Tran
   1000901761
   Kyra Belgica
   1001290832
*/

#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define ALIGN4(s)         (((((s) - 1) >> 2) << 2) + 4)
#define BLOCK_DATA(b)      ((b) + 1)
#define BLOCK_HEADER(ptr)   ((struct _block *)(ptr) - 1)


static int atexit_registered = 0;
static int num_mallocs       = 0; //
static int num_frees         = 0; //
static int num_reuses        = 0; //
static int num_grows         = 0; //
static int num_splits        = 0; //
static int num_coalesces     = 0; //
static int num_blocks        = 0; //
static int num_requested     = 0; //
static int max_heap          = 0; //

/*
 *  \brief printStatistics
 *
 *  \param none
 *
 *  Prints the heap statistics upon process exit.  Registered
 *  via atexit()
 *
 *  \return none
 */
void printStatistics( void )
{
  printf("\nheap management statistics\n");
  printf("mallocs:\t%d\n", num_mallocs );
  printf("frees:\t\t%d\n", num_frees );
  printf("reuses:\t\t%d\n", num_reuses );
  printf("grows:\t\t%d\n", num_grows );
  printf("splits:\t\t%d\n", num_splits );
  printf("coalesces:\t%d\n", num_coalesces );
  printf("blocks:\t\t%d\n", num_blocks );
  printf("requested:\t%d\n", num_requested );
  printf("max heap:\t%d\n", max_heap );
}

struct _block
{
   size_t  size;         /* Size of the allocated _block of memory in bytes */
   struct _block *next;  /* Pointer to the next _block of allcated memory   */
   bool   free;          /* Is this _block free?                     */
   char   padding[3];
};


struct _block *freeList = NULL; /* Free list to track the _blocks available */

/*
 * \brief findFreeBlock
 *
 * \param last pointer to the linked list of free _blocks
 * \param size size of the _block needed in bytes
 *
 * \return a _block that fits the request or NULL if no free _block matches
 */
struct _block *findFreeBlock(struct _block **last, size_t size)
{
   struct _block *curr = freeList;

#if defined FIT && FIT == 0
   /* First fit */
   while (curr && !(curr->free && curr->size >= size))
   {
      *last = curr;
      curr  = curr->next;
   }
#endif

#if defined BEST && BEST == 0
   /* Best fit */
   struct _block *smallestblock = NULL; /* Saves smallest block that can satisfy request */
   while (curr)
   {
      if (curr->size >= size && curr->free) /* If we find a big enough free block to satisfy request */
      {
         if(smallestblock == NULL)
         {
            smallestblock = curr;
         }
         else if(curr->size < smallestblock->size) /* If current free block is smaller than saved block */
         {
            smallestblock = curr;
         }
      }

      *last = curr;
      curr = curr->next;
   }
   curr = smallestblock;
#endif

#if defined WORST && WORST == 0
   struct _block *biggestblock = NULL; /* Saves biggest block that can satisfy request */
   while(curr)
   {
      if (curr->size >= size && curr->free) /* If we find a big enough free block to satisfy request */
      {
         if(biggestblock == NULL)
         {
            biggestblock = curr;
         }
         else if(curr->size > biggestblock->size) /* If current free block is bigger than saved block */
         {
            biggestblock = curr;
         }
      }

      *last = curr;
      curr = curr->next;
   }
   curr = biggestblock;
#endif

#if defined NEXT && NEXT == 0
   // Starting from the last used block
   curr = *last;
   if(curr)
     curr = curr->next;
   // Check from the last block to the end of the list.
   while(curr && !(curr->size >= size && curr->free))
   {
     *last = curr;
     curr = curr->next;
   }
   // If the check failed, reset to beginning and search to end. (Its First Fit again!)
   if(curr == NULL)
   {
     curr = freeList;
     while (curr && !(curr->free && curr->size >= size))
     {
        *last = curr;
        curr  = curr->next;
     }
   }
#endif

   return curr;
}

/*
 * \brief growheap
 *
 * Given a requested size of memory, use sbrk() to dynamically
 * increase the data segment of the calling process.  Updates
 * the free list with the newly allocated memory.
 *
 * \param last tail of the free _block list
 * \param size size in bytes to request from the OS
 *
 * \return returns the newly allocated _block of NULL if failed
 */
struct _block *growHeap(struct _block *last, size_t size)
{
   /* Request more space from OS */
   struct _block *curr = (struct _block *)sbrk(0);
   struct _block *prev = (struct _block *)sbrk(sizeof(struct _block) + size);

   assert(curr == prev);

   /* OS allocation failed */
   if (curr == (struct _block *)-1)
   {
      return NULL;
   }

   /* Update freeList if not set */
   if (freeList == NULL)
   {
      freeList = curr;
   }

   /* Attach new _block to prev _block */
   if (last)
   {
      last->next = curr;
   }

   /* Update _block metadata */
   curr->size = size;
   curr->next = NULL;
   curr->free = false;
   num_blocks++;
   num_grows++;
   return curr;
}

/*
 * \brief malloc
 *
 * finds a free _block of heap memory for the calling process.
 * if there is no free _block that satisfies the request then grows the
 * heap and returns a new _block
 *
 * \param size size of the requested memory in bytes
 *
 * \return returns the requested memory allocation to the calling process
 * or NULL if failed
 */
void *malloc(size_t size)
{
   num_requested += size;

   if( atexit_registered == 0 )
   {
      atexit_registered = 1;
      atexit( printStatistics );
   }

   /* Align to multiple of 4 */
   size = ALIGN4(size);

   /* Handle 0 size */
   if (size == 0)
   {
      return NULL;
   }

   /* Look for free _block */
   struct _block *last = freeList;
   struct _block *next = findFreeBlock(&last, size);
   /* Split free _block if possible */
   if(next)
   {
      num_reuses++;
      /* Disabled Splitting code.
      if(next->size > size + sizeof(struct _block)) // Check for enough space to split.
      {
         size_t total = next->size;
         struct _block* temp = next->next; // Saving old values for later
         next->size = size;
         next->next = next+size;

         next->next->free = true; // TODO - debugger segfaults HERE. Next 3 lines also crash.
         next->next->next = temp;
         next->next->size = total - size - sizeof(struct _block);

         num_splits++;
         num_blocks++;
      }*/
   }

   /* Could not find free _block, so grow heap */
   if (next == NULL)
   {
      next = growHeap(last, size);
   }

   /* Could not find free _block or grow heap, so just return NULL */
   if (next == NULL)
   {
      return NULL;
   }
   /* Mark _block as in use */
   next->free = false;
   num_mallocs++;

   struct _block *check = freeList;
   int heap_size = 0;
   while(check)
   {
      if(check->free == false)
      {
         heap_size += check->size;
      }
      check = check->next;
   }
   if(heap_size > max_heap)
   {
      max_heap = heap_size;
   }
   /* Return data address associated with _block */
   return BLOCK_DATA(next);
}

/*
 * \brief free
 *
 * frees the memory _block pointed to by pointer. if the _block is adjacent
 * to another _block then coalesces (combines) them
 *
 * \param ptr the heap memory to free
 *
 * \return none
 */
void free(void *ptr)
{

   if (ptr == NULL)
   {
      return;
   }

   /* Make _block as free */
   num_frees++;
   struct _block *curr = BLOCK_HEADER(ptr);
   assert(curr->free == 0);
   curr->free = true;

   /* Coalesce free _blocks if needed */
   curr = freeList;
   while(curr != NULL)
   {
      if (curr && curr->free && curr->next && curr->next->free)
      {
         curr->size = curr->size + curr->next->size + sizeof(struct _block);
         curr->next = curr->next->next;
         num_blocks--;
         num_coalesces++;
      }
      curr = curr->next;
   }
}

void *realloc(void* ptr, size_t size)
{
  if (ptr == NULL)
    return malloc(size);
  if (size == 0)
  {
    free(ptr);
    return NULL;
  }

  // Get the old block.
  struct _block* curr = BLOCK_HEADER(ptr);
  void* buffer[curr->size];
  memcpy(buffer, BLOCK_DATA(curr) , curr->size);
  size_t num_bytes;
  if(curr->size > size)
    num_bytes = size;
  else
    num_bytes = curr->size;
  free(ptr);

  struct _block* new = (struct _block*) malloc (size);
  memcpy(BLOCK_DATA(new), buffer, num_bytes);

  return BLOCK_DATA(new);
}
void *calloc(size_t nmemb, size_t size)
{
  if( !nmemb || !size)
  {
    return NULL;
  }
  void* ptr = malloc(size * nmemb);
  memset(ptr, 0, size*nmemb);
  return ptr;
}

/* vim: set expandtab sts=3 sw=3 ts=6 ft=cpp: --------------------------------*/
