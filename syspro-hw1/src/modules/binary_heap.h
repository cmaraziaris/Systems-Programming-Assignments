
struct binary_heap;


struct binary_heap *bh_create(int (*compare_func)(void *a, void *b), void (*destroy_func)(void *data));

int bh_size(struct binary_heap *bh);

void bh_insert(struct binary_heap *bh, void *data);

void *bh_remove_max(struct binary_heap *bh);

void bh_destroy(struct binary_heap *bh);
