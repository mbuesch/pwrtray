/*
 *   Copyright (C) 2010 Michael Buesch <mb@bu3sch.de>
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

#include "fileaccess.h"


#define SYSFS_BASE	"/sys"
#define PROCFS_BASE	"/proc"


FileAccess::FileAccess(const QString &name)
 : QFile (name)
{
}

FileAccess::~FileAccess()
{
	close();
}

bool FileAccess::listDir(const QString &path, QStringList *list, QDir::Filters filters)
{
	QDir dir(path);
	if (!dir.isReadable())
		return false;
	*list = dir.entryList(filters);
	return true;
}

bool FileAccess::readInt(int *value, int base)
{
	seek(0);
	QByteArray data(readAll());
	bool ok;
	*value = data.trimmed().toInt(&ok, base);
	return ok;
}

bool FileAccess::writeInt(int value, int base)
{
	QByteArray data(QByteArray::number(value, base));
	seek(0);
	bool ok = (write(data) == data.size());
	waitForBytesWritten(-1);
	return ok;
}

bool FileAccess::readBool(bool *value)
{
	int i;
	bool ok = readInt(&i);
	if (!ok || i < 0)
		return false;
	*value = (i != 0);
	return true;
}

bool FileAccess::writeBool(bool value)
{
	return writeInt(value ? 1 : 0);
}

bool FileAccess::readTextLines(QStringList *lines)
{
	lines->clear();
	seek(0);
	while (1) {
		QByteArray line(readLine(1024));
		if (line.isEmpty())
			break;
		lines->append(QString(line));
	}
	return !lines->isEmpty();
}

bool SysFsFile::listDir(const QString &path, QStringList *list, QDir::Filters filters)
{
	return FileAccess::listDir(QString(SYSFS_BASE "/") + path, list, filters);
}

SysFsFile * SysFsFile::openFile(const QString &path, QFile::OpenMode mode)
{
	SysFsFile *file = new SysFsFile(QString(SYSFS_BASE "/") + path);
	if (!file->open(mode)) {
		delete file;
		return NULL;
	}
	return file;
}

bool ProcFsFile::listDir(const QString &path, QStringList *list, QDir::Filters filters)
{
	return FileAccess::listDir(QString(PROCFS_BASE "/") + path, list, filters);
}

ProcFsFile * ProcFsFile::openFile(const QString &path, QFile::OpenMode mode)
{
	ProcFsFile *file = new ProcFsFile(QString(PROCFS_BASE "/") + path);
	if (!file->open(mode)) {
		delete file;
		return NULL;
	}
	return file;
}
