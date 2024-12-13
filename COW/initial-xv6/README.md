# xv6
# LAZY Fork (25 marks)

* add cow flag in kernel/riscv.h

```c
#define PTE_COW (1L << 5) // copy-on-write
```

* define addReference(void*) , copyndecref(void*), pagefhandler(pagetable_t ,uint64) in kernel/defs.h

```c
void            cow_incref(void*);
void*           cow_decref(void*);
int             cow_handler(pagetable_t ,uint64);
void            cow_init();
```

* define a struct  which stores the number of processes which are referring to a page of memory in read-only manner.

```c
struct {
  struct spinlock lock;
  int ref[MAXREF];
} cow_ref;
```

* define the following macros in kernel/memlayout.h

```c
#define PA2REF(pa) ((((uint64)pa))/PGSIZE)
// gives us the index of the physical page we are referencing to
#define MAXREF PA2REF(PHYSTOP)
// PHYSTOP is the max limit of physical memory
```

* kernel/main.c

```c
  cow_init();
```

* void cow_incref(void* pa) in kernel/kalloc.c
```c
void cow_incref(void* pa){
  int ref = PA2REF(pa);
  if(ref < 0 || ref >= MAXREF)
    return;
  acquire(&cow_ref.lock);
  cow_ref.ref[ref]++;
  release(&cow_ref.lock);
}
```

* add cow_decref(void* pa) in kernel/kalloc.c
```c
// decrements the reference count and copies memory
void *cow_decref(void *pa)
{
  int index = getIndex(pa);
  if(index == -1){
    return 0;
  }

  acquire(&cow_ref.lock);
  if (cow_ref.ref[index] <= 1)
  {
    release(&cow_ref.lock);
    return pa;
  }

  cow_ref.ref[index] = cow_ref.ref[index] - 1;

  uint64 copymem = (uint64)kalloc();
  if (copymem == 0)
  {
    release(&cow_ref.lock);
    return 0;
  }
  memmove((void *)copymem, (void *)pa, PGSIZE);
  release(&cow_ref.lock);
  return (void *)copymem;
}
```

* add cow_handler function in kernel/kalloc.c
```c
int cow_handler(pagetable_t pagetable, uint64 va)
{
  if (va >= MAXVA || va <= 0)
    return -1;
  // finds the page table entry corresponding to the va
  pte_t *pte = walk(pagetable, va, 0);
  if (pte == 0)
  {
    return -1;
  }

  // is the page valid
  if(!(*pte & PTE_V)){
    return -1;
  }
  // is the page in user space
  if(!(*pte & PTE_U)){
    return -1;
  }
  // is the page a cow page
  if(!(*pte & PTE_COW)){
    return -1;
  }
 
  uint64 pa = PTE2PA(*pte);
  // void *mem = cow_decref((void *)pa);
  char*mem = kalloc();
  if (mem == 0)
    return -1;

  memmove(mem, (char *)pa, PGSIZE);
  

 
    
  uint64 flags = PTE_FLAGS(*pte);
  flags |= PTE_W; // addding write flag
  flags &= (~PTE_COW); // removing cow flag

  uvmunmap(pagetable, PGROUNDDOWN(va), 1, 1);
  if (mappages(pagetable, va,1 , (uint64)mem, flags) == -1)
  {
    panic("Pagefhandler mappages");
  }

  return 0;
}
```

* usertrap() in kernel/trap.c

```c
else if(r_scause() == 15){

    // Handling the page fault
    if(killed(p))
      exit(-1);
    uint64 va = PGROUNDDOWN(r_stval());
     // Going to the start of the page which cause Page Fault Exception
    if(cow_handler(p->pagetable,va) < 0){
      p->killed = 1;
    }
  }
```

* uvmcopy() in kernel/vm.c
```c
// copies the page table of the parent process to its child's page table , instead of allocating new pages to child
int
uvmcopy(pagetable_t old, pagetable_t new, uint64 sz)
{
  pte_t *pte;
  uint64 pa, i;
  uint flags;
  // char *mem;

  for(i = 0; i < sz; i += PGSIZE){
    if((pte = walk(old, i, 0)) == 0)
      panic("uvmcopy: pte should exist");
    if((*pte & PTE_V) == 0)
      panic("uvmcopy: page not present");
    pa = PTE2PA(*pte);

    *pte = (*pte & ~PTE_W) | PTE_COW; // add cow flag and remove write flag

    flags = PTE_FLAGS(*pte);
    // if((mem = kalloc()) == 0)
    //   goto err;
    // memmove(mem, (char*)pa, PGSIZE);
    if(mappages(new, i, PGSIZE,pa, flags) != 0){
      // kfree(mem);
      goto err;
    }
    cow_incref((void*)pa);
  }
  return 0;

 err:
  uvmunmap(new, 0, i / PGSIZE, 1);
  return -1;
}
```

* copyout() in kernel/vm.c

```c
int
copyout(pagetable_t pagetable, uint64 dstva, char *src, uint64 len)
{
  uint64 n, va0, pa0;
    cow_handler(pagetable,dstva); // Makes page fault exception if it is a cow page
  while(len > 0){
    va0 = PGROUNDDOWN(dstva);
    pa0 = walkaddr(pagetable, va0);
    if(pa0 == 0)
      return -1;
    n = PGSIZE - (dstva - va0);
    if(n > len)
      n = len;
    memmove((void *)(pa0 + (dstva - va0)), src, n);

    len -= n;
    src += n;
    dstva = va0 + PGSIZE;
  }
  return 0;
}
```

* kfree() in kernel/kalloc.c
```c
void kfree(void *pa)
{
  struct run *r;

  int index = PA2REF(pa);

  if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  acquire(&cow_ref.lock);
  cow_ref.ref[index]--;
  if (cow_ref.ref[index] > 0)
  {
    release(&cow_ref.lock);
    return;
  }
  release(&cow_ref.lock);

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run *)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

```

* kalloc() in kernel/kalloc.c

```c
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if (r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if (r)
  {

    memset((char *)r, 5, PGSIZE); // fill with junk
    cow_ref.ref[PA2REF((uint64)r)] = 1;
  }
  return (void *)r;
}
```