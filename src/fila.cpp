#include "fila.hpp"
#include <algorithm>

Scheduler::Scheduler(std::vector<Process>& table, int aging_ms)
        : table(table), aging_ms(aging_ms) {}

// Converte as prioridades de usuario (1 a 3) em indices do array (0 a 2)
std::deque<int>& Scheduler::user_queue(int priority){
    return user_queues[priority - HIGH_USER_PRIORITY];
}

bool Scheduler::enqueue_global(int pid){
    // Impede acesso fora dos limites da tabela de processos
    if(pid < 0 || (std::size_t)pid >= table.size()) return false;

    Process& p = table[pid];
    // Impede insercoes duplicadas e reinsercoes de processos finalizados
    if(p.state != State::NEW) return false;
    // Valida os dados necessarios ao escalonamento de um processo novo
    if(p.priority < RT_PRIORITY || p.priority > LOW_USER_PRIORITY) return false;
    if(p.base_priority != p.priority || p.remaining_ms <= 0) return false;

    // O limite e compartilhado entre todas as filas e o processo na CPU
    // Processos ainda nao inseridos ou ja finalizados nao ocupam vagas
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

// A insercao ao final preserva a ordem FIFO dentro de cada prioridade
void Scheduler::push_ready(int pid){
    Process& p = table[pid];
    p.state = State::READY;
    p.wait_ms = 0;

    if(p.is_realtime()) rt_queue.push_back(pid);
    else user_queue(p.priority).push_back(pid);
}

void Scheduler::admit(int pid){
    // Localiza o PID na fila global, preservando a ordem dos demais
    auto it = std::find(global_queue.begin(), global_queue.end(), pid);
    if(it == global_queue.end()) return;
    // Garante que apenas processos aguardando admissao sejam transferidos
    if(table[pid].state != State::WAITING) return;

    global_queue.erase(it);
    push_ready(pid);
}

int Scheduler::pick_next(){
    // Preserva a CPU ocupada; processos de tempo real nao sofrem preempcao
    if(running != NO_PID) return running;

    int pid = NO_PID;
    // Tempo real tem precedencia sobre todas as filas de usuario
    if(!rt_queue.empty()){
        pid = rt_queue.front();
        rt_queue.pop_front();
    }
    else{
        // Seleciona a primeira fila de usuario nao vazia (prioridades 1 a 3)
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
    p.remaining_ms -= 1;            // um tick corresponde a 1 ms de CPU

    // Envelhece apenas quem esperou durante o tick, antes de reenfileirar
    // o processo que acabou de executar
    age_ready();

    if(p.remaining_ms <= 0){
        p.state = State::FINISHED;
        running = NO_PID;
        return true;
    }

    // Ao consumir o quantum sem terminar, o usuario perde a CPU
    // Rebaixa ate a prioridade 3; nesse nivel, retorna ao fim da mesma fila
    if(!p.is_realtime()){
        if(p.priority < LOW_USER_PRIORITY) p.priority++;
        running = NO_PID;
        push_ready(pid);
    }
    return false;
}

void Scheduler::age_ready(){
    if(aging_ms <= 0) return;
    // Percorre primeiro a fila 2 e depois a 3, evitando envelhecer
    // novamente um processo promovido no mesmo tick
    // A fila 1 ja possui a maior prioridade entre os usuarios
    for(int prio = HIGH_USER_PRIORITY+1; prio <= LOW_USER_PRIORITY; prio++){
        std::deque<int>& queue = user_queue(prio);
        std::deque<int> staying;    // processos que permanecem neste nivel

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