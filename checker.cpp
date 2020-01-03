/**
 * A model checker written in C++.
 * Demo https://github.com/jameshfisher/tlaplus/blob/master/examples/DieHard/DieHard.tla
 */

#include <cstddef>
#include <iostream>
#include <queue>
#include <unordered_map>
#include <exception>

using FingerPrint = long long;
int8_t operator "" _b(unsigned long long number) {
    return (int8_t)number;
}

struct State {
    int8_t big; 
    int8_t small;

    FingerPrint prevHash;

    // TODO: Symmetry.
    long long hash() const;

    friend std::ostream& operator << (std::ostream &out, const State& s) {
        return out << "[big: " << (int)s.big << ", small: " << (int)s.small << "]" << std::endl;
    }
};

FingerPrint State::hash() const {
    return ((FingerPrint)big << (sizeof(small) * 8)) | (FingerPrint)small;
}

class StateChecker {
public:
    virtual ~StateChecker() = default;
    virtual bool satisfyInvariant(const State&) { return true; }
    // TODO: Add check constraints.
};

class StateGenerator {
public:
    StateGenerator(std::function<void(State)> onNewState) : _onNewState(onNewState) {}
    virtual ~StateGenerator() = default;
    // Mutate the state in-place to generate a new state.
    // The new state can be the same as the old one.
    // Passing by reference for convenience.
    virtual void generate(State& state) = 0;
protected:
    std::function<void(State)> _onNewState;
};

class Checker {
public:
    Checker(StateChecker* stateChecker) : _stateChecker(stateChecker) {}
    void run(StateGenerator* generator);
    void onNewState(const State&);
private:
    StateChecker* _stateChecker;
    std::unordered_map<FingerPrint, State> _seenStates;
    std::queue<State> _unvisited;
};

void Checker::run(StateGenerator* generator) {
    while (!_unvisited.empty()) {
        auto curState = _unvisited.front();
        _unvisited.pop();

        // Create the new state.
        auto newState = curState;
        newState.prevHash = curState.hash();
        generator->generate(newState);
    }
}

void Checker::onNewState(const State& state) {
    // Check invariant.
    if (!_stateChecker->satisfyInvariant(state)) {
        std::cout << "Violated invraiant, last state: " << state << std::endl;
        throw std::exception();
    }
    
    // If the fp doesn't exist in the unique map, add it.
    auto fp = state.hash();
    if (!_seenStates.insert({fp, state}).second) {
        return;
    }

    // Add the new to unvisited.
    _unvisited.push(state);
}

class DieHardStateChecker : public StateChecker {
public:
    ~DieHardStateChecker() = default;
    bool satisfyInvariant(const State& state) override {
        return state.big != 4_b;
    }
};

class DieHardStateGenerator : public StateGenerator {
public:
    DieHardStateGenerator(std::function<void(State)> onNewState) : StateGenerator(onNewState){}
    ~DieHardStateGenerator() = default;

    void generate(State& state) override {
        auto wrap = [&](std::function<void()> fun) {
            // Run the state generator on a state copy.
            auto temp = state;
            fun();
            _onNewState(state);
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

void test() {
    State s;
    s.big = 5_b;
    std::cout << s.hash() << std::endl;
}
int main(int argv, char** argc) {
    // test();

    DieHardStateChecker stateChecker;
    Checker checker(&stateChecker);
    DieHardStateGenerator generator([&](State state){ checker.onNewState(state); });
    
    State s;
    s.big = 0_b;
    s.small = 0_b;
    checker.onNewState(s);

    checker.run(&generator);

    return 0;
}