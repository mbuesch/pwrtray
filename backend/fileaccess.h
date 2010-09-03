#ifndef BACKEND_FILEACCESS_H_
#define BACKEND_FILEACCESS_H_

#include "list.h"

#include <stdarg.h>
#include <fcntl.h>
#include <stdio.h>
#include <dirent.h>


struct fileaccess {
	int fd;
	FILE *stream;
};

void file_close(struct fileaccess *fa);
struct fileaccess * file_open(int flags, const char *path_fmt, ...);
struct fileaccess * sysfs_file_open(int flags, const char *path_fmt, ...);
struct fileaccess * procfs_file_open(int flags, const char *path_fmt, ...);

int file_read_int(struct fileaccess *fa, int *value, int base);
int file_write_int(struct fileaccess *fa, int value, int base);
int file_read_bool(struct fileaccess *fa, int *value);
int file_write_bool(struct fileaccess *fa, int value);

struct text_line {
	char *text;
	struct list_head list;
};

int file_read_text_lines(struct fileaccess *fa, struct list_head *lines_list);
void text_line_free(struct text_line *tl);
void text_lines_free(struct list_head *lines_list);

struct dir_entry {
	char *name;
	unsigned int type; /* DT_... */
	struct list_head list;
};

int list_directory(struct list_head *dir_entries, const char *path_fmt, ...);
int list_sysfs_directory(struct list_head *dir_entries, const char *path_fmt, ...);
int list_procfs_directory(struct list_head *dir_entries, const char *path_fmt, ...);
void dir_entry_free(struct dir_entry *dir_entry);
void dir_entries_free(struct list_head *dir_entries);

#endif /* BACKEND_FILEACCESS_H_ */
