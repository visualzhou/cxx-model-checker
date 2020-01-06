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
#include "abseil-cpp/absl/hash/hash.h"

using Fingerprint = uint64_t;

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

template <class StateType>
struct ModelState {
    Fingerprint prevHash;
    Fingerprint hash() const {
        return absl::Hash<StateType>{}(*static_cast<const StateType*>(this));
    }
protected:
    void either(const std::function<void()>& fun) {
        // Generate states on a copy of the current state.
        StateType temp = getState();
        fun();
        Checker<StateType>::get()->onNewState(getState());
        getState() = temp;
    }

private:
    StateType& getState() { return *static_cast<StateType*>(this); }
};

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
            onNewState(newState);
        }
    } catch (InvariantViolatedException& exp) {}
}

template <class StateType>
void Checker<StateType>::onNewState(const StateType& state) {
    // If the fp doesn't exist in the unique map, add it.
    auto fp = state.hash();
    if (!_seenStates.insert({fp, state}).second) {
        return;
    }

    // Check invariant.
    if (!state.satisfyInvariant()) {
        std::cout << "Violated invraiant, last state: " << state << std::endl;
        auto errorTrace = trace(state);
        for (size_t i = 0; i < errorTrace.size(); i++) {
            std::cout << "State: " << i << std::endl << errorTrace[i] << std::endl << std::endl;
        }
        throw InvariantViolatedException();
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
