#include "arquivos.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace {

std::runtime_error input_error(std::size_t line_number,
                               const std::string& reason) {
    return std::runtime_error("Entrada de arquivos, linha " +
                              std::to_string(line_number) + ": " + reason);
}

std::string trim(const std::string& text) {
    const std::size_t first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    const std::size_t last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

// Preserva campos vazios, inclusive depois de uma vírgula final, para que
// entradas incompletas não sejam aceitas silenciosamente.
std::vector<std::string> split_fields(const std::string& line) {
    std::vector<std::string> fields;
    std::size_t start = 0;
    while (true) {
        const std::size_t comma = line.find(',', start);
        const std::size_t end = comma == std::string::npos ? line.size() : comma;
        fields.push_back(trim(line.substr(start, end - start)));
        if (comma == std::string::npos) return fields;
        start = comma + 1;
    }
}

int parse_integer(const std::string& field, std::size_t line_number) {
    std::istringstream input(field);
    int value;
    if (!(input >> value)) {
        throw input_error(line_number, "inteiro inválido: '" + field + "'.");
    }
    input >> std::ws;
    if (!input.eof()) {
        throw input_error(line_number, "inteiro inválido: '" + field + "'.");
    }
    return value;
}

char parse_name(const std::string& field, std::size_t line_number) {
    if (field.size() != 1 ||
        !((field[0] >= 'A' && field[0] <= 'Z') ||
          (field[0] >= 'a' && field[0] <= 'z'))) {
        throw input_error(line_number, "o nome do arquivo deve ser uma letra (A-Z ou a-z).");
    }
    return field[0];
}

std::string required_line(std::istream& input, std::size_t& line_number) {
    std::string line;
    if (!std::getline(input, line)) {
        throw input_error(line_number + 1, "linha obrigatória ausente ou ilegível.");
    }
    ++line_number;
    return line;
}

std::string allocated_blocks(int first_block, int block_count) {
    std::ostringstream output;
    output << (block_count == 1 ? " (bloco " : " (blocos ");
    for (int offset = 0; offset < block_count; ++offset) {
        if (offset > 0) output << (offset == block_count - 1 ? " e " : ", ");
        output << first_block + offset;
    }
    output << ").";
    return output.str();
}

} // namespace

FileSystem::FileSystem(const std::string& input_path) {
    std::ifstream input(input_path);
    if (!input) {
        throw std::runtime_error("Não foi possível abrir a entrada de arquivos: " + input_path);
    }
    read_input(input);
}

FileSystem::FileSystem(std::istream& input) {
    read_input(input);
}

void FileSystem::read_input(std::istream& input) {
    std::size_t line_number = 0;
    const std::string size_line = required_line(input, line_number);
    const int disk_size = parse_integer(trim(size_line), line_number);
    if (disk_size <= 0) {
        throw input_error(line_number, "a quantidade de blocos deve ser positiva.");
    }

    const std::string segments_line = required_line(input, line_number);
    const int segment_count = parse_integer(trim(segments_line), line_number);
    if (segment_count < 0 || segment_count > disk_size) {
        throw input_error(line_number, "quantidade de segmentos inválida.");
    }
    disk.assign(static_cast<std::size_t>(disk_size), '0');

    for (int segment = 0; segment < segment_count; ++segment) {
        const std::string line = required_line(input, line_number);
        read_segment(line, line_number);
    }

    std::string line;
    while (std::getline(input, line)) {
        ++line_number;
        if (!trim(line).empty()) read_operation(line, line_number);
    }
    if (input.bad() || !input.eof()) {
        throw input_error(line_number + 1, "falha durante a leitura das operações.");
    }
}

void FileSystem::read_segment(const std::string& line, std::size_t line_number) {
    const std::vector<std::string> fields = split_fields(line);
    if (fields.size() != 3) {
        throw input_error(line_number, "segmento exige nome, primeiro bloco e quantidade.");
    }
    const char name = parse_name(fields[0], line_number);
    const int first_block = parse_integer(fields[1], line_number);
    const int block_count = parse_integer(fields[2], line_number);
    const int disk_size = static_cast<int>(disk.size());

    // A subtração evita estouro ao validar primeiro bloco + quantidade.
    if (first_block < 0 || first_block >= disk_size || block_count <= 0 ||
        block_count > disk_size - first_block) {
        throw input_error(line_number, "segmento fora dos limites do disco.");
    }
    if (files.count(name) != 0) {
        throw input_error(line_number, "arquivo inicial com nome repetido.");
    }
    const auto begin = disk.begin() + first_block;
    const auto end = begin + block_count;
    if (std::find_if(begin, end, [](char block) { return block != '0'; }) != end) {
        throw input_error(line_number, "segmentos iniciais sobrepostos.");
    }

    // Não há criador informado para arquivos iniciais: somente tempo real
    // poderá apagá-los, pois nenhum usuário é seu proprietário.
    files.emplace(name, FileEntry{NO_PID, first_block, block_count});
    std::fill(begin, end, name);
}

void FileSystem::read_operation(const std::string& line, std::size_t line_number) {
    const std::vector<std::string> fields = split_fields(line);
    if (fields.size() < 3) {
        throw input_error(line_number, "operação exige PID, código e nome do arquivo.");
    }
    const int pid = parse_integer(fields[0], line_number);
    const int code = parse_integer(fields[1], line_number);
    if (code != 0 && code != 1) {
        throw input_error(line_number, "código deve ser 0 (criar) ou 1 (deletar).");
    }
    const std::size_t expected_fields = code == 0 ? 4 : 3;
    if (fields.size() != expected_fields) {
        throw input_error(line_number, "quantidade de campos incompatível com a operação.");
    }
    const char name = parse_name(fields[2], line_number);
    const int block_count = code == 0 ? parse_integer(fields[3], line_number) : 0;
    operations.push_back(FileOperation{pid, code, name, block_count});
}

int FileSystem::find_first_fit(int block_count) const {
    int consecutive_free = 0;
    const int disk_size = static_cast<int>(disk.size());
    for (int block = 0; block < disk_size; ++block) {
        consecutive_free = disk[block] == '0' ? consecutive_free + 1 : 0;
        if (consecutive_free == block_count) return block - block_count + 1;
    }
    return -1;
}

FileOperationResult FileSystem::create_file(const FileOperation& operation,
                                           std::size_t number) {
    const std::string prefix = "O processo " + std::to_string(operation.pid) +
                              " não pode criar o arquivo " + operation.name;
    if (operation.block_count <= 0) {
        return {number, false, prefix + " (o tamanho deve ser positivo)."};
    }
    if (files.count(operation.name) != 0) {
        return {number, false, prefix + " (o arquivo já existe)."};
    }
    const int first_block = find_first_fit(operation.block_count);
    if (first_block == -1) {
        return {number, false, prefix + " (falta de espaço contíguo)."};
    }

    files.emplace(operation.name,
                  FileEntry{operation.pid, first_block, operation.block_count});
    std::fill(disk.begin() + first_block,
              disk.begin() + first_block + operation.block_count, operation.name);
    return {number, true, "O processo " + std::to_string(operation.pid) +
                         " criou o arquivo " + operation.name +
                         allocated_blocks(first_block, operation.block_count)};
}

FileOperationResult FileSystem::delete_file(const FileOperation& operation,
                                           const Process& process,
                                           std::size_t number) {
    const auto file = files.find(operation.name);
    const std::string prefix = "O processo " + std::to_string(operation.pid) +
                              " não pode deletar o arquivo " + operation.name;
    if (file == files.end()) {
        return {number, false, prefix + " porque ele não existe."};
    }
    if (!process.is_realtime() && file->second.owner_pid != process.pid) {
        return {number, false, prefix + " (permissão negada: o arquivo pertence a outro proprietário)."};
    }

    const FileEntry entry = file->second;
    std::fill(disk.begin() + entry.first_block,
              disk.begin() + entry.first_block + entry.block_count, '0');
    files.erase(file);
    return {number, true, "O processo " + std::to_string(operation.pid) +
                         " deletou o arquivo " + operation.name + "."};
}

std::vector<FileOperationResult> FileSystem::execute_all(
    const std::vector<Process>& processes) {
    if (executed) {
        throw std::logic_error("As operações de arquivos já foram executadas.");
    }
    executed = true;
    std::vector<FileOperationResult> results;
    results.reserve(operations.size());

    for (std::size_t index = 0; index < operations.size(); ++index) {
        const FileOperation& operation = operations[index];
        const std::size_t number = index + 1;
        if (operation.pid < 0 ||
            static_cast<std::size_t>(operation.pid) >= processes.size() ||
            processes[operation.pid].pid != operation.pid) {
            results.push_back({number, false, "O processo " +
                               std::to_string(operation.pid) + " não existe."});
        } else if (operation.code == 0) {
            results.push_back(create_file(operation, number));
        } else {
            results.push_back(delete_file(operation, processes[operation.pid], number));
        }
    }
    return results;
}

const std::vector<char>& FileSystem::disk_map() const {
    return disk;
}
