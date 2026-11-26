#include "fila.hpp"
#include <algorithm>

Scheduler::Scheduler(std::vector<Process>& table, int aging_ms)
        : table(table), aging_ms(aging_ms) {}

// a fila de usuario de maior prioridade = posicao 0, a seguinte = 1 e assim por diante
std::deque<int>& Scheduler::user_queue(int priority){
    return user_queues[priority - HIGH_USER_PRIORITY];
}

bool Scheduler::enqueue_global(int pid){
    // verifica se o PID existe na tabela de processos
    if(pid < 0 || (std::size_t)pid >= table.size()) return false;

    Process& p = table[pid];
    // somente processos novos podem entrar na fila global
    if(p.state != State::NEW) return false;
    // valida a prioridade e o tempo restante de CPU
    if(p.priority < RT_PRIORITY || p.priority > LOW_USER_PRIORITY) return false;
    if(p.base_priority != p.priority || p.remaining_ms <= 0) return false;

    // conta os processos ativos em todas as filas e na CPU
    std::size_t total = global_queue.size() + rt_queue.size();
    for(const std::deque<int>& queue: user_queues) total += queue.size();
    if(running != NO_PID) total++;
    if(total >= MAX_PROCESS) return false;

    // insere o processo na fila global
    p.state = State::WAITING;
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
    // somente processos aguardando admissao podem ser admitidos
    if(table[pid].state != State::WAITING) return;

    global_queue.erase(it);
    push_ready(pid);
}

int Scheduler::pick_next(){
    // tempo real n sofre preempcao: enquanto n terminar, continua rodando
    if(running != NO_PID) return running;

    int pid = NO_PID;

    if(!rt_queue.empty()){
        pid = rt_queue.front();
        rt_queue.pop_front();
    }
    else{
        // prioridade 1 primeiro, depois 2, depois 3
        for(std::deque<int>& queue : user_queues){
            if(queue.empty()) continue;
            pid = queue.front();
            queue.pop_front();
            break;
        }
    }

    if(pid != NO_PID){
        table[pid].state = State::RUNNING;
        running = pid;
    }
    return pid;
}

// executa 1 ms de CPU do processo em execucao, retorna true quando o processo termina
bool Scheduler::tick(){
    if(running == NO_PID) return false;

    int pid = running;
    Process& p = table[pid];
    p.remaining_ms -= 1;

    // envelhece quem esperou neste 1ms (antes de devolver pid pra fila)
    age_ready();

    if(p.remaining_ms <= 0){
        p.state = State::FINISHED;
        running = NO_PID;
        return true;
    }

    // processo de usuario: acabou o quantum, perde a CPU e e rebaixado
    if(!p.is_realtime()){
        if(p.priority < LOW_USER_PRIORITY) p.priority++;
        running = NO_PID;
        push_ready(pid);
    }
    return false;
}

void Scheduler::age_ready(){
    if(aging_ms <= 0) return;
    // da fila 2 pra 3; a fila 1 n envelhece (ja e a mais alta)
    for(int prio = HIGH_USER_PRIORITY+1; prio <= LOW_USER_PRIORITY; prio++){
        std::deque<int>& queue = user_queue(prio);
        std::deque<int> staying;

        for(int pid: queue){
            Process& p = table[pid];
            p.wait_ms++;

            if(p.wait_ms >= aging_ms){
                p.priority--;
                p.wait_ms = 0;
                user_queue(p.priority).push_back(pid);
            }
            else staying.push_back(pid);
        }
        queue.swap(staying);
    }
}

bool Scheduler::has_work() const{
    if(running != NO_PID || !global_queue.empty() || !rt_queue.empty()) return true;

    for(const std::deque<int>& queue: user_queues){
        if(!queue.empty()) return true;
    }
    return false;
}