#include <iostream>
#include <cstring>
#include <stdexcept>
#include "mango.hpp"

void mango::problem::optimize_least_squares() {
  int j;
  bool proc0_world = mpi_partition.get_proc0_world();

  // I need to think about this next bit. Do all group leaders need a copy of sigmas and targets?
  // For hopspack, they do, because the residuals are combined into the total objective function on each group leader.
  // But for finite difference Jacobians, only proc0 needs sigmas and targets.
  // If they do, then they must all agree on N_terms; otherwise probably the size of targets and sigmas
  // will be incorrect on non-master procs.
  // Make sure that all procs agree on sigmas and targets.
  MPI_Bcast(targets, N_terms, MPI_DOUBLE, 0, mpi_partition.get_comm_group_leaders());
  MPI_Bcast(sigmas,  N_terms, MPI_DOUBLE, 0, mpi_partition.get_comm_group_leaders());

  if (algorithms[algorithm].uses_derivatives && !proc0_world) {
    group_leaders_least_squares_loop();
    return;
  }

  // proc0_world always continues past this point.
  // For finite-difference-derivative algorithms, the other procs do not go past this point.
  // For parallel algorithms that do not use finite-difference derivatives, such as HOPSPACK, the other group leader procs DO continue past this point.

  if (verbose > 0) std::cout << "Hello world from optimize_least_squares()" << std::endl;
  function_evaluations = 0;

  // Verify that the sigmas array is all nonzero.
  for (j=0; j<N_terms; j++) {
    if (sigmas[j] == 0.0) {
      std::cerr << "Error! The (0-based) entry " << j << " in the sigmas array is 0. sigmas must all be nonzero." << std::endl;
      throw std::runtime_error("Error in mango::problem::optimize_least_squares. sigmas is not all nonzero.");
    }
  }

  if (proc0_world) {
    // Open output file
    output_file.open(output_filename.c_str());
    if (!output_file.is_open()) {
      std::cout << "output file: " << output_filename << std::endl;
      throw std::runtime_error("Error in mango::problem::optimize_least_squares(). Unable to open output file.");
    }
    // Write header line of output file
    output_file << "Least squares?" << std::endl << "yes" << std::endl << "N_parameters:" << std::endl << N_parameters << std::endl << "function_evaluation,seconds";
    for (j=0; j<N_parameters; j++) {
      output_file << ",x(" << j+1 << ")";
    }
    output_file << ",objective_function";
    if (print_residuals_in_output_file) {
      for (j=0; j<N_terms; j++) {
	output_file << ",F(" << j+1 << ")";
      }
    }
    output_file << std::endl << std::flush;
    
    /* Sanity test */
    /* if (!least_squares_algorithm) {
       std::cout << "Error in optimize_least_squares(). Should not get here!\n";
       exit(1);
       }*/
    
    /*  objective_function = (mango::objective_function_type) &mango::problem::least_squares_to_single_objective; */
  } // if (proc0_world)

  objective_function = &mango::problem::least_squares_to_single_objective;

  // Perform the main optimization.
  if (algorithms[algorithm].least_squares) {
    package->optimize_least_squares(this);
  } else {
    // Non-least-squares algorithms
    package->optimize(this);
  }

  if (!proc0_world) return;
  // Only proc0_world continues past this point.

  // Tell the other group leaders to exit.
  int data = -1;
  MPI_Bcast(&data,1,MPI_INT,0,mpi_partition.get_comm_group_leaders());

  memcpy(state_vector, best_state_vector, N_parameters * sizeof(double)); // Make sure we leave state_vector equal to the best state vector seen.

  // Copy the line corresponding to the optimum to the bottom of the output file.
  int function_evaluations_temp= function_evaluations;
  function_evaluations = best_function_evaluation;
  write_least_squares_file_line(best_time, state_vector, best_objective_function, best_residual_function);
  function_evaluations = function_evaluations_temp;

  output_file.close();

  if (verbose > 0) {
    std::cout << "Here comes the optimal state_vector from optimize_least_squares.cpp: " << state_vector[0];
    for (int j=1; j<N_parameters; j++) {
      std::cout << ", " << state_vector[j];
    }
    std::cout << std::endl;
  }
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

void mango::problem::least_squares_to_single_objective(int* N, const double* x, double* f, int* failed_int, mango::problem* this_problem, void* user_data) {
  // Note that this function is static, so "this" does not exist, and hence we must use "this_problem" instead.

  // Note that this subroutine sets the 'residuals' array of the mango::problem class.

  int N_terms = this_problem->get_N_terms();

  if (this_problem->verbose > 0) std::cout << "Hello from least_squares_to_single_objective" << std::endl;
  //double* residuals = new double[N_terms];

  bool failed_bool;
  this_problem->residual_function_wrapper(x, this_problem->residuals, &failed_bool);
  if (failed_bool) {
    *failed_int = 1; 
  } else {
    *failed_int = 0;
  }

  double term;
  *f = 0;
  for (int j=0; j<N_terms; j++) {
    term = (this_problem->residuals[j] - this_problem->targets[j]) / this_problem->sigmas[j];
    *f += term*term;
  }

  //delete[] residuals;
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

double mango::problem::residuals_to_single_objective(double* residuals) {
  // Combine the residuals into the total objective function.
  double total_objective_function, temp;
  int j;

  total_objective_function = 0;
  for (j=0; j<N_terms; j++) {
    temp = (residuals[j] - targets[j]) / sigmas[j];
    total_objective_function += temp*temp;
  }
  return(total_objective_function);
}


