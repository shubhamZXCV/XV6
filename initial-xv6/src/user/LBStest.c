#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define NFORK 10
#define IO 5

// Define constants for the LCG
#define MODULUS 2147483648 // 2^31 (a common modulus)
#define MULTIPLIER 1103515245
#define INCREMENT 12345
unsigned long seed = 1; // Initial seed value (can be any positive value)

unsigned long lcg()
{
  seed = (MULTIPLIER * seed + INCREMENT) % MODULUS;
  return seed;
}

int random(int l, int r)
{
  return l + lcg() % (r - l);
}

int main()
{
  int n, pid;
  int wtime, rtime;
  int twtime = 0, trtime = 0;
  for (n = 0; n < NFORK; n++)
  {
    pid = fork();
   

    if (pid < 0)
      break;
    if (pid == 0)
    {
      settickets(n+1);
      if (n < IO)
      {
        sleep(200); // IO bound processes
      }
      else
      {
        for (volatile int i = 0; i < 1000000000; i++)
        {
        } // CPU bound process
      }
      // printf("Process %d finished\n", n);
      exit(0);
    }
  }
  for (; n > 0; n--)
  {
    if (waitx(0, &wtime, &rtime) >= 0)
    {
      trtime += rtime;
      twtime += wtime;
    }
  }
  printf("Average rtime %d,  wtime %d\n", trtime / NFORK, twtime / NFORK);
  exit(0);
}