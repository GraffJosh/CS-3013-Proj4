
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "mmu.h"
#include <sys/time.h>

#define PAGES 1000

#define RAM_SIZE 25
#define SSD_SIZE 100
#define HDD_SIZE 1000
// #define RAM_SIZE 5
// #define SSD_SIZE 10
// #define HDD_SIZE 10

#define BIT(n) (1ull<<n)

// #define RAM_DELAY 10*1000
// #define SSD_DELAY 100*1000
// #define HDD_DELAY 2500*1000
#define RAM_DELAY 0
#define SSD_DELAY 0
#define HDD_DELAY 0

int first_unallocated = 0;

struct page page_table[PAGES];

unsigned int ram_bits;
u_int32_t ram_memory[RAM_SIZE];

unsigned long long ssd_bits[2];
u_int32_t ssd_memory[SSD_SIZE];

unsigned long long hdd_bits[16];
u_int32_t hdd_memory[HDD_SIZE];

static int clockHour = 0;


vAddr create_page() {
    if (first_unallocated >= PAGES) {
        exit(0);
        return -1;
    }
    vAddr n = first_unallocated;
    struct page *new_page = &page_table[first_unallocated];
    ++first_unallocated;
    while (page_table[first_unallocated].allocated) {
        if (first_unallocated >= PAGES) {
            exit(0);
            break;
        }
        ++first_unallocated;
    }
    new_page->location = -1;
    new_page->allocated = 1;
    new_page->r = 1;
    gettimeofday(&(new_page->modTime),NULL);
    new_page->index = n;
    to_ram(new_page);
    printf("created %d (%d %d)\n", n, new_page->location, new_page->frame);
    return n;
}

u_int32_t* get_value(vAddr address) {
    struct page *new_page = &page_table[address];
    printf("getting %d (%d %d)\n", address, new_page->location, new_page->frame);
    to_ram(new_page);
    new_page->r = 1;
    gettimeofday(&(new_page->modTime),NULL);
    usleep(RAM_DELAY);
    return &ram_memory[new_page->frame];
    //printf("%#X %#llX %#llX\n", ram_bits, ssd_bits[0], hdd_bits[0]);
}

void store_value(vAddr address, u_int32_t *value) {
    struct page *new_page = &page_table[address];
    printf("storing %d (%d %d)\n", address, new_page->location, new_page->frame);
    to_ram(new_page);
    new_page->r = 1;
    gettimeofday(&(new_page->modTime),NULL);
    usleep(RAM_DELAY);
    ram_memory[new_page->frame] = *value;
    //printf("%#X %#llX %#llX\n", ram_bits, ssd_bits[0], hdd_bits[0]);
}

void free_page(vAddr address) {

    struct page *new_page = &page_table[address];
    new_page->allocated = 0;
    new_page->r = 0;
    gettimeofday(&(new_page->modTime),NULL);
    if (address < first_unallocated) {
        first_unallocated = address;
    }
}

void to_ram(struct page *page) {
    if(page->location == 0) {
        return;
    }
    printf("moving %d to ram (%d, %d)\n",
        page->index, page->location, page->frame);

    int i, n = ram_bits;
    int old = pop_mem(page->location, page->frame);
    printf(" -- (%d)\n", old);

    for (i = 0; i < RAM_SIZE; ++i) {
        if (!(n&1)) {
            ram_bits |= BIT(i);
            break;
        }
        n >>= 1;
    }

    if (i == RAM_SIZE) {
        i = evict_ram();
        ram_bits |= BIT(i);
    }

    page->location = 0;
    page->frame = i;
    printf("--> 0, %d\n", i);
    usleep(RAM_DELAY);
    ram_memory[i] = old;
}

void to_ssd(struct page *page) {
    if(page->location == 1) {
        return;
    }
    printf(" moving %d to ssd (%d, %d)\n",
        page->index, page->location, page->frame);
    
    int old = pop_mem(page->location, page->frame);
    printf("  -- (%d)\n", old);
    int addr = -1;

    int i, j;
    unsigned long long n;
    for (i = 0; i < 2; ++i) {

        n = ssd_bits[i];
        for (j = 0; j < 64 && (i*64 + j < SSD_SIZE); ++j) {
            if (!(n&1)) {
                ssd_bits[i] |= BIT(j);
                addr = i*64+j;
                goto brk;
            }
            n >>= 1;
        }
    }

    addr = evict_ssd();
    i = addr/64;
    j = addr%64;
    ssd_bits[i] |= BIT(j);

    brk:
    page->location = 1;
    page->frame = addr;
    printf(" --> 1, %d\n", addr);
    usleep(SSD_DELAY);
    ssd_memory[addr] = old;
}

void to_hdd(struct page *page) {
    int addr = -1;
    int i, j, old;
    unsigned long long n;
    if(page->location == 2) {
        return;
    }
    printf("  moving %d to hdd\n", page->index);
    
    old = pop_mem(page->location, page->frame);
    printf("   -- (%d)\n", old);
    for (i = 0; i < 16; ++i) {
        n = hdd_bits[i];
        for (j = 0; j < 64 && (i*64 + j < HDD_SIZE); ++j) {
            if (!(n&1)) {
                hdd_bits[i] |= BIT(j);
                addr = i*64 + j;
                goto brk;
            }
            n >>= 1;
        }
    }
    exit(0);
    brk:
    page->location = 2;
    page->frame = addr;
    printf("  --> 2, %d\n", addr);
    usleep(HDD_DELAY);
    hdd_memory[addr] = old;
}

int evict_ram(void) {
    struct page *old_page = evict_algo(0);
    printf(" evicting %d from ram\n", old_page->index);
    int old = old_page->frame;
    to_ssd(old_page);
    return old;
}

int evict_ssd(void) {
    struct page *old_page = evict_algo(1);
    printf("  evicting %d from ssd\n", old_page->index);
    int old = old_page->frame;
    to_hdd(old_page);
    return old;
}

struct page* random_evict(int location) {
    int n = 0;
    switch(location) {
        case 0: n = RAM_SIZE; break;
        case 1: n = SSD_SIZE; break;
    }
    int frame = rand() % n;
    for (int i = 0; i<PAGES; i++) {
        if (page_table[i].allocated &&
            page_table[i].location == location &&
            page_table[i].frame == frame) {

            return &page_table[i];
        }
    }
    int* m = NULL;
    printf("whoops %d\n", *m);
}


struct page* time_evict(int location){
    
    double currTime = 0,earliest = 0;
    while(1)
    {
        for(int i=0;i<PAGES;i++)
        {
            if (page_table[i].location == location && 
                    page_table[i].allocated == 1) {
                    
                    currTime = page_table[i].modTime.tv_sec * 1000 + page_table[i].modTime.tv_usec / 1000.;

                if (currTime > earliest)
                {
                    earliest = currTime;
                }else{
                    printf("pretime: %f, currTime: %f\n", earliest, currTime);
                    return &page_table[i];
                }

            }

        }
    }
}


struct page* clock_evict(int location){
    while(1)
    {  
        while(clockHour<PAGES)
        {
            for (int i = 0; i<PAGES; i++) {
                if (page_table[i].allocated &&
                    page_table[i].location == location ) {

                    if(page_table[i].frame == clockHour)
                    {
                        if(page_table[i].r == 0 ){
                            return &page_table[clockHour];
                        } else {
                            page_table[clockHour].r = 0;
                        }
                        clockHour++;
                    }
                }
            }
            
        }
        clockHour = 0;
    }
}

struct page* filo_evict(int location){

    for(int i=0;i<PAGES;i++)
    {
        if (page_table[i].location == location){
                if(page_table[i].allocated == 1) {
                    return &page_table[i];
                }
        }
    }


}


int pop_mem(int location, int address) {
    int i,j;
    switch(location){
        case 0:
            usleep(RAM_DELAY);
            ram_bits &= ~BIT(address);
            return ram_memory[address];
            break;
        case 1:
            usleep(SSD_DELAY);
            i = address/64;
            j = address%64;
            ssd_bits[i] &= ~BIT(j);
            return ssd_memory[address];
            break;
        case 2:
            usleep(HDD_DELAY);
            i = address/64;
            j = address%64;
            hdd_bits[i] &= ~BIT(j);
            return hdd_memory[address];
            break;
    }
    return 0;
}