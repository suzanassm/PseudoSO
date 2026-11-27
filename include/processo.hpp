#ifndef PSEUDO_SO_PROCESSO_HPP
#define PSEUDO_SO_PROCESSO_HPP

#include <vector>

const int NO_PID = -1;                  // quando nao ha um processo associado
const int MAX_PROCESS = 1000;           // limite de processos ativos
const int RT_PRIORITY = 0;              // prioridade exclusiva de tempo real
const int HIGH_USER_PRIORITY = 1;       // maior prioridade de usuario
const int LOW_USER_PRIORITY = 3;        // menor prioridade de usuario

// Estados do ciclo de vida de um processo simulado.
// NEW: ainda nao inserido; WAITING: aguarda admissao; READY: apto a executar;
// RUNNING: ocupa a CPU; FINISHED: concluiu; REJECTED: rejeitado pelo sistema
enum class State {NEW, WAITING, READY, RUNNING, FINISHED, REJECTED};

// Indica quais categorias de dispositivos de E/S o processo solicita
struct ResourceRequest{
    bool printer;
    bool scanner;
    bool modem;
    bool sata;
};               

// PCB (Process Control Block): concentra os dados de entrada e o estado
// de execucao de cada processo simulado
struct Process{
    // Identificacao e dados provenientes do arquivo de processos
    int pid = NO_PID;                   // identificador do processo
    int arrival_ms = 0;                 // instante de chegada em ms
    int priority = RT_PRIORITY;         // prioridade dinamica
    int base_priority = RT_PRIORITY;    // prioridade original
    int cpu_ms = 0;                     // tempo total de CPU em ms
    int frames = 0;                     // limite do conjunto de trabalho
    ResourceRequest resources{};

    // Informacoes atualizadas durante o escalonamento
    State state = State::NEW;
    int remaining_ms = 0;               // tempo restante de CPU
    int wait_ms = 0;                    // espera acumulada para aging

    // Cria um PCB com os valores iniciais padrao
    Process() = default;

    // Inicializa o PCB com os dados de entrada, preservando a prioridade
    // original e definindo o tempo restante como o tempo total de CPU
    Process(int pid, int arrival_ms, int priority, int cpu_ms, int frames, ResourceRequest res = {})
            : pid(pid), 
              arrival_ms(arrival_ms), 
              priority(priority), 
              base_priority(priority), 
              cpu_ms(cpu_ms), 
              frames(frames), 
              resources(res), 
              remaining_ms(cpu_ms) {}  

    // Classifica o processo pela prioridade original, nao pela dinamica    
    bool is_realtime() const {
        return base_priority == RT_PRIORITY;
    }
};

#endif