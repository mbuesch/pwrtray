#ifndef CONFIG_FILE_H_
#define CONFIG_FILE_H_

#include "list.h"

struct config_item;
struct config_section;
struct config_file;

struct config_item {
	struct config_section *section;
	char *name;
	char *value;

	struct list_head list;
};

struct config_section {
	struct config_file *file;
	char *name;
	struct list_head items;

	struct list_head list;
};

struct config_file {
	struct list_head sections;
};

const char * config_get(struct config_file *f,
			const char *section,
			const char *item,
			const char *_default);

int config_get_int(struct config_file *f,
		   const char *section,
		   const char *item,
		   int _default);

int config_get_bool(struct config_file *f,
		    const char *section,
		    const char *item,
		    int _default);

struct config_file * config_file_parse(const char *path);
void config_file_free(struct config_file *f);

#endif /* CONFIG_FILE_H_ */
