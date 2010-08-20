#ifndef FILEACCESS_H_
#define FILEACCESS_H_

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QFile>
#include <QtCore/QDir>


class FileAccess : public QFile
{
public:
	FileAccess(const QString &name);
	virtual ~FileAccess();

	static bool listDir(const QString &path, QStringList *list,
			    QDir::Filters filters = QDir::NoDotAndDotDot);

	bool readInt(int *value, int base=10);
	bool writeInt(int value, int base=10);

	bool readBool(bool *value);
	bool writeBool(bool value);

	bool readTextLines(QStringList *lines);
};

class SysFsFile : public FileAccess
{
protected:
	SysFsFile(const QString &name)
		 : FileAccess(name)
		{ }
public:
	virtual ~SysFsFile()
		{ }

public:
	static bool listDir(const QString &path, QStringList *list,
			    QDir::Filters filters = QDir::NoDotAndDotDot);
	static SysFsFile * openFile(const QString &path,
				    QFile::OpenMode mode = QFile::ReadOnly);
};

class ProcFsFile : public FileAccess
{
protected:
	ProcFsFile(const QString &name)
		 : FileAccess(name)
		{ }
public:
	virtual ~ProcFsFile()
		{ }

public:
	static bool listDir(const QString &path, QStringList *list,
			    QDir::Filters filters = QDir::NoDotAndDotDot);
	static ProcFsFile * openFile(const QString &path,
				     QFile::OpenMode mode = QFile::ReadOnly);
};

#endif /* FILEACCESS_H_ */
