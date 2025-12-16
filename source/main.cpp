#include <cstring>
#include <fstream>
#include <string>
#include <vector>
#include <stdexcept>
#include "core.hpp"

// Загрузка программы из файла в вектор
// filename - имя файла
// output - вектор куда будет загружена программа
void load_program(std::string &filename, std::vector<int> &output) {
  int value;
  std::ifstream f(filename);
  while (f >> value) {
    std::cout << value << std::endl;
    output.push_back(value);
  }
}

int main(int argc, char **argv) {
  // Проверяем количество аргументов
  if (not (argc == 3 or argc == 4)) {
    std::cerr << "Invalid arguments\n";
    return 1;
  }
  std::vector<int> program;
  std::size_t size;
  std::string filename = argv[1];
  // Проверяем аргументыы
  try {
    load_program(filename, program);
    size = std::stoi(argv[2]);
  } catch (std::runtime_error &e) {
    std::cerr << e.what();
    return 2;
  }

  cpu_unit::core cpu0;

  try {
    cpu0.init(program, size);
    bool is_debug = false;
    if (argc == 4) {
      if (std::strcmp(argv[3], "-debug") == 0) {
        is_debug = true;
      }
    }
    cpu0.start_process(is_debug);
  } catch (std::runtime_error &e) {
    std::cerr << e.what();
    return 3;
  }

}
