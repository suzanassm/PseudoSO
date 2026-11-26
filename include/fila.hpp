#ifndef PSEUDO_SO_FILA_HPP
#define PSEUDO_SO_FILA_HPP

#include "processo.hpp"
#include <vector>
#include <deque>
#include <array>

// fila e escalonador

const int AGING_LIMIT_MS = 5;  // provisorio

class Scheduler{
public:
    Scheduler(std::vector<Process>& table, int aging_ms = AGING_LIMIT_MS);

    // coloca na fila global (state = WAITING), false se ela ja ta cheia
    bool enqueue_global(int pid);
    const std::deque<int>& global() const;

    // tira da fila global e coloca na fila de prontos (state = READY)
    void admit(int pid);

    // escolhe quem usa a CPU (state = RUNNING). NO_PID se nao ha ninguem
    int pick_next();

    // executa 1 ms de pid, retorna true se ele terminou
    bool tick();

    // ainda tem processo na fila global, nas filas de prontos ou rodando?
    bool has_work() const;
    
private:
    void push_ready(int pid);
    void age_ready();
    std::deque<int>& user_queue(int priority);

    std::vector<Process>& table_;
    int aging_ms;
    int running = NO_PID;

    std::deque<int> global_queue;
    std::deque<int> rt_queue;
    std::array<std::deque<int>, LOW_USER_PRIORITY> user_queues;
};

#endif
