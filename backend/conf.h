#ifndef CONFIG_FILE_H_
#define CONFIG_FILE_H_


#define CONF_HASHTAB_BITS	5
#define CONF_HASHTAB_SIZE	(1 << CONF_HASHTAB_BITS)
#define CONF_HASHTAB_MASK	(CONF_HASHTAB_SIZE - 1)


struct config_item;
struct config_section;
struct config_file;

struct config_item {
	struct config_section *section;
	char *name;
	char *value;

	struct config_item *next;
};

struct config_section {
	struct config_file *file;
	char *name;

	struct config_item *item_hashtab[CONF_HASHTAB_SIZE];

	struct config_section *next;
};

struct config_file {
	struct config_section *section_hashtab[CONF_HASHTAB_SIZE];
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
