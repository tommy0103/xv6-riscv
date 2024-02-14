#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_pgaccess(void)
{
  int len = 0;
  uint64 va = 0, addr = 0, mask = 0;
  struct proc *p = myproc();
  argaddr(0, &va);
  argint(1, &len);
  argaddr(2, &addr);
  pte_t *pte = 0;
  // printf("%p\n", va);
  for(int i = 0; i < len; ++i) {
    // printf("%p %d\n", va, i);
    pte = walk(p->pagetable, va, 0);
    if(pte != 0 && (*pte & PTE_A)) {
    // if(*pte & PTE_A) {
      *pte ^= PTE_A;
      mask |= (1 << i);
    }
    va += PGSIZE;
  }
  // pagetable_t pgtbl = walk(p->pagetable, va, 0);
  // for(int i = 0; i < len; ++i) {
  //   if(*pgtbl & PTE_A) {
  //     *pgtbl ^= PTE_A;
  //     mask |= (1 << i);
  //   }
  //   pgtbl += PGSIZE; 
  // } // not right because va is continuous but address of pgtbl is not continuous
  if(copyout(p->pagetable, (uint64)addr, (char*)&mask, 8) < 0) {
    panic("pgaccess: copyout error\n");
    return 0;
  }
  return mask;
}
