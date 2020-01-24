#include "settings.h"

#include <QDebug>
#include <QApplication>
#include <QDesktopWidget>
#include <QKeySequence>

using namespace Settings;

//Privé
namespace {
    static int keyCodes[Settings::ACTIONS];
    const int INVALID_KEYCODE = 33554431;

    void setIfNull(QString key, QVariant value) {
        if(!settings->contains(key)) {
            settings->setValue(key, value);
        }
    }

    QString moveToString(Action move) {
        switch(move) {
            case Forward:
                return QString("input/camera/Forward");
            case Backward:
                return QString("input/camera/Backward");
            case RightStrafe:
                return QString("input/camera/RightStrafe");
            case Right:
                return QString("input/camera/Right");
            case LeftStrafe:
                return QString("input/camera/LeftStrafe");
            case Left:
                return QString("input/camera/Left");
            case Up:
                return QString("input/camera/Up");
            case Down:
                return QString("input/camera/Down");
            case Exit:
                return QString("input/Exit");
            case Menu:
                return QString("input/ui/Menu");
            default:
                return QString("error");
        }
    }

    void updateKey(Action act) {
        QString codeStr = settings->value(moveToString(act)).toString();
        bool isNumber = false;
        int code = codeStr.toInt(&isNumber);
        if(!isNumber) {
            code = QKeySequence::fromString(codeStr)[0];
            if(code == INVALID_KEYCODE) {
                qWarning() << "Touche incorrecte !";
            }
        }

        keyCodes[static_cast<unsigned int>(act)] = code;
    }

    void updateKeys() {
        for(uint i = Forward; i < ACTIONS; i++) {
            updateKey(static_cast<Action>(i));
        }
    }
}

//Publique
void Settings::init(QGuiApplication* parent) {
    settings = new QSettings(QSettings::Format::NativeFormat, QSettings::UserScope, QString("Slay3"), QString("SlayAndConquer"), parent);
    settings->sync();
    
    setIfNull("save/Path", "savefile.saq");
    
    setIfNull("game/MapLength", 15);
    setIfNull("game/MapWidth", 15);
    setIfNull("game/NumPlayers", 1);
    setIfNull("game/NumComputers", 2);
    
    setIfNull("fix/input/MouseHack", false);

    QDesktopWidget* desk = QApplication::desktop();
    int mainScreen = desk->primaryScreen();
    setIfNull("display/Resolution", desk->screen(mainScreen)->size());
    setIfNull("display/Fullscreen", true);

    setIfNull(moveToString(Forward), "Z");
    setIfNull(moveToString(Backward), "S");
    setIfNull(moveToString(RightStrafe), "D");
    setIfNull(moveToString(Right), "E");
    setIfNull(moveToString(LeftStrafe), "Q");
    setIfNull(moveToString(Left), "A");
    setIfNull(moveToString(Up), "Space");
    setIfNull(moveToString(Down), "16777248"); //On ne peut pas écrire Shift car c'est une touche modificatrice; On est obligé de mettre son code brut.
    setIfNull(moveToString(Exit), "F35"); //F35 est une touche qui n'est sur quasiment aucun clavier, donc on l'utilise comme touche "null" . Pour fermer, on fait Alt-F4
    setIfNull(moveToString(Menu), "16777216"); //On ne peut pas écrire Echap car c'est une touche modificatrice; On est obligé de mettre son code brut.
    updateKeys();

    setIfNull("input/camera/MouseRotateCamera", false);
    setIfNull("input/camera/MouseMoveCamera", true);
    setIfNull("input/camera/speed", 0.015);

    setIfNull("logging/PrintFPS", false);
    setIfNull("logging/PrintResize", false);
    setIfNull("logging/PrintDebugVulkan", false);

    setIfNull("debug/EnableConsole", false);
    
    settings->sync();
}

bool Settings::is(const char* str) {
    return settings->value(QString(str)).toBool();
}

QString Settings::getString(QString key) {
    return settings->value(key).toString();
}

int Settings::getKey(Action act) {
    return keyCodes[static_cast<unsigned int>(act)];
}

QSize Settings::getResolution() {
    return settings->value("display/Resolution").toSize();
}

float Settings::cameraSpeed() {
    return settings->value("input/camera/speed").toFloat();
}

float Settings::getFloat(QString key) {
    return settings->value(key).toFloat();
}

uint Settings::getUInt(QString key) {
    return settings->value(key).toUInt();
}

int Settings::getInt(QString key) {
    return settings->value(key).toInt();
}

void Settings::finish() {
    settings->sync();
    delete settings;
}


