#ifndef SYSTEM_H
#define SYSTEM_H

class Game;
class System {
public:
    System(Game* game);
    Game* game;
};

#endif //SYSTEM_H
