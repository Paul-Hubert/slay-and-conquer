#include <QLoggingCategory>
#include <QApplication>
#include <QDebug>
#include <QTemporaryDir>

#include "renderer/windu.h"
#include "renderer/helper.h"

#include "logic/systems/render_system.h"
#include "logic/systems/ui_system.h"
#include "logic/game.h"

#include "util/settings.h"
#include "util/resource_copy.h"
#include "util/debug_console.h"

Q_LOGGING_CATEGORY(lcVk, "qt.vulkan")

int main(int argc, char *argv[]) {
    
    QApplication app(argc, argv);
    
    app.addLibraryPath(app.applicationDirPath());
    
    ResourceCopy::init();
    Settings::init(&app);
    
    if(Settings::is("logging/PrintDebugVulkan")) QLoggingCategory::setFilterRules(QStringLiteral("qt.vulkan=true"));

    Windu* win = new Windu(Settings::getResolution());

    if(Settings::is("display/Fullscreen")) {
        win->showFullScreen();
    } else {
        win->show();
    }
    
    Game* game = new Game();

    RenderSystem* view = new RenderSystem(game, win);
    UISystem* ui = new UISystem(game, win);
    
    DebugConsole::init(game, win);

    int returnCode = app.exec();

    delete win;
    delete ui;
    delete view;
    delete game;

    ResourceCopy::finish();
    Settings::finish();

    return returnCode;
}
