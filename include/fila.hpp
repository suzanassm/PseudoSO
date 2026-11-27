#ifndef PSEUDO_SO_FILA_HPP
#define PSEUDO_SO_FILA_HPP

#include "processo.hpp"
#include <vector>
#include <deque>
#include <array>

// fila e escalonador

// Intervalo de espera, em ms, para promover um processo de usuario
const int AGING_LIMIT_MS = 5; 

// Gerencia a admissao, as filas de prontos e o compartilhamento da CPU
// Opera sobre uma tabela externa de processos, que deve permanecer valida
// O despachante controla chegadas, relogio, memoria e recursos de E/S
class Scheduler{
public:
    // Associa a tabela de processos e configura o intervalo de aging
    // Um intervalo nao positivo desabilita o aging
    Scheduler(std::vector<Process>& table, int aging_ms = AGING_LIMIT_MS);

    // Insere um processo NEW na fila global e altera seu estado para WAITING
    // Retorna false se o PID, estado, prioridade ou tempo restante for invalido,
    // ou se o limite de processos ativos tiver sido atingido
    bool enqueue_global(int pid);
    // Disponibiliza a fila global para consulta, sem permitir sua alteracao
    const std::deque<int>& global() const;

    // Transfere um processo WAITING para a fila de prontos correspondente
    // O despachante deve verificar memoria e E/S antes da chamada
    // Nao faz nada se o PID nao estiver na fila global ou no estado WAITING
    void admit(int pid);

    // Mantem o processo em execucao ou seleciona o proximo por prioridade
    // Retorna seu PID, ou NO_PID se nao houver processo pronto
    int pick_next();

    // Simula 1 ms de CPU, aplica aging e trata termino ou preempcao
    // Retorna true apenas quando o processo em execucao termina
    bool tick();

    // Indica se ha processos nas filas ou ocupando a CPU
    // Nao considera processos NEW que ainda nao chegaram
    bool has_work() const;
    
private:
    // Insere um PID valido no fim da fila de prontos correspondente
    // Atualiza seu estado para READY e reinicia o contador de espera
    void push_ready(int pid);
    // Atualiza a espera nas filas 2 e 3 e promove quem atingiu o limite
    // Chamado a cada tick de CPU, antes de reenfileirar o processo executado
    void age_ready();
    // Acessa a fila de usuario correspondente a uma prioridade valida (1 a 3)
    std::deque<int>& user_queue(int priority);

    // Os PIDs devem corresponder aos indices da tabela externa
    std::vector<Process>& table;
    int aging_ms;
    int running = NO_PID;   // PID que ocupa a CPU

    std::deque<int> global_queue;   // processos aguardando admissao
    std::deque<int> rt_queue;       // fila FIFO de tempo real
    // Filas de usuario em ordem decrescente de prioridade (1, 2 e 3)
    std::array<std::deque<int>, LOW_USER_PRIORITY> user_queues;
};

#endif
