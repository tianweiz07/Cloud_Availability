CC = gcc

all: user

user:
	$(CC) -o search_cache search_cache.c
	$(CC) -o check_conflicts_timing check_conflicts_timing.c
	$(CC) -o check_conflicts_set check_conflicts_set.c
	$(CC) -o clean_cache_all clean_cache_all.c -lpthread
	$(CC) -o clean_cache_spec clean_cache_spec.c -lpthread
clean:
	rm -rf search_cache check_conflicts_timing check_conflicts_set clean_cache_all clean_cache_spec
