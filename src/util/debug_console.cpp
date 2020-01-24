#include "debug_console.h"

#include "logic/game.h"
#include "renderer/windu.h"
#include "settings.h"

#include <iostream>
#include <thread>
#include <string>
#include <sstream>

#include <QString>
#include <QVariant>
#include <QApplication>

using namespace std;

namespace  {
    static Game* game;
    static Windu* win;
    static bool stop = false;
    thread* consoleThread;

    string getParameter(istringstream* stream) {
        string s;
        getline(*stream, s, ' ');
        return s;
    }

    void settings(istringstream* split) {
        string par = getParameter(split);
        if(par == "set") {
            Settings::settings->setValue(QString::fromStdString(getParameter(split)), QVariant(QString::fromStdString(getParameter(split))));
        } else if(par == "get") {
            QVariant value = Settings::settings->value(QString::fromStdString(getParameter(split)));
            if(value.isNull()) {
                cout << "Pas de valeur pour cette clé (null)" << endl;
            } else  {
                cout << value.toString().toStdString() << endl;
            }
        } else if(par == "sync") {
            Settings::settings->sync();
        } else {
            cout << "Syntaxe incorrecte. Les commandes possibles sont:" << endl
                 << "settings set <clé> <valeur" << endl
                 << "settings get <clé>" << endl
                 << "settings sync" << endl;
        }
    }

    void pause() {
        win->pause.lock();
    }

    void unpause() {
        win->pause.unlock();
    }

    void help() {
        cout << "help: Affiche cette page d'aide" << endl
             << "settings: Permet de modifier les paramètres" << endl
             << "quit: Stoppe l'éxecution du programme" << endl
             << "pause: Pause le thread principal" << endl
             << "unpause: Continue le thread principal" << endl;
       //      << "game: Permet de modifier des variables de la classe game" << endl
       //      << "windu: Permet de modifier des variables de la classe windu" << endl;
    }

    void quit() {
        QApplication::quit();
    }

    void main() {
        cout << "Console de debug activé. Entrez 'help'" << endl;
        for (string line; !stop && std::getline(std::cin, line);) {
            istringstream split(line);
            string par = getParameter(&split);

            if(par == "help") {
                help();
            } else if(par == "quit") {
                quit();
            } else if(par == "settings") {
                settings(&split);
            } else if(par == "pause") {
                pause();
            } else if(par == "unpause") {
                unpause();
            } else {
                cout << "Commande inconnue. Tapez 'help' pour avoir une liste des commandes disponible." << endl;
            }
        }
    }


}

void DebugConsole::init(Game* gamu, Windu* windu) {
    game = gamu;
    win = windu;
    if(Settings::is("debug/EnableConsole")) {
        consoleThread = new thread(main);
    }
}
