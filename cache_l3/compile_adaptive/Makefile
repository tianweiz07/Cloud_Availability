CC = gcc

all: user

user:
	$(CC) -w -o search_cache search_cache.c
	$(CC) -w -o check_conflicts_timing check_conflicts_timing.c
	$(CC) -w -o check_conflicts_set check_conflicts_set.c
	$(CC) -w -o clean_cache_spec1 clean_cache_spec1.c -lpthread
	$(CC) -w -o clean_cache_spec4 clean_cache_spec4.c -lpthread
clean:
	rm -rf search_cache check_conflicts_timing check_conflicts_set clean_cache_spec1 clean_cache_spec4
