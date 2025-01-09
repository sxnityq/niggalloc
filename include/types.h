
void coalesce(void *);
void nigga_free(void *);
void mem_init();
void *niggalloc(int);


/* CURRENTLY NOT USED */

struct Nigga_general {
    unsigned int block_size : 29;
    unsigned int info       : 3;
};
