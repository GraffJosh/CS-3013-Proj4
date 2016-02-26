
#include <sys/time.h>
#include <sys/types.h>
#include "mmu.c"

#define MAXPAGES 999
#define MAXTHREADS 3
#define NUMTHREADPAGES MAXPAGES/MAXTHREADS 

int memMaxs[MAXTHREADS];


void* dskchk(void* arg) {



vAddr memory[NUMTHREADPAGES];
int i,j,initValues[NUMTHREADPAGES],returnValues[NUMTHREADPAGES];
struct timeval start_time, end_time;

	gettimeofday(&start_time, NULL);



    for (i = 0; i < NUMTHREADPAGES; ++i) {
        memory[i] = create_page();
    }
    
    printf("%d %d\n", memory[10], memory[100]);

    for (i = 0; i < NUMTHREADPAGES; ++i) {
        j = i*3;
        store_value(memory[i], &j);
        initValues[i]=j;
    }

    for (i = 0; i < NUMTHREADPAGES; ++i) {
    	j= *get_value(memory[i]);

    	if(j != initValues[i])
    		printf("ErrorInMemory! expected: %d, actual: %d\n", j, initValues[i]);
    }

    for (i = 0; i < NUMTHREADPAGES; ++i) {
    	free_page(memory[i]);
    }


	gettimeofday(&end_time, NULL);


    double msec = (end_time.tv_sec - start_time.tv_sec) * 1000 +
				  (end_time.tv_usec - start_time.tv_usec) / 1000.;

	printf("Thread: %d time = %f\n", *((int*) arg), msec);
}


int main(int argc, char const *argv[]) {

    pthread_t c_threads[MAXTHREADS];
	int i;


    for (i = 0; i < MAXTHREADS; ++i)
    {
    	pthread_create(&c_threads[i], NULL, &dskchk, &i);
    }

    for (i = 0; i < MAXTHREADS; ++i)
    {
    	pthread_join(c_threads[i], NULL);
    }




    return 0;
}
