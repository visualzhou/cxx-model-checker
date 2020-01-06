/**
 * A model checker written in C++.
 * Demo https://github.com/jameshfisher/tlaplus/blob/master/examples/DieHard/DieHard.tla
 */

#include <cstddef>

#include "checker.h"
#include <vector>

//
// Define the state.
//
using TermType = uint8_t;
enum RaftState { Primary, Secondary };
enum Node { N1, N2, N3, ALL_NODES };
using LogEntry = TermType;
std::vector<Node> all_nodes = {N1, N2, N3};

std::ostream& operator << (std::ostream &out, const TermType& v) {
    return out << (uint)v;
}
template <class T>
std::ostream& operator << (std::ostream &out, const std::vector<T>& v) {
    out << "[";
    for (auto& e : v) {
        out << e << ",";
    }
    out << "]";
    return out;
}

struct MongoState : public ModelState<MongoState> {
    TermType globalCurrentTerm;

    std::vector<RaftState> states{ALL_NODES, Secondary};

    using Log = std::vector<LogEntry>;
    std::vector<Log> logs{ALL_NODES, Log()};

    friend bool operator==(const MongoState& lhs, const MongoState& rhs) {
        return lhs.globalCurrentTerm == rhs.globalCurrentTerm
            && lhs.states == rhs.states
            && lhs.logs == rhs.logs;
    }

    // TODO: Symmetry.
    template <typename H>
    friend H AbslHashValue(H h, const MongoState& s) {
        return H::combine(std::move(h), s.globalCurrentTerm, s.states, s.logs);
    }

    friend std::ostream& operator << (std::ostream &out, const MongoState& s) {
        return out << "fp: " << s.hash() << " [globalCurrentTerm: " << s.globalCurrentTerm
                   << ", states: " << s.states  << ", logs: " << s.logs << "]";
    }
    bool satisfyInvariant() const;
    void generate();
};

// Define invariant.
bool MongoState::satisfyInvariant() const {
    return logs[N1].size() < 3;
}

// Define the model.
void MongoState::generate() {
    auto AppendOplog = [&](Node receiver, Node sender){
        auto& rlog = logs[receiver];
        auto& slog = logs[sender];
        // Sender's log must be longer.
        if (rlog.size() >= slog.size()) return;
        // Sender has the last entry on receiver.
        if (rlog.empty() || (slog[rlog.size() - 1] == rlog.back())) {
            rlog.push_back(slog[rlog.size()]);
        }
    };
    either([&](){
        for (auto receiver : all_nodes) {
            for (auto sender : all_nodes) {
                either([&]() {
                    AppendOplog(receiver, sender);
                });
            }
        }
    });

    auto ClientWrite = [&]() {
        for (auto n : all_nodes) {
            if (states[n] == Primary) {
                either([&]() {
                    logs[n].push_back({globalCurrentTerm});
                });
            }
        }
    };
    either(ClientWrite);
}

int main(int argv, char** argc) {
    MongoState initialState;
    initialState.globalCurrentTerm = 0;
    initialState.states[N1] = Primary;


    Checker<MongoState>::get()->run({initialState});

    return 0;
}
