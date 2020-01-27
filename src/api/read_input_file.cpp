#include <string>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include "mango.hpp"

void mango::Problem::read_input_file(std::string filename) {
  std::ifstream file;

  file.open(filename.c_str());
  if (!file.is_open()) {
    std::cerr << "Error! Unable to open file " << filename << std::endl;
    throw std::runtime_error("Error in mango::Problem::read_input_file. Unable to open file.");
  }

  int N_worker_groups;
  std::string algorithm_str;
  file >> N_worker_groups;
  file >> algorithm_str;
  set_algorithm(algorithm_str);
  file.close();

  mpi_partition.set_N_worker_groups(N_worker_groups);
}
