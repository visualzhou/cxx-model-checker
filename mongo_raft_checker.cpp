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
using Log = std::vector<LogEntry>;
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
        return out << " [globalCurrentTerm: " << s.globalCurrentTerm
                   << ", states: " << s.states  << ", logs: " << s.logs << "]";
    }
    bool satisfyInvariant() const;
    bool satisfyConstraint() const;
    void generate();
};

// Define invariant.
bool MongoState::satisfyInvariant() const {
    return true;
}

bool MongoState::satisfyConstraint() const {
    if (globalCurrentTerm > 3) return false;
    return std::all_of(logs.begin(), logs.end(), [&](const Log& log){
        return log.size() < 3;
    });
}

bool CanRollbackOplog(const Log& rlog, const Log& slog) {
    if (rlog.empty() || slog.empty()) return false;
    return rlog.back() < slog.back();
}

bool NotBehind(const Log& me, const Log& syncSource) {
    if (syncSource.empty()) return true;
    if (me.empty()) return false;
    return (me.back() > syncSource.back())
        || (me.back() == syncSource.back() && me.size() >= syncSource.size());
}

// Define the model.
void MongoState::generate() {
    // AppendOplog
    auto AppendOplog = [&](Node receiver, Node sender){
        auto& rlog = logs[receiver];
        auto& slog = logs[sender];
        // Sender's log must be longer.
        if (rlog.size() >= slog.size()) return;
        // Sender has the last entry on receiver.
        if (rlog.empty() || (slog[rlog.size() - 1] == rlog.back())) {
            either([&]() {
                rlog.push_back(slog[rlog.size()]);
            });
        }
    };
    // Rollback
    auto RollbackOplog = [&](Node receiver, Node sender) {
        if (!CanRollbackOplog(logs[receiver], logs[sender])) return;
        either([&](){
            logs[receiver].pop_back();
        });
    };

    for (auto receiver : all_nodes) {
        for (auto sender : all_nodes) {
            AppendOplog(receiver, sender);
            RollbackOplog(receiver, sender);
        }
    }

    auto BecomePrimaryByMagic = [&](Node p) {
        auto notBehindCount = std::count_if(logs.begin(), logs.end(),
                [&](const Log& log) {
            return NotBehind(logs[p], log);
        });
        if (notBehindCount * 2 > ALL_NODES) {
            either([&](){
                // Step down all nodes.
                for (auto& s : states) {
                    s = Secondary;
                }
                states[p] = Primary;
                globalCurrentTerm++;
            });
        }
    };

    // ClientWrite
    for (auto n : all_nodes) {
        BecomePrimaryByMagic(n);
        if (states[n] == Primary) {
            either([&]() {
                logs[n].push_back({globalCurrentTerm});
            });
        }
    }
}

int main(int argv, char** argc) {
    MongoState initialState;
    initialState.globalCurrentTerm = 0;
    initialState.states[N1] = Primary;

    Checker<MongoState>::get()->run({initialState});

    return 0;
}
