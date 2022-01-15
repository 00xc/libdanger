#define _GNU_SOURCE
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "dngr_domain.h"

#define NUM_ITERS 20

typedef unsigned int uint;

typedef struct {
	uint v1;
	uint v2;
	uint v3;
} Config;

Config* shared_config;
DngrDomain* config_dom;

Config* create_config() {
	Config* out;

	out = calloc(1, sizeof(Config));
	if (out == NULL)
		err(EXIT_FAILURE, "calloc");
	return out;
}

void delete_config(Config* conf) {
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
}

void deinit() {
	delete_config(shared_config);
	dngr_domain_free(config_dom);
}

void* reader_thread(void* arg) {
	Config* safe_config;
	unsigned long i;

	(void)arg;

	/* Wait until the writer starts modifying the config */
	while (1) {
		safe_config = dngr_load(config_dom, &shared_config);
		i = safe_config->v1 + safe_config->v2;
		dngr_drop(config_dom, safe_config);
		if (i != 0)
			break;
	}

	for (i = 0; i < NUM_ITERS; ++i) {
		safe_config = dngr_load(config_dom, &shared_config);
		print_config("read config   ", safe_config);
		dngr_drop(config_dom, safe_config);
		pthread_yield();
		pthread_yield();
	}

	return NULL;
}

void* writer_thread(void* arg) {
	Config* new_config;
	uint i;

	(void)arg;

	srand(0x1000);

	for (i = 0; i < NUM_ITERS/2; ++i) {
		new_config = create_config();
		new_config->v1 = rand();
		new_config->v2 = rand();
		new_config->v3 = rand();
		print_config("updated config", new_config);
		dngr_swap(config_dom, &shared_config, new_config, 0);
		pthread_yield();
	}

	return NULL;
}

int main() {
	pthread_t rd_thrd;
	pthread_t wr_thrd;

	init();

	if (pthread_create(&rd_thrd, NULL, reader_thread, NULL))
		warn("pthread_create");

	if (pthread_create(&wr_thrd, NULL, writer_thread, NULL))
		warn("pthread_create");

	if (pthread_join(rd_thrd, NULL) || pthread_join(wr_thrd, NULL))
		warn("pthread_join");

	deinit();

	return EXIT_SUCCESS;
}
