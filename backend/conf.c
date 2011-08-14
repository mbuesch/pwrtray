/*
 *   Copyright (C) 2010 Michael Buesch <m@bues.ch>
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation; either version 2
 *   of the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 */

#include "conf.h"
#include "log.h"
#include "util.h"
#include "fileaccess.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>


static inline uint_fast8_t conf_hash(const char *str)
{
	return tiny_hash(str) & CONF_HASHTAB_MASK;
}

#define hashtab_add(tab, element)					\
	do {								\
		unsigned int _hash = conf_hash((element)->name);	\
		typeof(*(element)) *a, *b;				\
		(element)->next = NULL;					\
		if (tab[_hash]) {					\
			b = tab[_hash];					\
			for (; b; a = b, b = b->next);			\
			a->next = (element);				\
		} else							\
			tab[_hash] = (element);				\
	} while (0)

#define hashtab_get(tab, element_name)					\
	({								\
		typeof(*(tab[0])) *_e;					\
		for (_e = tab[conf_hash(element_name)];			\
		     _e && strcmp(_e->name, (element_name)) != 0;	\
		     _e = _e->next);					\
		_e;							\
	})

const char * config_get(struct config_file *f,
			const char *section,
			const char *item,
			const char *_default)
{
	struct config_section *s;
	struct config_item *i;

	if (!f || !section || !item)
		return _default;

	s = hashtab_get(f->section_hashtab, section);
	if (s) {
		i = hashtab_get(s->item_hashtab, item);
		if (i)
			return i->value;
	}

	return _default;
}

static int string_to_int(const char *string, int *i)
{
	char *tailptr;
	long retval;

	retval = strtol(string, &tailptr, 0);
	if (tailptr == string || tailptr[0] != '\0')
		return -EINVAL;
	*i = retval;

	return 0;
}

int config_get_int(struct config_file *f,
		   const char *section,
		   const char *item,
		   int _default)
{
	const char *value;
	int i;

	value = config_get(f, section, item, NULL);
	if (!value)
		return _default;
	if (string_to_int(value, &i))
		return _default;

	return i;
}

int config_get_bool(struct config_file *f,
		    const char *section,
		    const char *item,
		    int _default)
{
	const char *value;
	int i;

	value = config_get(f, section, item, NULL);
	if (!value)
		return _default;
	if (strcasecmp(value, "yes") == 0 ||
	    strcasecmp(value, "true") == 0 ||
	    strcasecmp(value, "on") == 0)
		return 1;
	if (strcasecmp(value, "no") == 0 ||
	    strcasecmp(value, "false") == 0 ||
	    strcasecmp(value, "off") == 0)
		return 0;
	if (string_to_int(value, &i))
		return _default;

	return !!i;
}

static void dump_config_tree(struct config_file *f)
{
	struct config_section *s;
	struct config_item *i;
	unsigned int j, k;
	int sect_coll, item_coll;
	const char *collstr = "\t\t<== ### COLLISION ###";

	logverbose("Parsed config file:\n");
	for (j = 0; j < CONF_HASHTAB_SIZE; j++) {
		sect_coll = 0;
		for (s = f->section_hashtab[j]; s; s = s->next) {
			logverbose("\t[%02X] Section '%s'%s\n",
				   j & 0xFF, s->name,
				   sect_coll ? collstr : "");
			for (k = 0; k < CONF_HASHTAB_SIZE; k++) {
				item_coll = 0;
				for (i = s->item_hashtab[k]; i; i = i->next) {
					logverbose("\t\t[%02X] Item '%s'%s\n",
						   k & 0xFF, i->name,
						   item_coll ? collstr : "");
					item_coll = 1;
				}
			}
			sect_coll = 1;
		}
	}
}

struct config_file * config_file_parse(const char *path)
{
	struct config_file *retval = NULL, *c;
	struct config_section *s = NULL;
	struct config_item *i;
	struct fileaccess *fa = NULL;
	LIST_HEAD(lines);
	struct text_line *l;
	char *line, *name, *value;
	int err;
	size_t len;
	unsigned int lineno = 0;

	c = zalloc(sizeof(*c));
	if (!c)
		goto out_error;

	fa = file_open(O_RDONLY, path);
	if (!fa)
		goto out_ok;
	err = file_read_text_lines(fa, &lines, 1);
	if (err) {
		logerr("Failed to read config file %s: %s\n",
		       path, strerror(-err));
		goto out_error;
	}

	list_for_each_entry(l, &lines, list) {
		lineno++;
		line = l->text;
		len = strlen(line);
		if (!len)
			continue;
		if (line[0] == '#') /* comment */
			continue;
		if (len >= 3 && line[0] == '[' && line[len - 1] == ']') {
			/* New section */
			s = zalloc(sizeof(*s));
			if (!s)
				goto error_unwind;
			s->file = c;
			line[len - 1] = '\0';
			s->name = strdup(line + 1);
			if (!s->name) {
				free(s);
				goto error_unwind;
			}
			hashtab_add(c->section_hashtab, s);
			continue;
		}
		if (!s) {
			logerr("%s:%u: Stray characters\n", path, lineno);
			goto error_unwind;
		}
		/* Config item in section */
		value = strchr(line, '=');
		if (!value) {
			logerr("%s:%u: Invalid config item\n", path, lineno);
			goto error_unwind;
		}
		value[0] = '\0';
		value++;
		name = line;
		i = zalloc(sizeof(*i));
		if (!i)
			goto error_unwind;
		i->section = s;
		i->name = strdup(name);
		if (!i->name) {
			free(i);
			goto error_unwind;
		}
		i->value = strdup(value);
		if (!i->value) {
			free(i->name);
			free(i);
			goto error_unwind;
		}
		hashtab_add(s->item_hashtab, i);
	}
out_ok:
	retval = c;
	if (loglevel_is_verbose())
		dump_config_tree(retval);

out_error:
	text_lines_free(&lines);
	file_close(fa);

	return retval;

error_unwind:
	config_file_free(c);
	goto out_error;
}

static void free_item(struct config_item *i)
{
	if (i) {
		free(i->name);
		free(i->value);
		free(i);
	}
}

static void free_section(struct config_section *s)
{
	unsigned int i;
	struct config_item *it, *next;

	if (s) {
		for (i = 0; i < CONF_HASHTAB_SIZE; i++) {
			for (it = s->item_hashtab[i]; it; it = next) {
				next = it->next;
				free_item(it);
			}
		}
		free(s->name);
		free(s);
	}
}

void config_file_free(struct config_file *f)
{
	unsigned int i;
	struct config_section *s, *next;

	if (f) {
		for (i = 0; i < CONF_HASHTAB_SIZE; i++) {
			for (s = f->section_hashtab[i]; s; s = next) {
				next = s->next;
				free_section(s);
			}
		}
		free(f);
	}
}
