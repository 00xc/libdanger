#define _GNU_SOURCE
#include <assert.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "dngr_domain.h"

#define NUM_READERS 1
#define NUM_WRITERS 1
#define NUM_ITERS 200000
#define ARR_SIZE(x) sizeof(x)/sizeof(*x)

typedef unsigned int uint;

typedef struct {
	uint v1;
	uint v2;
	uint v3;
} Config;

static Config* shared_config;
static DngrDomain* config_dom;
static pthread_barrier_t barrier;

Config* create_config() {
	Config* out;

	out = calloc(1, sizeof(Config));
	if (out == NULL)
		err(EXIT_FAILURE, "calloc");
	return out;
}

void delete_config(void* conf) {
	assert(conf != NULL);
	free(conf);
}

void print_config(const char* name, const Config* conf) {
	printf("%s : { 0x%08x, 0x%08x, 0x%08x }\n",
		name, conf->v1, conf->v2, conf->v3);
}

void init() {
	shared_config = create_config();
	config_dom = dngr_domain_new(delete_config);
	if (config_dom == NULL)
		err(EXIT_FAILURE, "dngr_domain_new");
	if (pthread_barrier_init(&barrier, NULL, NUM_READERS + NUM_WRITERS) != 0)
		err(EXIT_FAILURE, "pthread_barrier_init");
}

void deinit() {
	delete_config(shared_config);
	dngr_domain_free(config_dom);
	pthread_barrier_destroy(&barrier);
}

void* reader_thread(void* arg) {
	Config* safe_config;
	unsigned long i;
	int ret;

	(void)arg;

	ret = pthread_barrier_wait(&barrier);
	if (ret != 0 && ret != PTHREAD_BARRIER_SERIAL_THREAD)
		err(EXIT_FAILURE, "pthread_barrier_wait");

	for (i = 0; i < NUM_ITERS; ++i) {
		safe_config = (Config*)dngr_load(config_dom, (uintptr_t*)&shared_config);
		if (safe_config == NULL)
			err(EXIT_FAILURE, "dngr_load");

		print_config("read config    ", safe_config);
		dngr_drop(config_dom, (uintptr_t)safe_config);
	}

	return NULL;
}

void* writer_thread(void* arg) {
	Config* new_config;
	uint i;
	int ret;

	(void)arg;

	ret = pthread_barrier_wait(&barrier);
	if (ret != 0 && ret != PTHREAD_BARRIER_SERIAL_THREAD)
		err(EXIT_FAILURE, "pthread_barrier_wait");

	for (i = 0; i < NUM_ITERS / 2; ++i) {
		new_config = create_config();
		new_config->v1 = rand();
		new_config->v2 = rand();
		new_config->v3 = rand();
		print_config("updating config", new_config);

		dngr_swap(config_dom, (uintptr_t*)&shared_config, (uintptr_t)new_config, 0);
		print_config("updated config ", new_config);
	}

	return NULL;
}

int main() {
	pthread_t readers[NUM_READERS];
	pthread_t writers[NUM_WRITERS];
	unsigned int i;

	init();

	/* Start threads */
	for (i = 0; i < ARR_SIZE(readers); ++i) {
		if (pthread_create(readers + i, NULL, reader_thread, NULL))
			warn("pthread_create");
	}
	for (i = 0; i < ARR_SIZE(writers); ++i) {
		if (pthread_create(writers + i, NULL, writer_thread, NULL))
			warn("pthread_create");
	}

	/* Wait for threads to finish */
	for (i = 0; i < ARR_SIZE(readers); ++i) {
		if (pthread_join(readers[i], NULL))
			warn("pthread_join");
	}
	for (i = 0; i < ARR_SIZE(writers); ++i) {
		if (pthread_join(writers[i], NULL))
			warn("pthread_join");
	}

	deinit();

	return EXIT_SUCCESS;
}
