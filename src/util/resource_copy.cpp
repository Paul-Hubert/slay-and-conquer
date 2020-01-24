#include "resource_copy.h"

#include <QDebug>

namespace {
    static QTemporaryDir* dir;
}

void ResourceCopy::init() {
    dir = new QTemporaryDir();
}

QString ResourceCopy::loadResource(QString resourcePath) {
    if(dir->isValid()) {
        QString copyDest = dir->filePath(QFileInfo(resourcePath).fileName());
        QFile::copy(resourcePath, copyDest);
        return copyDest;
    } else {
        qWarning() << "Impossible de charger : " << resourcePath;
        return QString();
    }
}

QString ResourceCopy::dirPath() {
    return dir->path();
}

void ResourceCopy::finish() {
    delete dir;
}
