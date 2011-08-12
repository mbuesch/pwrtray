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
	struct config_item *i, *i_tmp;

	if (s) {
		list_for_each_entry_safe(i, i_tmp, &s->items, list)
			free_item(i);
		free(s->name);
		free(s);
	}
}

const char * config_get(struct config_file *f,
			const char *section,
			const char *item,
			const char *_default)
{
	struct config_section *s;
	struct config_item *i;
	const char *retval = _default;

	if (!f || !section || !item)
		return _default;
	list_for_each_entry(s, &f->sections, list) {
		if (strcmp(s->name, section) == 0) {
			list_for_each_entry(i, &s->items, list) {
				if (strcmp(i->name, item) == 0) {
					retval = i->value;
					break;
				}
			}
			break;
		}
	}

	return retval;
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
	INIT_LIST_HEAD(&c->sections);

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
			INIT_LIST_HEAD(&s->items);
			INIT_LIST_HEAD(&s->list);
			line[len - 1] = '\0';
			s->name = strdup(line + 1);
			if (!s->name) {
				free(s);
				goto error_unwind;
			}
			list_add_tail(&s->list, &c->sections);
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
		INIT_LIST_HEAD(&i->list);
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
		list_add_tail(&i->list, &s->items);
	}
out_ok:
	retval = c;

out_error:
	text_lines_free(&lines);
	file_close(fa);

	return retval;

error_unwind:
	config_file_free(c);
	goto out_error;
}

void config_file_free(struct config_file *f)
{
	struct config_section *s, *s_tmp;

	if (f) {
		list_for_each_entry_safe(s, s_tmp, &f->sections, list)
			free_section(s);
		free(f);
	}
}
