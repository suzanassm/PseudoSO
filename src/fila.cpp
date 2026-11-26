#include "fila.hpp"
#include <algorithm>

Scheduler::Scheduler(std::vector<Process>& table, int aging_ms)
        : table(table), aging_ms(aging_ms) {}

// a fila de usuario de maior prioridade = posicao 0, a seguinte = 1 e assim por diante
std::deque<int>& Scheduler::user_queue(int priority){
    return user_queues[priority - HIGH_USER_PRIORITY];
}

bool Scheduler::enqueue_global(int pid){
    if(global_queue.size() >= MAX_PROCESS) return false;

    table[pid].state = State::WAITING;
    global_queue.push_back(pid);
    return true;
}

const std::deque<int>& Scheduler::global() const{
    return global_queue;
}

// coloca o processo no fim da fila de prontos que corresponde a ele
void Scheduler::push_ready(int pid){
    Process& p = table[pid];
    p.state = State::READY;
    p.wait_ms = 0;

    if(p.is_realtime()) rt_queue.push_back(pid);
    else user_queue(p.priority).push_back(pid);
}

void Scheduler::admit(int pid){
    auto it = std::find(global_queue.begin(), global_queue.end(), pid);
    if(it == global_queue.end()) return;

    global_queue.erase(it);
    push_ready(pid);
}