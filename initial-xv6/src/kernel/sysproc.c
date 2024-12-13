#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

char * sysMap[]={
  "fork",
  "exit",
  "wait",
  "pipe",
  "read",
  "kill",
  "exec",
  "fstat",
  "chdir",
  "dup",
  "getpid",
  "sbrk",
  "sleep",
  "uptime",
  "open",
  "write",
  "mknod",
  "unlink",
  "link",
  "mkdir",
  "close",
  "waitx",
  "hello",
  "count"
};


uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0; // not reached
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
  if (growproc(n) < 0)
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
  while (ticks - ticks0 < n)
  {
    if (killed(myproc()))
    {
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
sys_waitx(void)
{
  uint64 addr, addr1, addr2;
  uint wtime, rtime;
  argaddr(0, &addr);
  argaddr(1, &addr1); // user virtual memory
  argaddr(2, &addr2);
  int ret = waitx(addr, &wtime, &rtime);
  struct proc *p = myproc();
  if (copyout(p->pagetable, addr1, (char *)&wtime, sizeof(int)) < 0)
    return -1;
  if (copyout(p->pagetable, addr2, (char *)&rtime, sizeof(int)) < 0)
    return -1;
  return ret;
}

// uint64
// sys_halt(void)
// {
//   outb(0xf4, 0x00);
//   return 0;
// }

uint64
getSysCount(void){
  int mask;

  argint(0,&mask);

 

  struct proc * curr_proc=myproc();

  int syscall_idx=0;

  for(int i=0;i<MAX_SYS_CALLS;i++){
    if(mask==(1<<i)){
      syscall_idx=i;
      break;
    }
  }

  int total_syscall_cnt=curr_proc->syscall_count[syscall_idx];

  



  printf("PID %d called %s %d times. \n",curr_proc->pid,sysMap[ syscall_idx-1],total_syscall_cnt);

  return total_syscall_cnt;
}

uint64
sys_hello(void) {
    printf("Hello world\n");
    return 12;
 }

 uint64
 sigalarm(void){

  int interval;
  uint64 functionHandler;

  argint(0,&interval);
  argaddr(1,&functionHandler);

  if(interval<0){
    return -1;
  }

  struct proc * curr_process=myproc();

  curr_process->interval=interval;
  curr_process->handler_func=functionHandler;
  return 0;
  }

uint64
sigreturn(void){

  struct proc * curr_process=myproc();
  memmove(curr_process->trapframe,curr_process->initital_trapframe,sizeof(struct trapframe));

  curr_process->interval=curr_process->CPUticks;
  curr_process->CPUticks=0;

  return curr_process->trapframe->a0;
 }  

 uint64
settickets(void){
  int ticketValue;
  argint(0,&ticketValue);
  if(ticketValue<0){
    return -1;
  }
  struct proc * curr_proc=myproc();
  curr_proc->tickets=ticketValue;
  
  return 0;
}  