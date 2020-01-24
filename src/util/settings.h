#ifndef SETTINGS_H
#define SETTINGS_H

#include <QGuiApplication>
#include <QString>
#include <QSize>
#include <QSettings>

namespace Settings {
    
    void init(QGuiApplication* parent);
    bool is(const char* str);

    enum Action {Forward = 0, Backward, RightStrafe, Right, LeftStrafe, Left, Up, Down, Exit, Menu};
    const unsigned int ACTIONS = Menu + 1;

    int getKey(Action move);

    QSize getResolution();
    
    QString getString(QString key);

    float cameraSpeed();
    
    float getFloat(QString key);
    uint getUInt(QString key);
    int getInt(QString key);

    void finish();

    static QSettings* settings;

    static QString hoi("ya");
    
};

#endif // SETTINGS_H
