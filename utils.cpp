#include "utils.h"

#include <QFile>
#include <QDebug>

int Utils::AppendBytesToFile(QString path, QByteArray &bytes)
{

    QFile file(path);
    if (false == file.open(QIODevice::ReadWrite | QIODevice::Append))
        qWarning() << QString("open file $1 error").arg(path);
    qint64 n = file.write(bytes);

    file.close();

//    qDebug() << QString("AppendBytesToFile: need write %1 bytes, actual write %2 bytes")
//                .arg(bytes.size()).arg(n);

    return n;
}

int Utils::AppendBytesToFile(QString path, const char *bytes, qint64 size)
{
    QFile file(path);
    if (false == file.open(QIODevice::ReadWrite | QIODevice::Append))
        qWarning() << QString("open file $1 error").arg(path);
    qint64 n = file.write(bytes, size);
    file.close();

//    qDebug() << QString("AppendBytesToFile: need write %1 bytes, actual write %2 bytes")
//                .arg(size).arg(n);

    return n;
}
