#ifndef UTILS_H
#define UTILS_H

#include <QString>

class Utils
{
public:

    static int AppendBytesToFile(QString path, QByteArray &bytes);
    static int AppendBytesToFile(QString path, const char *bytes, qint64 size);
};

#endif // UTILS_H
