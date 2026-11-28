#ifndef PSEUDO_SO_ARQUIVOS_HPP
#define PSEUDO_SO_ARQUIVOS_HPP

#include "processo.hpp"
#include <cstddef>
#include <istream>
#include <map>
#include <string>
#include <vector>

// O despachante imprime o cabeçalho Sucesso/Falha e a mensagem retornada.
struct FileOperationResult {
    std::size_t number;
    bool success;
    std::string message;
};

// Simula o disco com alocação contígua. Não modifica a tabela de processos
// nem escreve no terminal; a apresentação dos resultados cabe ao despachante.
class FileSystem {
public:
    // Entrada inválida ou erro de leitura lança std::runtime_error.
    explicit FileSystem(const std::string& input_path);
    explicit FileSystem(std::istream& input);

    // Executa uma única vez, na ordem da entrada. PID corresponde ao índice
    // da tabela, como no escalonador. Processos finalizados continuam válidos.
    // Uma segunda chamada lança std::logic_error para impedir reexecução.
    std::vector<FileOperationResult> execute_all(
        const std::vector<Process>& processes);

    // Cada posição é um bloco; '0' representa um bloco livre.
    const std::vector<char>& disk_map() const;

private:
    struct FileEntry {
        int owner_pid;
        int first_block;
        int block_count;
    };

    struct FileOperation {
        int pid;
        int code;           // 0 = criar; 1 = deletar
        char name;
        int block_count;    // ignorado na exclusão
    };

    void read_input(std::istream& input);
    void read_segment(const std::string& line, std::size_t line_number);
    void read_operation(const std::string& line, std::size_t line_number);
    int find_first_fit(int block_count) const;
    FileOperationResult create_file(const FileOperation& operation,
                                    std::size_t number);
    FileOperationResult delete_file(const FileOperation& operation,
                                    const Process& process,
                                    std::size_t number);

    std::vector<char> disk;
    std::map<char, FileEntry> files;
    std::vector<FileOperation> operations;
    bool executed = false;
};

#endif
