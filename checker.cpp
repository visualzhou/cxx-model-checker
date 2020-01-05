/**
 * A model checker written in C++.
 * Demo https://github.com/jameshfisher/tlaplus/blob/master/examples/DieHard/DieHard.tla
 */

#include <cstddef>
#include <iostream>
#include <functional>
#include <queue>
#include <unordered_map>
#include <exception>
#include <algorithm>

using Fingerprint = long long;

template <class StateType>
class Checker {
public:
    void run(std::vector<StateType> initialStates);
    void onNewState(const StateType&);

    static Checker<StateType>* get() { return globalChecker; }

private:
    static Checker<StateType>* globalChecker;
    std::vector<StateType> trace(const StateType& endState) const;

    std::unordered_map<Fingerprint, StateType> _seenStates;
    std::queue<StateType> _unvisited;
};

template <class StateType>
Checker<StateType>* Checker<StateType>::globalChecker = new Checker<StateType>;

int8_t operator "" _b(unsigned long long number) {
    return (int8_t)number;
}

struct State {
    int8_t big;
    int8_t small;

    Fingerprint prevHash;

    // TODO: Symmetry.
    Fingerprint hash() const;

    friend std::ostream& operator << (std::ostream &out, const State& s) {
        return out << "[big: " << (int)s.big << ", small: " << (int)s.small << "]";
    }
    bool satisfyInvariant() const;
    void generate();
};

Fingerprint State::hash() const {
    return ((Fingerprint)big << (sizeof(small) * 8)) | (Fingerprint)small;
}

bool State::satisfyInvariant() const {
    return big != 4_b;
}

class InvariantViolatedException : public std::exception {};

template <class StateType>
void Checker<StateType>::run(std::vector<StateType> initialStates) {
    try {
        for (auto& s : initialStates) {
            onNewState(s);
        }

        while (!_unvisited.empty()) {
            auto curState = _unvisited.front();
            _unvisited.pop();

            // Create the new state.
            auto newState = curState;
            newState.prevHash = curState.hash();
            newState.generate();
        }
    } catch (InvariantViolatedException& exp) {}
}

template <class StateType>
void Checker<StateType>::onNewState(const StateType& state) {
    // Check invariant.
    if (!state.satisfyInvariant()) {
        std::cout << "Violated invraiant, last state: " << state << std::endl;
        auto errorTrace = trace(state);
        for (size_t i = 0; i < errorTrace.size(); i++) {
            std::cout << "State: " << i << std::endl << errorTrace[i] << std::endl << std::endl;
        }
        throw InvariantViolatedException();
    }

    // If the fp doesn't exist in the unique map, add it.
    auto fp = state.hash();
    if (!_seenStates.insert({fp, state}).second) {
        return;
    }

    // Add the new to the unvisited queue.
    _unvisited.push(state);
}

template <class StateType>
std::vector<StateType> Checker<StateType>::trace(const StateType& endState) const {
    std::vector<StateType> trace;
    trace.push_back(endState);
    auto cur = endState;
    while (cur.prevHash != 0) {
        cur = _seenStates.find(cur.prevHash)->second;
        trace.push_back(cur);
    }
    std::reverse(trace.begin(), trace.end());
    return trace;
}

void State::generate() {
    auto wrap = [&](std::function<void()> fun) {
        // Generate states on a copy of the current state.
        State temp = *this;
        fun();
        Checker<State>::get()->onNewState(*this);
        *(State*)this = temp;
    };

    // FillSmallJug
    wrap([&](){ small = 3_b; });

    // FillBigJug
    wrap([&](){ big = 5_b; });

    // EmptySmallJug
    wrap([&](){ small = 0_b; });

    // EmptyBigJug
    wrap([&](){ big = 0_b; });

    // SmallToBig
    wrap([&](){
        if (big + small > 5) {
            big = 5_b;
            small = big + small - 5_b;
        } else {
            big += small;
            small = 0;
        }
    });

    // BigToSmall
    wrap([&](){
        if (big + small > 3) {
            big = big + small - 3_b;
            small = 3_b;
        } else {
            small += big;
            big = 0;
        }
    });
}

int main(int argv, char** argc) {
    State initialState;
    initialState.big = 0_b;
    initialState.small = 0_b;

    Checker<State>::get()->run({initialState});

    return 0;
}
