#include "controller.hpp"

#include <algorithm>

void Controller::UpdateButtonHeld(bool& down, bool held) {
    if (down) {
        this->step = 50;
        this->counter = 0;
    } else if (held) {
        this->counter += this->step;

        if (this->counter >= this->MAX) {
            down = true;
            this->counter = 0;
            this->step = std::min(this->step + 50, this->MAX_STEP);
        }
    }
}

