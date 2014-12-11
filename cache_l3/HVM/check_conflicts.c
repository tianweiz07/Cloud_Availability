#define _POSIX_C_SOURCE 199309
#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <time.h>

#define ASSOC 20
#define SLICES 6
#define WAY_SIZE 131072
#define LINE_SIZE 64

#define TRIALS 10000

char *buf;
char *head;
char *tail;
char value;
unsigned int time_buf;

int set;
int assoc;
int slice;

unsigned int conflict_sets[SLICES][21] ={
/*{6,7,10,11,19,30,31,36,37,54,55,58,76,77,88,89,98,99,110,111,114},
{14,15,26,27,32,33,44,45,50,51,68,69,72,73,80,92,93,102,103,118,119},
{0,13,20,25,35,41,46,49,59,60,65,70,75,82,85,95,101,104,116,121,126},
{3,4,9,16,23,29,39,42,53,56,63,66,79,81,86,91,97,107,108,112,122},
{2,5,8,17,22,28,38,43,52,57,62,67,78,87,90,96,106,109,113,123,124},
{1,12,18,21,24,34,40,47,48,61,64,71,74,83,84,94,100,105,117,120,130}};
*/
{5,	8,	15,	17,	27,	28,	38,	43,	51,	52,	57,	67,	73,	78,	87,	90,	96,	103,	109,	113},	
{6,	7,	11,	18,	19,	30,	31,	40,	41,	54,	55,	58,	59,	64,	65,	84,	85,	98,	110,	111,	114},	
{4,	9,	14,	16,	26,	29,	32,	39,	42,	50,	53,	56,	66,	79,	86,	91,	97,	102,	108,	112,	119},	
//{10,	13,	20,	25,	35,	36,	46,	49,	60,	70,	75,	76,	82,	88,	95,	101,	104,	116,	121},	
{128,	123,	121,	116,	33,	104,	23,	76,	95,	69,	80,	25,	13,	3,	93,	60,	20,	122,	45,	72,	22},
{2,	3,	22,	23,	33,	44,	45,	62,	63,	68,	69,	72,	80,	81,	92,	93,	106,	107,	122,	123,	128},	
{1,	12,	21,	24,	34,	37,	47,	48,	61,	71,	74,	77,	83,	89,	94,	99,	100,	105,	117,	120,	130}};
void initialize() {
	char **ptr1, **ptr2;
	int j,k;
	char *tmp;
	int idx1, idx2;

	for (j=0; j<assoc; j++){
		idx1 = conflict_sets[slice][j];
		ptr1 = (char **)&buf[idx1*WAY_SIZE+set*LINE_SIZE];
		*ptr1 = (char *)ptr1;
	}

	for (j=assoc-1; j>=1; j--){
		k = rand()%j;
		idx1 = conflict_sets[slice][j];
		idx2 = conflict_sets[slice][k];
		ptr1 = (char**)&buf[idx1*WAY_SIZE+set*LINE_SIZE];
		ptr2 = (char**)&buf[idx2*WAY_SIZE+set*LINE_SIZE];
		tmp = *ptr1;
		*ptr1 = *ptr2;
		*ptr2 = tmp;
	}

	for (j=0; j<assoc; j++){
		idx1 = conflict_sets[slice][j];
		ptr1 = (char **)&buf[idx1*WAY_SIZE+set*LINE_SIZE];
		ptr2 = (char **)*ptr1;
		*(ptr2+1) = (char*)(ptr1+1);
	}

	idx1 = conflict_sets[slice][0];
	head = &buf[idx1*WAY_SIZE+set*LINE_SIZE];

	ptr1 = (char **)head;

	for (j=0; j<assoc-1; j++){
		ptr1 = (char**)(*ptr1);
	}
   
	tail = (char*)ptr1;
} 



void prime()
{
   __asm__("mov %0,%%r8\n\t"
           "mov %%r8,%%rsi\n\t"
           "xor %%eax, %%eax\n\t"
           "cpuid\n\t"
           "loop: mov (%%r8), %%r8\n\t"
           "cmp %%r8,%%rsi\n\t"
           "jne loop\n\t"
           "xor %%eax, %%eax\n\t"
           "cpuid\n\t"
           :
           :"r"(head)
           :"esi","r8","eax");
}

void probe()
{
   __asm__("mov %0,%%r8\n\t"
           "mov %%r8,%%rsi\n\t"
           "mov %1,%%r10\n\t"
           "xor %%eax, %%eax\n\t"
           "cpuid\n\t"
         //  "lfence\n\t"
           "rdtsc\n\t"
           "mov %%eax,%%edi\n\t"
           "loop1: mov (%%r8), %%r8\n\t"
           "cmp %%r8,%%rsi\n\t"
           "jne loop1\n\t"
           "xor %%eax, %%eax\n\t"
           "cpuid\n\t"
         //  "lfence\n\t"
           "rdtsc\n\t"
           "sub %%edi, %%eax\n\t"
           "mov %%eax, (%%r10)\n\t"
           :
           :"r"(head),"r"(&time_buf)
           :"eax","edx","esi","edi","r8","r10"
           );
}

int main (int argc, char *argv[]) {
        cpu_set_t mask;
        CPU_ZERO(&mask);
        CPU_SET(0, &mask);
        sched_setaffinity(0, sizeof(cpu_set_t), &mask);

        set = atoi(argv[1]);
	assoc = atoi(argv[2]);
	slice = atoi(argv[3]);

        uint64_t buf_size = 1024*1024*1024;
        int fd = open("/mnt/hugepages/nebula1", O_CREAT|O_RDWR, 0755);
        if (fd<0) {
                printf("file open error!\n");
                return 0;
        }

        buf = mmap(0, buf_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
        if (buf == MAP_FAILED) {
                printf("map error!\n");
                unlink("/mnt/hugepages/nebula1");
                return 0;
        }

	initialize();
	int i;
	unsigned int total_time = 0;
	while (i < TRIALS){
		prime();
		probe();
		total_time += time_buf;
		i++;
	} 
	printf("access time: %u\n", total_time/TRIALS);
	munmap(buf, buf_size);
}
