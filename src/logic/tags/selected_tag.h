#ifndef SELECTED_TAG_H
#define SELECTED_TAG_H

class SelectedTag final {
public:
    enum State {unit, structure};
    SelectedTag(State state = unit) : state(state) {};
    State state;
};

#endif
