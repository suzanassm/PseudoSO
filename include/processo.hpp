#ifndef PSEUDO_SO_PROCESSO_HPP
#define PSEUDO_SO_PROCESSO_HPP

#include <vector>

namespace pseudo_so {
    const int NO_PID = -1;
    const int MAX_PROCESS = 1000;           // capacidade das filas
    const int RT_PRIORITY = 0;              // maior prioridade = menor valor
    const int HIGH_USER_PRIORITY = 1;       // filas de usuario: 1, 2 e 3
    const int LOW_USER_PRIORITY = 3;

    enum class State {NEW, WAITING, READY, RUNNING, FINISHED};

    struct ResourceRequest{};               // bool ou int?

    struct Process{
        // lidos no .txt
        int pid = NO_PID;
        int arrival_ms = 0;
        int priority = RT_PRIORITY;
        int base_priority = RT_PRIORITY;
        int cpu_ms = 0;
        int frames = 0;
        ResourceRequest resources{};

        // controlados pelo escalonador
        State state = State::NEW;
        int remaining_ms = 0;
        int wait_ms = 0;

        bool is_realtime() const {return base_priority == RT_PRIORITY;}
    };

    using ProcessTable = std::vector<Process>;
}

#endif