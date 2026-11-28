#include "arquivos.hpp"
#include "fila.hpp"
#include <initializer_list>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

std::string disk_contents(const FileSystem& system) {
    return std::string(system.disk_map().begin(), system.disk_map().end());
}

void check_results(const std::vector<FileOperationResult>& results,
                   std::initializer_list<bool> expected) {
    require(results.size() == expected.size(), "Quantidade incorreta de resultados.");
    std::size_t index = 0;
    for (bool success : expected) {
        require(results[index].number == index + 1, "Numeração fora da ordem de entrada.");
        require(results[index].success == success,
                "Resultado inesperado na operação " + std::to_string(index + 1));
        require(!results[index].message.empty(), "Resultado sem explicação.");
        ++index;
    }
}

std::vector<Process> realtime_processes() {
    return {Process(0, 2, 0, 3, 4), Process(1, 8, 0, 2, 8)};
}

void check_input_error(const std::string& text, std::size_t expected_line) {
    std::istringstream input(text);
    bool rejected = false;
    try {
        FileSystem system(input);
    } catch (const std::runtime_error& error) {
        rejected = true;
        require(std::string(error.what()).find(
                    "linha " + std::to_string(expected_line) + ":") != std::string::npos,
                "O erro não identifica a linha correta: " + std::string(error.what()));
    }
    require(rejected, "Entrada inválida foi aceita: " + text);
}

void test_initial_disk() {
    FileSystem system("entradas/file.txt");
    require(disk_contents(system) == "XX0Y0ZZZ00", "Disco inicial incorreto.");
}

void test_specification_example() {
    FileSystem system("entradas/file.txt");
    const auto results = system.execute_all(realtime_processes());
    check_results(results, {false, true, false, true, true});
    require(disk_contents(system) == "DDDY0ZZZEE", "Mapa final do exemplo incorreto.");
    require(results[2].message == "O processo 2 não existe.", "PID inexistente sem motivo correto.");
    require(results[3].message.find("blocos 0, 1 e 2") != std::string::npos,
            "Criação não descreveu os blocos alocados.");
    require(results[4].message.find("blocos 8 e 9") != std::string::npos,
            "O código 0 deve criar E, apesar da inconsistência no texto do exemplo.");
}

void test_first_fit_earliest_region() {
    std::istringstream input("10\n1\nX,3,2\n0,0,A,2\n0,0,B,1\n");
    FileSystem system(input);
    check_results(system.execute_all(realtime_processes()), {true, true});
    require(disk_contents(system) == "AABXX00000", "First-fit ignorou a primeira região livre.");
}

void test_first_fit_restarts_after_delete() {
    std::istringstream input(
        "8\n1\nX,2,2\n0,0,A,2\n0,0,B,3\n0,1,A\n0,0,C,2\n");
    FileSystem system(input);
    check_results(system.execute_all(realtime_processes()), {true, true, true, true});
    require(disk_contents(system) == "CCXXBBB0", "A busca não reiniciou no bloco zero.");
}

void test_fragmentation() {
    std::istringstream input(
        "7\n3\nX,1,1\nY,3,1\nZ,5,1\n0,0,A,2\n0,1,Y\n0,0,B,3\n");
    FileSystem system(input);
    const auto results = system.execute_all(realtime_processes());
    check_results(results, {false, true, true});
    require(results[0].message.find("espaço contíguo") != std::string::npos,
            "Falta de contiguidade não foi explicada.");
    require(disk_contents(system) == "0XBBBZ0", "Disco incorreto após unir espaços livres.");
}

void test_permissions() {
    const std::vector<Process> processes = {
        Process(0, 0, 1, 1, 1), Process(1, 0, 2, 1, 1), Process(2, 0, 0, 1, 1)};
    std::istringstream input(
        "8\n1\nX,6,2\n0,0,A,2\n1,1,A\n1,1,X\n0,1,A\n1,0,B,2\n2,1,B\n2,1,X\n");
    FileSystem system(input);
    const auto results = system.execute_all(processes);
    check_results(results, {true, false, false, true, true, true, true});
    require(results[1].message.find("permissão negada") != std::string::npos,
            "Exclusão de arquivo de outro usuário não informou a permissão.");
    require(disk_contents(system) == "00000000", "Exclusões não liberaram todos os blocos.");
}

void test_ownership_after_recreation() {
    const std::vector<Process> processes = {
        Process(0, 0, 1, 1, 1), Process(1, 0, 1, 1, 1)};
    std::istringstream input(
        "4\n0\n0,0,A,2\n0,1,A\n1,0,A,2\n0,1,A\n1,1,A\n");
    FileSystem system(input);
    check_results(system.execute_all(processes), {true, true, true, false, true});
    require(disk_contents(system) == "0000", "O proprietário não foi atualizado na recriação.");
}

void test_failures_preserve_disk() {
    const std::vector<Process> processes = {Process(0, 0, 1, 1, 1)};
    std::istringstream input(
        "4\n1\nX,1,2\n0,1,X\n0,1,Q\n0,0,X,1\n0,0,A,2\n9,1,X\n0,0,B,0\n");
    FileSystem system(input);
    const std::string before = disk_contents(system);
    check_results(system.execute_all(processes), {false, false, false, false, false, false});
    require(disk_contents(system) == before, "Uma operação que falhou alterou o disco.");
}

void test_duplicate_creation_and_missing_file() {
    std::istringstream input(
        "4\n1\nX,2,1\n0,0,X,1\n0,0,A,1\n0,0,A,2\n0,1,Q\n");
    FileSystem system(input);
    const auto results = system.execute_all(realtime_processes());
    check_results(results, {false, true, false, false});
    require(results[0].message.find("já existe") != std::string::npos,
            "Criação duplicada não informou a causa.");
    require(results[3].message.find("não existe") != std::string::npos,
            "Arquivo inexistente não informou a causa.");
    require(disk_contents(system) == "A0X0", "Falhas alteraram um arquivo já existente.");
}

void test_invalid_pids() {
    const std::vector<Process> processes = {
        Process(9, 0, 0, 1, 1), Process(1, 0, 0, 1, 1)};
    std::istringstream input(
        "3\n0\n0,0,A,1\n9,1,X\n-1,0,C,1\n1,0,B,1\n");
    FileSystem system(input);
    check_results(system.execute_all(processes), {false, false, false, true});
    require(disk_contents(system) == "B00", "PID inválido alterou o disco.");
}

void test_empty_process_table() {
    std::istringstream input("2\n0\n0,0,A,1\n");
    FileSystem system(input);
    check_results(system.execute_all({}), {false});
    require(disk_contents(system) == "00", "Uma tabela vazia autorizou uma operação.");
}

void test_sizes_and_full_disk() {
    std::istringstream input(
        "4\n0\n0,0,A,0\n0,0,B,-2\n0,0,C,5\n0,0,D,4\n0,0,E,1\n0,1,D\n0,0,F,4\n");
    FileSystem system(input);
    check_results(system.execute_all(realtime_processes()),
                  {false, false, false, true, false, true, true});
    require(disk_contents(system) == "FFFF", "Erro ao preencher ou reutilizar o disco inteiro.");
}

void test_scheduler_integration_and_persistence() {
    std::vector<Process> processes = {
        Process(0, 0, 1, 3, 1), Process(1, 0, 2, 2, 1), Process(2, 0, 0, 1, 1)};
    Scheduler scheduler(processes);
    for (std::size_t pid = 0; pid < processes.size(); ++pid) {
        require(scheduler.enqueue_global(static_cast<int>(pid)), "Admissão de teste falhou.");
        scheduler.admit(static_cast<int>(pid));
    }
    while (scheduler.has_work()) {
        require(scheduler.pick_next() != NO_PID, "Escalonador não selecionou processo.");
        scheduler.tick();
    }
    for (const Process& process : processes) {
        require(process.state == State::FINISHED, "Processo não finalizou.");
    }

    std::istringstream input(
        "5\n1\nX,4,1\n0,0,A,1\n1,1,A\n2,1,X\n1,0,B,1\n");
    FileSystem system(input);
    check_results(system.execute_all(processes), {true, false, true, true});
    require(disk_contents(system) == "AB000", "Arquivos não persistiram após o término.");
    require(processes[0].priority == 3 && processes[1].priority == 3,
            "O módulo alterou as prioridades definidas pelo escalonador.");
    for (const Process& process : processes) {
        require(process.state == State::FINISHED && process.remaining_ms == 0,
                "O módulo alterou a execução dos processos.");
    }
}

void test_file_persists_after_process_finishes() {
    std::vector<Process> processes = {Process(0, 0, 1, 2, 1)};
    Scheduler scheduler(processes);
    require(scheduler.enqueue_global(0), "Admissão do processo criador falhou.");
    scheduler.admit(0);
    require(scheduler.pick_next() == 0, "O processo criador não recebeu a CPU.");
    require(processes[0].state == State::RUNNING, "O criador deve estar em execução.");

    std::istringstream input("6\n1\nX,5,1\n0,0,A,3\n");
    FileSystem system(input);
    check_results(system.execute_all(processes), {true});
    const std::string before_completion = disk_contents(system);
    require(before_completion == "AAA00X", "Arquivo não foi criado nos blocos esperados.");

    // A foi criado enquanto P0 executava; o primeiro quantum ainda não o encerra.
    require(!scheduler.tick(), "O processo terminou antes de consumir os dois quanta.");
    require(processes[0].state == State::READY && processes[0].remaining_ms == 1,
            "O criador deveria aguardar seu último quantum.");
    require(disk_contents(system) == before_completion,
            "O arquivo desapareceu antes do término do processo.");

    require(scheduler.pick_next() == 0, "O criador não recebeu seu último quantum.");
    require(scheduler.tick(), "O processo não terminou após consumir toda a CPU.");
    require(processes[0].state == State::FINISHED && processes[0].remaining_ms == 0,
            "O processo criador não foi marcado como finalizado.");
    require(!scheduler.has_work(), "O escalonador ainda possui trabalho pendente.");

    // Consulta o mesmo disco, sem recriar o sistema nem repetir as operações.
    require(disk_contents(system) == before_completion,
            "O término do processo removeu ou alterou os arquivos do disco.");
}

void test_original_priority_controls_permissions() {
    std::vector<Process> processes = {
        Process(0, 0, 1, 1, 1), Process(1, 0, 0, 1, 1)};
    // Mesmo com valores dinâmicos diferentes, o tipo vem da prioridade original.
    processes[0].priority = 0;
    processes[1].priority = 3;
    std::istringstream input("2\n1\nX,0,2\n0,1,X\n1,1,X\n");
    FileSystem system(input);
    check_results(system.execute_all(processes), {false, true});
    require(disk_contents(system) == "00", "Permissões usaram a prioridade dinâmica.");
}

void test_single_execution() {
    std::istringstream input("2\n0\n0,0,A,1\n");
    FileSystem system(input);
    check_results(system.execute_all(realtime_processes()), {true});
    bool rejected = false;
    try {
        system.execute_all(realtime_processes());
    } catch (const std::logic_error&) {
        rejected = true;
    }
    require(rejected, "O lote foi executado pela segunda vez.");
    require(disk_contents(system) == "A0", "A segunda chamada alterou o disco.");
}

void test_no_operations() {
    std::istringstream input("3\n1\nX,2,1\n\n");
    FileSystem system(input);
    require(system.execute_all({}).empty(), "Um arquivo sem operações gerou resultados.");
    require(disk_contents(system) == "00X", "Um lote vazio alterou os arquivos iniciais.");
}

void test_whitespace_and_crlf() {
    std::istringstream input(
        "\t5 \r\n 1\r\n X , 2 , 1 \r\n\r\n 0 , 0 , A , 2 \r\n \t\r\n 0 , 1 , A \r\n");
    FileSystem system(input);
    check_results(system.execute_all(realtime_processes()), {true, true});
    require(disk_contents(system) == "00X00", "Espaços ou CRLF prejudicaram a leitura.");
}

void test_case_sensitive_names_without_final_newline() {
    std::istringstream input("4\n0\n0,0,A,1\n0,0,a,1\n0,1,A");
    FileSystem system(input);
    check_results(system.execute_all(realtime_processes()), {true, true, true});
    require(disk_contents(system) == "0a00",
            "Nomes maiúsculos e minúsculos devem identificar arquivos diferentes.");
}

void test_malformed_input() {
    struct InvalidInput {
        const char* text;
        std::size_t line;
    };
    const InvalidInput inputs[] = {
        {"", 1}, {"0\n0\n", 1}, {"-1\n0\n", 1}, {"4x\n0\n", 1},
        {"999999999999999999999999\n0\n", 1}, {"4,2\n0\n", 1},
        {"4\n", 2}, {"4\n-1\n", 2}, {"4\n5\n", 2}, {"4\n\n", 2},
        {"4\n1\n", 3}, {"4\n1\nX,-1,1\n", 3}, {"4\n1\nX,4,1\n", 3},
        {"4\n1\nX,3,2\n", 3}, {"4\n1\nX,0,0\n", 3},
        {"4\n1\nX,0,2147483647\n", 3}, {"4\n1\nX,0,-2\n", 3},
        {"4\n1\nX,0,1,\n", 3}, {"4\n1\n0,0,1\n", 3},
        {"4\n1\nAB,0,1\n", 3}, {"4\n1\n,0,1\n", 3},
        {"4\n2\nX,0,2\nY,1,2\n", 4}, {"4\n2\nX,0,1\nX,2,1\n", 4},
        {"4\n0\n0,2,A\n", 3}, {"4\n0\n0,0,A\n", 3},
        {"4\n0\n0,1,A,1\n", 3}, {"4\n0\n0,1,A,\n", 3},
        {"4\n0\n0,0,A,\n", 3}, {"4\n0\n0,0,A,1,\n", 3},
        {"4\n0\n0,1\n", 3}, {"4\n0\nq,0,A,1\n", 3},
        {"4\n0\n0,0,A,1x\n", 3}, {"4\n0\n0,0,0,1\n", 3},
        {"4\n0\n0,0,A,1 2\n", 3}, {"4\n0\n\n0,0,A,\n", 4}
    };
    for (const InvalidInput& input : inputs) check_input_error(input.text, input.line);
}

void test_input_io_errors() {
    bool rejected = false;
    try {
        FileSystem system("entradas/__arquivo_inexistente_para_teste__.txt");
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    require(rejected, "Arquivo de entrada inexistente foi aceito.");

    std::istringstream input("4\n0\n");
    input.setstate(std::ios::badbit);
    rejected = false;
    try {
        FileSystem system(input);
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    require(rejected, "Erro de leitura não foi reportado.");
}

} // namespace

int main() {
    struct TestCase {
        const char* name;
        void (*run)();
    };
    const TestCase tests[] = {
        {"disco inicial", test_initial_disk},
        {"exemplo da especificação", test_specification_example},
        {"primeira região disponível", test_first_fit_earliest_region},
        {"busca reinicia no bloco zero", test_first_fit_restarts_after_delete},
        {"fragmentação externa", test_fragmentation},
        {"permissões de usuário e tempo real", test_permissions},
        {"propriedade após recriação", test_ownership_after_recreation},
        {"falhas preservam o disco", test_failures_preserve_disk},
        {"arquivo duplicado e arquivo inexistente", test_duplicate_creation_and_missing_file},
        {"PIDs inválidos", test_invalid_pids},
        {"tabela de processos vazia", test_empty_process_table},
        {"tamanhos inválidos e disco cheio", test_sizes_and_full_disk},
        {"integração com escalonador e persistência", test_scheduler_integration_and_persistence},
        {"arquivo persiste após término do processo", test_file_persists_after_process_finishes},
        {"permissões pela prioridade original", test_original_priority_controls_permissions},
        {"execução única", test_single_execution},
        {"entrada sem operações", test_no_operations},
        {"espaços e quebras de linha CRLF", test_whitespace_and_crlf},
        {"nomes distintos e última linha sem quebra", test_case_sensitive_names_without_final_newline},
        {"entradas malformadas", test_malformed_input},
        {"erros de abertura e leitura", test_input_io_errors}
    };

    int failures = 0;
    for (const TestCase& test : tests) {
        try {
            test.run();
            std::cout << "[OK] " << test.name << '\n';
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "[FALHA] " << test.name << ": " << error.what() << '\n';
        }
    }
    const std::size_t total = sizeof(tests) / sizeof(tests[0]);
    std::cout << total - failures << '/' << total << " testes aprovados.\n";
    return failures == 0 ? 0 : 1;
}
