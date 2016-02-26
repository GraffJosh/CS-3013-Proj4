
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include "mmu.h"
#include <sys/time.h>

#define NUMPAGES 40
vAddr memory[NUMPAGES];

/*int main(int argc, char const *argv[]) {

	unsigned int seed;
    if (argc > 1) {
        seed = (unsigned int) strtol(argv[1], NULL, 0);
    } else {
        seed = (unsigned int) time(NULL);
    }
    printf("Rand Seed: %u\n", seed);
    srand(seed);
    
    int j;
    for (int i = 0; i < NUMPAGES; ++i) {
    	printf("\n");
        memory[i] = create_page();
        j = i+2;
        store_value(memory[i], &j);
        // int* ret = get_value(memory[i]);
        // printf("%d %d\n", j, *ret);
    }
    
    // 
    int error = 0;
    for (int i = 0; i < NUMPAGES; ++i) {
        j = i+2;
        int* ret = get_value(memory[i]);
        if(j-(*ret) != 0)
        	error = i;

        printf("%d => %d %d\n\n", memory[i], j, *ret);
    }
    if(error)
    	printf("Error occured, %d\n", error);
}*/

    #define MAXPAGES 750
#define MAXTHREADS 1
#define NUMTHREADPAGES MAXPAGES/MAXTHREADS 

int memMaxs[MAXTHREADS];
    int main(int argc, char const *argv[])
    {
        

int i,j,initValues[NUMTHREADPAGES],returnValues[NUMTHREADPAGES];
struct timeval start_time, end_time;

    gettimeofday(&start_time, NULL);


//create
    for (i = 0; i < NUMTHREADPAGES; ++i) {
        memory[i] = create_page();
    }
    
    printf("%d %d\n", memory[10], memory[100]);


//store
    for (i = 0; i < NUMTHREADPAGES; ++i) {
        j = i*3;
        store_value(memory[i], &j);
        initValues[i]=j;
    }


//get
    for (i = 0; i < NUMTHREADPAGES; ++i) {
        j= *get_value(memory[i]);

        if(j != initValues[i])
            printf("ErrorInMemory! expected: %d, actual: %d\n",  initValues[i],j);
    }

//free
    for (i = 0; i < NUMTHREADPAGES; ++i) {
        free_page(memory[i]);
    }


    gettimeofday(&end_time, NULL);


    double msec = (end_time.tv_sec - start_time.tv_sec) * 1000 +
                  (end_time.tv_usec - start_time.tv_usec) / 1000.;

    printf("Thread: %d time = %f\n", 1, msec);
}