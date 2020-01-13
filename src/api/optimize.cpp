#include <iostream>
#include <math.h>
#include <limits>
#include <cstring>
#include <stdexcept>
#include <ctime>
#include "mango.hpp"


double mango::problem::optimize() {

  if (! mpi_partition.get_proc0_worker_groups()) 
    throw std::runtime_error("Error! The mango_optimize() subroutine should only be called by group leaders, not by all workers.");

  function_evaluations = 0;
  at_least_one_success = false;
  best_objective_function = std::numeric_limits<double>::quiet_NaN();
  best_function_evaluation = -1;
  start_time = clock();

  // To simplify code a bit...
  MPI_Comm mpi_comm_group_leaders = mpi_partition.get_comm_group_leaders();

  // Make sure that parameters used by the finite-difference gradient routine are the same for all group leaders:
  MPI_Bcast(&N_parameters, 1, MPI_INT, 0, mpi_comm_group_leaders);
  MPI_Bcast(&N_terms, 1, MPI_INT, 0, mpi_comm_group_leaders);
  MPI_Bcast(&centered_differences, 1, MPI_C_BOOL, 0, mpi_comm_group_leaders);
  MPI_Bcast(&finite_difference_step_size, 1, MPI_DOUBLE, 0, mpi_comm_group_leaders);
  MPI_Bcast(&least_squares, 1, MPI_C_BOOL, 0, mpi_comm_group_leaders);
  MPI_Bcast(&algorithm, 1, MPI_INT, 0, mpi_comm_group_leaders);

  if (algorithms[algorithm].requires_bound_constraints && (!bound_constraints_set)) 
    throw std::runtime_error("Error! A MANGO algorithm was chosen that requires bound constraints, but bound constraints were not set.");

  if (bound_constraints_set && (!algorithms[algorithm].allows_bound_constraints) && mpi_partition.get_proc0_world()) {
#define star_line "*******************************************************************************************************"
    std::cerr << star_line << std::endl;
    std::cerr << "WARNING! Bound constraints were set, but an algorithm was chosen that does not allow bound constraints." << std::endl;
    std::cerr << "Therefore, the bound constraints will be ignored for this calculation." << std::endl;
    std::cerr << star_line << std::endl;
  }

  if (algorithms[algorithm].uses_derivatives) {
    if (centered_differences) {
      max_function_and_gradient_evaluations = ceil(max_function_evaluations / 7.0);
    } else {
      max_function_and_gradient_evaluations = ceil(max_function_evaluations / 4.0);
    }
  } else {
    max_function_and_gradient_evaluations = max_function_evaluations;
  }

  set_package();

  if (verbose > 0) {
    std::cout << "Proc " << mpi_partition.get_rank_world() << " is entering optimize(), and thinks proc0_world=" << mpi_partition.get_proc0_world() << std::endl;
    std::cout << "max_function_evaluations = " << max_function_evaluations << 
      ", max_function_and_gradient_evaluations = " << max_function_and_gradient_evaluations << std::endl;
  }

  if (least_squares) {
    optimize_least_squares();
    return(best_objective_function);
  }

  // Beyond this point, the problem must be non-least-squares.
  bool proc0_world = mpi_partition.get_proc0_world();
  if (algorithms[algorithm].uses_derivatives && !proc0_world) {
    group_leaders_loop();
    return(std::numeric_limits<double>::quiet_NaN());
  }

  // proc0_world always continues past this point.
  // For finite-difference-derivative algorithms, the other procs do not go past this point.
  // For parallel algorithms that do not use finite-difference derivatives, such as HOPSPACK, the other group leader procs DO continue past this point.

  if (proc0_world) {
    if (verbose > 0) std::cout << "Hello world from optimize()" << std::endl;

    // Open output file
    output_file.open(output_filename.c_str());
    if (!output_file.is_open()) {
      std::cerr << "output file: " << output_filename << std::endl;
      throw std::runtime_error("Error! Unable to open output file.");
    }
    // Write header line of output file
    output_file << "Least squares?" << std::endl << "no" << std::endl << "N_parameters:" << std::endl << N_parameters << std::endl << "function_evaluation,seconds";
    for (int j=0; j<N_parameters; j++) {
      output_file << ",x(" << j+1 << ")";
    }
    output_file << ",objective_function" << std::endl;
  }

  if (algorithms[algorithm].least_squares)
    throw std::runtime_error("Error! An algorithm for least-squares problems was chosen, but the problem specified is not least-squares.");

  // This next line is where the main optimization happens.
  package->optimize(this);

  if (!proc0_world) return(std::numeric_limits<double>::quiet_NaN());
  // Only proc0_world continues past this point.

  // Tell the other group leaders to exit.
  int data = -1;
  MPI_Bcast(&data,1,MPI_INT,0,mpi_comm_group_leaders);

  memcpy(state_vector, best_state_vector, N_parameters * sizeof(double)); // Make sure we leave state_vector equal to the best state vector seen.

  // Copy the line corresponding to the optimum to the bottom of the output file.
  int function_evaluations_temp = function_evaluations;
  function_evaluations = best_function_evaluation;
  write_file_line(best_time, state_vector, best_objective_function);
  function_evaluations = function_evaluations_temp;

  output_file.close();

  if (verbose > 0) {
    std::cout << "Here comes the optimal state_vector from optimize.cpp: " << state_vector[0];
    for (int j=1; j<N_parameters; j++) {
      std::cout << ", " << state_vector[j];
    }
    std::cout << std::endl;
  }

  return(best_objective_function);
}
