#include <sys/types.h>

typedef unsigned short vAddr;

struct page {
	int index;
	struct timeval modTime;
    unsigned short allocated ;//: 1,
	unsigned short location ;//: 2,
    unsigned short r ;//: 1,
	unsigned short frame ;//: 12;

};

vAddr create_page();
u_int32_t* get_value(vAddr address);
void store_value(vAddr address, u_int32_t *value);
void free_page(vAddr address);

void to_ram(struct page *page);
void to_ssd(struct page *page);
void to_hdd(struct page *page);
int evict_ram(void);
int evict_ssd(void);
int pop_mem(int location, int address);

struct page* clock_evict(int location);
struct page* random_evict(int location);
struct page* filo_evict(int location);
struct page* time_evict(int location);


static struct page *(*evict_algo)(int location) = &random_evict;// &clock_evict;//&random_evict;