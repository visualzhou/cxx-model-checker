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
int8_t operator "" _b(unsigned long long number) {
    return (int8_t)number;
}

struct State {
    int8_t big;
    int8_t small;

    Fingerprint prevHash;

    // TODO: Symmetry.
    long long hash() const;

    friend std::ostream& operator << (std::ostream &out, const State& s) {
        return out << "[big: " << (int)s.big << ", small: " << (int)s.small << "]" << std::endl;
    }
    bool satisfyInvariant() const;
};

Fingerprint State::hash() const {
    return ((Fingerprint)big << (sizeof(small) * 8)) | (Fingerprint)small;
}

bool State::satisfyInvariant() const {
    return big != 4_b;
}

class StateGenerator {
public:
    virtual ~StateGenerator() = default;
    // Mutate the state in-place to generate a new state.
    // The new state can be the same as the old one.
    // Passing by reference for convenience.
    virtual void generate(State& state) = 0;

protected:
    void onNewState(State s) { _onNewState(s); }
private:
    std::function<void(State)> _onNewState;
    friend class Checker;
};

class InvariantViolatedException : public std::exception {};

class Checker {
public:
    Checker(StateGenerator* generator) : _generator(generator) {
            // Set the callback on generator.
            generator->_onNewState = [this](State state) { _onNewState(state); };
    }
    void run(StateGenerator* generator, std::vector<State> initialStates);
private:
    void _onNewState(const State&);

    std::vector<State> trace(const State& endState) const;
    StateGenerator* _generator;
    std::unordered_map<Fingerprint, State> _seenStates;
    std::queue<State> _unvisited;
};

void Checker::run(StateGenerator* generator, std::vector<State> initialStates) {
    try {
        for (auto& s : initialStates) {
            _onNewState(s);
        }

        while (!_unvisited.empty()) {
            auto curState = _unvisited.front();
            _unvisited.pop();

            // Create the new state.
            auto newState = curState;
            newState.prevHash = curState.hash();
            generator->generate(newState);
        }
    } catch (InvariantViolatedException& exp) {}
}

void Checker::_onNewState(const State& state) {
    // Check invariant.
    if (!state.satisfyInvariant()) {
        std::cout << "Violated invraiant, last state: " << state << std::endl;
        auto errorTrace = trace(state);
        for (size_t i = 0; i < errorTrace.size(); i++) {
            std::cout << "State: " << i << std::endl << errorTrace[i] << std::endl;
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

std::vector<State> Checker::trace(const State& endState) const {
    std::vector<State> trace;
    trace.push_back(endState);
    auto cur = endState;
    while (cur.prevHash != 0) {
        cur = _seenStates.find(cur.prevHash)->second;
        trace.push_back(cur);
    }
    std::reverse(trace.begin(), trace.end());
    return trace;
}

class DieHardStateGenerator : public StateGenerator {
public:
    ~DieHardStateGenerator() = default;

    void generate(State& state) override {
        // TODO: Replace this with "Either" syntax sugar.
        auto wrap = [&](std::function<void()> fun) {
            // Run the state generator on a state copy.
            auto temp = state;
            fun();
            onNewState(state);
            state = temp;
        };

        // FillSmallJug
        wrap([&](){ state.small = 3_b; });

        // FillBigJug
        wrap([&](){ state.big = 5_b; });

        // EmptySmallJug
        wrap([&](){ state.small = 0_b; });

        // EmptyBigJug
        wrap([&](){ state.big = 0_b; });

        // SmallToBig
        wrap([&](){
            if (state.big + state.small > 5) {
                state.big = 5_b;
                state.small = state.big + state.small - 5_b;
            } else {
                state.big += state.small;
                state.small = 0;
            }
        });

        // BigToSmall
        wrap([&](){
            if (state.big + state.small > 3) {
                state.big = state.big + state.small - 3_b;
                state.small = 3_b;
            } else {
                state.small += state.big;
                state.big = 0;
            }
        });
    }
};

int main(int argv, char** argc) {
    DieHardStateGenerator generator;

    Checker checker(&generator);

    State initialState;
    initialState.big = 0_b;
    initialState.small = 0_b;

    checker.run(&generator, {initialState});

    return 0;
}
