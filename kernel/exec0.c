/*
 * A minimal version of exec.c, specific to loading the nes0 binary ONLY.
 * - No filesystem dependency, parses ELF binary at a kernel VA.
 */

#define K2_DEBUG_WARN 
// #define K2_DEBUG_VERBOSE

#include "plat.h"
#include "utils.h"
#include "sched.h"
#include "mmu.h"
#include "elf.h"

/*
 * fxl: derived from xv6, for which user stack NOT at the very end of va
 * but next page boundary of user code (with 1 pg of stack guard
 * in between) this limits to user stack to 4KB. (not growing)
 * (not sure why xv6 was designed like this)
 */
static int loadseg0(struct mm_struct *mm, uint64 va, const char *elfbase, 
  uint offset, uint sz);

static int flags2perm(int flags) {
    // elf, https://refspecs.linuxbase.org/elf/gabi4+/ch5.pheader.html#p_flags
#define   PF_X    1
#define   PF_W    2
#define   PF_R    4

    int perm = 0;    
    BUG_ON(!(flags & PF_R));  // not readable? exec only??

    if(flags & PF_W)
      perm = MM_AP_RW;
    else 
      perm = MM_AP_EL0_RO; 

    if(!(flags & PF_X)) 
      perm |= MM_XN;

    return perm;
}


#include "fb.h"  // see below 

/*
  exec0: a minimal version of exec() to load a nes0 binary
  Called from sys_exec() 

  Main idea: we'll prepare a fresh pgtable tree "on the side". In doing so,
  we allocate/map new pages along the way. If everything
  works out, "swap" the fresh tree with the existing pgtable tree. As a result,
  all existing user pages are freed and their mappings won't be 
  in the new pgtable tree.
  
  To implement this, we duplicate the task's mm; clean all 
  info for user pages; keep the info for kernel pages.
  In the end, we free all user pages (no need to unmap them).

  The function: 
    - it loads the elf binary into the user space; 
    - it passes arguments to the user task;
    - it maps framebuffer area to the user space; 
    - it sets the pagetable tree for the user task.

  elfbase: kernel VA for the elf binary
*/
// Q5: mario
int exec0(const char *elfbase, char **argv_unused /*ignored*/) {
  // char *s, *last;
  int i, off;
  uint64 argc, sz = 0, sz1, sp, ustack[MAXARG]/*a scratch buf*/, argbase; 
  struct mm_struct *tmpmm = 0;  // new mm to prep aside, and commit to user VM
  struct elfhdr elf;
  // struct inode *ip;
  struct proghdr ph; // program header (a segment)
  void *kva; 
  struct task_struct *p = myproc();

  I("pid %d exec0 called. elfbase %lx", myproc()->pid, (unsigned long)elfbase);

  memmove((void*)&elf, elfbase, sizeof(elf)); 

  if(elf.magic != ELF_MAGIC) // ELF magic good?
    goto bad;

  I("elf magic good %x", elf.magic); 

  if(p->mm->pgd == 0) // task has a valid pgtable tree? 
    goto bad;   // XXXX race condition; need lock

  // for simplicity, we alloc a a single page for mm_struct
  _Static_assert(sizeof(struct mm_struct) <= PAGE_SIZE);  
  tmpmm = kalloc(); BUG_ON(!tmpmm); 
  // we are not using tmpmm::lock, just to appease copyout(tmpmm) etc. which grabs mm->lock
  initlock(&tmpmm->lock, "tmpmmlock"); 
  
  /* exec() only remaps user pages, so copy over kernel pages bookkeeping info.
   the caveat is that some of the kernel pages (eg for the old pgtables) will
   become unused and will not be freed until exit() */
  tmpmm->kernel_pages_count = p->mm->kernel_pages_count; 
  memmove(&tmpmm->kernel_pages, &p->mm->kernel_pages, sizeof(tmpmm->kernel_pages)); 
  tmpmm->pgd = 0; // start from a fresh pgtable tree...

  /* Load program into memory of tmpmm: iterate over all elf segments (NOT sections) */
  // NB: code below assumes segment vaddrs in ascending order
  for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
    // load a program header (a segment)
    memmove((void *)&ph, elfbase+off, sizeof(ph));

    if(ph.type != ELF_PROG_LOAD)
      continue;
    V("pid %d vaddr %lx sz %lx", myproc()->pid, ph.vaddr, sz);
    BUG_ON(ph.vaddr < sz); // sz: last seg's va end (exclusive)
    if(ph.memsz < ph.filesz)  // memsz: seg size in mem; filesz: seg bytes to load from file
      goto bad;
    // a segment's start must be page aligned, but end does not have to 
    if(ph.vaddr + ph.memsz < ph.vaddr)
      goto bad;
    if(ph.vaddr % PAGE_SIZE != 0) 
      goto bad;
    for (sz1 = ph.vaddr; sz1 < ph.vaddr + ph.memsz; sz1 += PAGE_SIZE) {
      kva = allocate_user_page_mm(tmpmm, sz1, flags2perm(ph.flags)); 
      BUG_ON(!kva); 
    }
    sz = sz1; 
    if(loadseg0(tmpmm, ph.vaddr, elfbase, ph.off, ph.filesz) < 0)
      goto bad;
  }
  
  sz = PGROUNDUP(sz);
  
  V("pid %d done loading prog. end sz (VA) %lx", myproc()->pid, sz); 
  
  /* Alloc a fresh user stack  */
  assert(sz + PAGE_SIZE + USER_MAX_STACK <= USER_VA_END); 
  // alloc the 1st stack page (instead of demand paging), for pushing args (below)
  if (!(kva=allocate_user_page_mm(tmpmm, USER_VA_END - PAGE_SIZE, MMU_PTE_FLAGS | MM_AP_RW))) {
    BUG(); goto bad; 
  }
  memzero_aligned(kva, PAGE_SIZE); 
  // map a guard page (inaccessible) near USER_MAX_STACK
  if (!(kva=allocate_user_page_mm(tmpmm, USER_VA_END - USER_MAX_STACK - PAGE_SIZE, MMU_PTE_FLAGS | MM_AP_EL1_RW))) {
    BUG(); 
    goto bad; 
  }

  /* Prep & passes arguments to the user task */
#define NARGS   7
#define ARGLEN  32
  char nes0argv[NARGS][ARGLEN]; // hardcoded  
  char *argv[NARGS+1] = {
    nes0argv[0],
    nes0argv[1],
    nes0argv[2],
    nes0argv[3],
    nes0argv[4],
    nes0argv[5],
    nes0argv[6],
    0 // end 
  }; 
  {
    unsigned long fb_pa, fb_pa_end; 
    BUG_ON(the_fb.fb == 0); 
    fb_pa = VA2PA(the_fb.fb);  
    fb_pa_end = fb_pa + the_fb.vheight * the_fb.pitch;

    // kinda reserve lookup PA for fb; 
    // fb_pa should be around 0x3c000000. we'll just use this PA as user VA 
    
    int offsetx = 0, offsety = 0; // can be changed for diff nes0 instances
    // prepare args for nes0. cf LiteNES0/main.c for args def 
    snprintf(nes0argv[0], ARGLEN, "%s", "nes0"); // convention
    snprintf(nes0argv[1], ARGLEN, "0x%lx", fb_pa); // user VA for fb
    snprintf(nes0argv[2], ARGLEN, "%d", the_fb.vwidth);
    snprintf(nes0argv[3], ARGLEN, "%d", the_fb.vheight);
    snprintf(nes0argv[4], ARGLEN, "%d", the_fb.pitch);
    snprintf(nes0argv[5], ARGLEN, "%d", offsetx);
    snprintf(nes0argv[6], ARGLEN, "%d", offsety);

    // mmap fb area to user VM    
    for (; fb_pa < fb_pa_end; fb_pa += PAGE_SIZE) {
      unsigned long * ret = map_page(tmpmm, 
          fb_pa, fb_pa, /* TODO: replace this */
          1 /* alloc pgtable on demand*/, 
          MMU_PTE_FLAGS | MM_AP_RW /* perm */); 
      BUG_ON(!ret);     
    }
  }

  // project idea: alternatively, make subsequent fork() inhert fb mapping, so that 
  // they can write to fb & draw donuts 

  /* Prep prog arguments on user stack  */
  sp = USER_VA_END; 
  argbase = USER_VA_END - PAGE_SIZE; // args at most 1 PAGE
  // Push argument strings, prepare rest of stack in ustack.
  // Populate arg strings on stack. save arg pointers to ustack (a scratch buf)
  //    then populate "ustack" on the stack
  for(argc = 0; argv[argc]; argc++) {
    if(argc >= MAXARG)
      goto bad;
    sp -= strlen(argv[argc]) + 1;
    sp -= sp % 16; // riscv sp must be 16-byte aligned  aarch64: may not need this?
    if(sp < argbase)
      goto bad;
    if(copyout(tmpmm, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
      goto bad;
    ustack[argc] = sp;
  }
  ustack[argc] = 0; // last arg

  // push the array of argv[] pointers.
  sp -= (argc+1) * sizeof(uint64);
  sp -= sp % 16;  // from riscv, arm64 may not need this
  if(sp < argbase)
    goto bad;
  if(copyout(tmpmm, sp, (char *)ustack, (argc+1)*sizeof(uint64))<0) /* TODO: replace this */
    goto bad;

  // arguments to user main(argc, argv)
  // argc is returned via the system call return value, which goes in r0.
  struct trapframe *regs = task_pt_regs(p);
  regs->regs[1] = sp; 

  safestrcpy(p->name, "nes0", 4);   // side quest: your prog name 
    
  /* Commit to the user VM. free previous user mapping & pages. if any */
  regs->pc = elf.entry;  // initial program counter = main
  // set the initial stack pointer
  regs->sp = sp; /* TODO: replace this */
  I("pid %d (%s) commit to user VM, sp 0x%lx", myproc()->pid, myproc()->name, sp);

  acquire(&p->mm->lock); 
  free_task_pages(p->mm, 1 /*useronly*/);  

  // Careful: transfer refcnt/lock from the existing mm
  tmpmm->ref = p->mm->ref; // mm::lock ensures memory barriers needed for mm::ref
  tmpmm->lock = p->mm->lock; 
  *(p->mm) = *tmpmm;  // commit (NB: compiled as memcpy())
  p->mm->sz = p->mm->codesz = sz;  
  V("pid %d p->mm %lx p->mm->sz %lu", p->pid,(unsigned long)p->mm, p->mm->sz);
  kfree(tmpmm); 
  // make the pgtable tree effective
  set_pgd(p->mm->pgd); /* TODO: replace this */
  release(&p->mm->lock);

  I("pid %d exec succeeds", myproc()->pid);
  return argc; // this ends up in x0, the first argument to main(argc, argv)

 bad:
  if (tmpmm && tmpmm->pgd) { // new pgtable tree ever allocated
    free_task_pages(tmpmm, 1 /*useronly*/);   
  }
  if (tmpmm)
    kfree(tmpmm); 
  I("exec failed");
  return -1;
}

/*
 * Load a program segment into pagetable at virtual address va.
 * mm: user mm 
 * va: user VA, must be page-aligned
 * offset: from the beginning of the elf "file"
 * sz: segment size, in bytes
 * Returns 0 on success, -1 on failure.
 *
 * The pages from va to va+sz must already be mapped.
 * Caller must hold mm->lock
 */
static int loadseg0(struct mm_struct *mm, uint64 va, const char *elfbase, 
  uint offset, uint sz) {
  uint i, n;
  uint64 pa;

  assert(va % PAGE_SIZE == 0); 
  assert(mm);

  // Verify mapping page by page, then load from file
  for(i = 0; i < sz; i += PAGE_SIZE){
    pa = walkaddr(mm, va + i); // given user VA, find PA
    if(pa == 0)
      panic("loadseg: address should exist");
    if(sz - i < PAGE_SIZE)
      n = sz - i;
    else
      n = PAGE_SIZE;
    memmove(PA2VA(pa), elfbase+offset+i, n); 
  }
  
  return 0;
}

// from sysfile.c
// only support in-kernel image: nes-min, for which elf binary starts
// at kernel VA: "nes_start". cf linker script
extern char nes_start;
int sys_exec(unsigned long upath /*must be 0. ignored for this lab*/, 
  unsigned long argv /*ignored for this lab*/) {
  
  if (upath) return -1; 
  int ret = exec0(&nes_start, 0 /*ignored for this lab*/);

  return ret;
}
