#ifndef PSEUDO_SO_PROCESSO_HPP
#define PSEUDO_SO_PROCESSO_HPP

#include <vector>

const int NO_PID = -1;
const int MAX_PROCESS = 1000;           // capacidade das filas
const int RT_PRIORITY = 0;              // maior prioridade = menor valor
const int HIGH_USER_PRIORITY = 1;       // filas de usuario: 1, 2 e 3
const int LOW_USER_PRIORITY = 3;

enum class State {NEW, WAITING, READY, RUNNING, FINISHED, REJECTED};

struct ResourceRequest{
    bool printer;
    bool scanner;
    bool modem;
    bool sata;
};               

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

    // construtor padrao
    Process() = default;

    // construtor com parametro
    Process(int pid, int arrival_ms, int priority, int cpu_ms, int frames, ResourceRequest res = {})
            : pid(pid), 
              arrival_ms(arrival_ms), 
              priority(priority), 
              base_priority(priority), 
              cpu_ms(cpu_ms), 
              frames(frames), 
              resources(res), 
              remaining_ms(cpu_ms) {}  

        
    bool is_realtime() const {
        return base_priority == RT_PRIORITY;
    }
};

#endif