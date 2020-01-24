#ifndef RESOURCECOPY_H
#define RESOURCECOPY_H

#include <QFile>
#include <QTemporaryDir>

namespace ResourceCopy {
    void init();
    QString loadResource(QString resourcePath);
    QString dirPath();
    void finish();
};

#endif // RESOURCECOPY_H
