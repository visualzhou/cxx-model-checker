A Small Model Checker in C++
============================
TLA+ and TLC are great tools, but not perfect for a few reasons.
1. Users have to learn a new language and a new toolbox.
2. TLC's performance isn't transparent to beginners, e.g. execution ordering and the optimization of set logic.
3. TLA+ is mathenmatical and elegant but isn't designed for execution.
4. C++ has a richer community for system programming, low-level performance optimization and distributed execution than Java.

Explict-state model checking seems more of an engineering problem than a mathenmatical problem. This project is an attampt to build a small model checker in C++, so developers can write their models and execute them in a familiar language, just like writing unit tests with unit testing frameworks.

[Cases](https://learntla.com/pluscal/behaviors/) that can generate new states:
* Either
* With

TODO:
* Expose a clear interface to write C++ model.
* Support "Either" and "With" helpers or macros.
* Support set logic.
* Support constraints.
* Remember and print action names in trace.
* Implement a robust hashing function and support symmetry checking.
* Output execution statistics.
* Test on a large real model and measure the single thread performance.
* Adopt a concurrent hash table and a concurrent queue.
* Explore the state space in parallel.

Open Questions:
* How to model temporal formulas in C++ and support liveness properties.
* How to make the syntax easy to model multiple steps like PlusCal.
