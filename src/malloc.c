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
      if (curr->size >= size) /* If we find a big enough free block to satisfy request */
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
      if (curr->size >= size) /* If we find a big enough free block to satisfy request */
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
#endif

#if defined NEXT && NEXT == 0
   struct _block *beginning = *last;
   curr = (*last)->next;
   while(*last != beginning && !(curr->size >= size)) 
   {
      if(curr == NULL)
      {
         curr = freeList;
      }

      *last = curr;
      curr = curr->next;
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
   num_grows++;
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
   num_mallocs++;

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
      if(next->size > size)
      {
         /* Creates new link of appropriate size*/
         struct _block* split = BLOCK_HEADER(next) + size;
         split->size = next->size - size;
         next->size = size;

         /* Sets proper next variables */
         split->next = next->next;
         next->next = split;

         /* Sets proper free variable */
         split->free = true;
      }
      next->free = false;
      num_splits++;
      num_blocks++;
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

   struct _block *check = freeList;
   int heap_size = 0;
   while(check)
   {
      if(check->free == false)
      {
         heap_size += check->size;
      }
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
   num_frees++;
   if (ptr == NULL) 
   {
      return;
   }

   /* Make _block as free */
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
   }
}


/* vim: set expandtab sts=3 sw=3 ts=6 ft=cpp: --------------------------------*/
