// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define BNUM 17

struct {
  struct spinlock lock;
  struct spinlock block[BNUM];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head[BNUM];
} bcache;

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");

  // Create linked list of buffers
  for (int i = 0; i < BNUM; i++) {
      initlock(bcache.block + i, "bcache");
      bcache.head[i].prev = &bcache.head[i];
      bcache.head[i].next = &bcache.head[i];
  }
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.head[0].next;
    b->prev = &bcache.head[0];
    initsleeplock(&b->lock, "buffer");
    bcache.head[0].next->prev = b;
    bcache.head[0].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  int entry = blockno % BNUM;
  acquire(bcache.block + entry);

  // Is the block already cached?
  for(b = bcache.head[entry].next; b != &bcache.head[entry]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(bcache.block + entry);
      acquiresleep(&b->lock);
      return b;
    }
  }


  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for (int i = (entry + 1) % BNUM; i != entry; i = (i + 1) % BNUM) {
      uint minticks = 0x3fffffff; struct buf *minbuf = 0;
      acquire(bcache.block + i);
      for(b = bcache.head[i].prev; b != &bcache.head[i]; b = b->prev)
          if (b->refcnt == 0 && b->ticks < minticks) {
              minticks = b->ticks; minbuf = b;
          }
      if (minbuf != 0) {
          minbuf->dev = dev;
          minbuf->blockno = blockno;
          minbuf->valid = 0;
          minbuf->refcnt = 1;
          minbuf->next->prev = minbuf->prev;
          minbuf->prev->next = minbuf->next;
          minbuf->next = bcache.head[entry].next;
          minbuf->prev = &bcache.head[entry];
          bcache.head[entry].next->prev = minbuf;
          bcache.head[entry].next = minbuf;
          release(bcache.block + i);
          release(bcache.block + entry);
          acquiresleep(&minbuf->lock);
          return minbuf;
      }
      release(bcache.block + i);
  }
  // for(b = bcache.head[entry].prev; b != &bcache.head[entry]; b = b->prev){
  //   if(b->refcnt == 0) {
  //     b->dev = dev;
  //     b->blockno = blockno;
  //     b->valid = 0;
  //     b->refcnt = 1;
  //     release(&bcache.lock);
  //     acquiresleep(&b->lock);
  //     return b;
  //   }
  // }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int entry = b->blockno % BNUM;
  acquire(bcache.block + entry);
  b->refcnt--;
  if (b->refcnt == 0) b->ticks = ticks;
    // no one is waiting for it.
    // b->next->prev = b->prev;
    // b->prev->next = b->next;
    // b->next = bcache.head[entry].next;
    // b->prev = &bcache.head[entry];
    // bcache.head[entry].next->prev = b;
    // bcache.head[entry].next = b;
  release(bcache.block + entry);
}

void
bpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt++;
  release(&bcache.lock);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt--;
  release(&bcache.lock);
}


