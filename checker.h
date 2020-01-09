#pragma once

#include <cstddef>
#include <iostream>
#include <sstream>
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
    std::string getStats() const;

    static Checker<StateType>* get() { return globalChecker; }

private:
    struct Stats {
        uint64_t generated = 0;
        uint64_t unique = 0;
        friend std::ostream& operator << (std::ostream &out, const Stats& s) {
            return out << "generated: " << s.generated << " unique: " << s.unique;
        }
    };
    static Checker<StateType>* globalChecker;
    std::vector<StateType> trace(const StateType& endState) const;

    std::unordered_map<Fingerprint, StateType> _seenStates;
    std::queue<StateType> _unvisited;
    Stats _stats;
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

    std::cout << "Model checking finished." << std::endl << getStats() << std::endl;
}

template <class StateType>
void Checker<StateType>::onNewState(const StateType& state) {
    _stats.generated++;

    // If the fp doesn't exist in the unique map, add it.
    auto fp = state.hash();
    if (!_seenStates.insert({fp, state}).second) {
        return;
    }
    _stats.unique++;

    // Check invariant.
    if (!state.satisfyInvariant()) {
        std::cout << "Violated invariant." << std::endl;
        auto errorTrace = trace(state);
        for (size_t i = 0; i < errorTrace.size(); i++) {
            std::cout << "State: " << i << std::endl << errorTrace[i] << std::endl << std::endl;
        }
        throw InvariantViolatedException();
    }

    if (!state.satisfyConstraint()) return;

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

template <class StateType>
std::string Checker<StateType>::getStats() const {
    std::stringstream str;
    str << _stats << " hash table size: " << _seenStates.size();
    return str.str();
}