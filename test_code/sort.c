typedef unsigned long ulong_t;

#ifdef TESTING
# include <stdio.h>
# include <assert.h>
#else TESTING
# define ARRAYBASE 0xa0000000
# define ARRAYSIZE 128
#endif TESTING

void insertion_sort(ulong_t *array, ulong_t size);
void insert(ulong_t *array, ulong_t size, ulong_t x);
#ifdef TESTING
void parray(ulong_t *array, ulong_t size);
int main(int argc, char **argv);
#endif TESTING

void insertion_sort(ulong_t *array, ulong_t size)
{
 ulong_t x;

 for (x = 1; x < size; x++) {
  insert(array, size, x);
 }
}

void insert(ulong_t *array, ulong_t size, ulong_t x)
{
 ulong_t temp;
 long y, behind;

 temp = array[x];
 /* find insertion point */
 for (behind = x - 1; behind >= 0; behind--) {
  if (temp >= array[behind]) break;
 }
 /* move elements over */
 for (y = x - 1; y > behind; y--) {
  array[y+1] = array[y];
 }
 /* insert new element */
 array[behind + 1] = temp;
}

#ifdef TESTING

void parray(ulong_t *array, ulong_t size)
{
 ulong_t x;
 for (x = 0; x < size; x++) {
  printf("%8ld ",array[x]);
  if (x % 8 == 7 || x == size - 1) {
   printf("\n");
  }
 }
}

int main(int argc, char **argv)
{
 ulong_t ASIZ;
 ulong_t *x, y;
 assert(argc>1);
 ASIZ = strtoul(argv[1],NULL,0);
 x = (ulong_t *)malloc(ASIZ * sizeof(ulong_t));
 assert(x!=NULL);
 srand(time(NULL)|getpid());
 for (y = 0; y < ASIZ; y++) x[y] = rand() % 100;
 printf("before sorting:\n");
 parray(x,ASIZ);
 inssort(x,ASIZ);
 printf("after sorting:\n");
 parray(x,ASIZ);
 return 0;
}

#else TESTING

# ifdef USING_RANDOM_REGISTER
volatile ulong_t random(void)
{
 register ulong_t randomval;
 __asm__("\tmfc0 %0, $1\n" : "=r" (randomval)); /* CP0 Random register */
 return randomval;
}
# else USING_RANDOM_REGISTER
extern long random();
# endif USING_RANDOM_REGISTER

void entry(void)
{
 ulong_t *x, y;

 x = (ulong_t *)ARRAYBASE;
 for (y = 0; y < ARRAYSIZE; y++) {
  x[y] = random();
 }
 insertion_sort(x,ARRAYSIZE);
}

#endif TESTING
