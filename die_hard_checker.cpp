/**
 * A model checker written in C++.
 * Demo https://github.com/jameshfisher/tlaplus/blob/master/examples/DieHard/DieHard.tla
 */

#include <cstddef>

#include "checker.h"

//
// Define the state.
//
struct State : public ModelState<State> {
    int8_t big;
    int8_t small;

    friend bool operator==(const State& lhs, const State& rhs) {
        return lhs.big == rhs.big && lhs.small == rhs.small;
    }

    // TODO: Symmetry.
    template <typename H>
    friend H AbslHashValue(H h, const State& s) {
        return H::combine(std::move(h), s.big, s.small);
    }

    friend std::ostream& operator << (std::ostream &out, const State& s) {
        return out << "fp: " << s.hash() << " [big: " << (int)s.big << ", small: " << (int)s.small << "]";
    }
    bool satisfyInvariant() const;
    void generate();
};

// Define invariant.
bool State::satisfyInvariant() const {
    return big != 4;
}

// Define the model.
void State::generate() {
    // FillSmallJug
    either([&](){ small = 3; });

    // FillBigJug
    either([&](){ big = 5; });

    // EmptySmallJug
    either([&](){ small = 0; });

    // EmptyBigJug
    either([&](){ big = 0; });

    // SmallToBig
    either([&](){
        if (big + small > 5) {
            big = 5;
            small = big + small - 5;
        } else {
            big += small;
            small = 0;
        }
    });

    // BigToSmall
    either([&](){
        if (big + small > 3) {
            big = big + small - 3;
            small = 3;
        } else {
            small += big;
            big = 0;
        }
    });
}

int main(int argv, char** argc) {
    State initialState;
    initialState.big = 0;
    initialState.small = 0;

    Checker<State>::get()->run({initialState});

    return 0;
}
