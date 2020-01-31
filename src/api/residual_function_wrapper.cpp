#include <iostream>
#include <iomanip>
#include <cstring>
#include <ctime>
#include "mango.hpp"
#include "Least_squares_solver.hpp"

void mango::Least_squares_solver::residual_function_wrapper(const double* x, double* f, bool* failed) {
  // This subroutine is used to call the residual function when N_worker_groups = 1 (i.e. parallelization only within the objective function evaluation),
  // or for algorithms like hopspack that allow concurrent evaluations of the objective function but not using finite difference Jacobians.
  // This subroutine is not used for finite difference Jacobians.

  // I chose to make this subroutine take a parameter f[] to store the residuals rather than always storing them
  // in the "residuals" array of the mango::problem class because PETSc uses its own storage for the residuals.

  // For non-least-squares algorithms, 

  int failed_int;
  residual_function(&(N_parameters), x, &N_terms, f, &failed_int, problem, original_user_data);
  *failed = (failed_int != 0);

  if (verbose > 0) {
    std::cout << "Hello from residual_function_wrapper. Here comes x:" << std::endl;
    int j;
    for (j=0; j < N_parameters; j++) {
      std::cout << std::setw(24) << std::setprecision(15) << x[j];
    }
    std::cout << std::endl;
  }

  record_function_evaluation(x, f, *failed);

}

void mango::Least_squares_solver::record_function_evaluation(const double* x, double *f, bool failed) {
  // This method is NOT an override: note double*f instead of double f, and return void instead of bool.
  if (verbose>0) std::cout << "Hello from Least_squares_solver::record_function_evaluation, the non-override." << std::endl;
  double objective_value = residuals_to_single_objective(f);
  current_residuals = f;
  record_function_evaluation(x, objective_value, failed);
}

bool mango::Least_squares_solver::record_function_evaluation(const double* x, double f, bool failed) {
  // This method overrides mango::Solver::record_function_evaluation()

  if (verbose>0) std::cout << "Hello from Least_squares_solver::record_function_evaluation, the override." << std::endl;
  // Call the overridden function from the base class:
  bool new_optimum = mango::Solver::record_function_evaluation(x,f,failed);
  if (new_optimum) {
    memcpy(best_residual_function, current_residuals, N_terms * sizeof(double));
  }
  
  return new_optimum;
  /*

  // For non-least-squares algorithms, the function evaluations are recorded in objective_function_wrapper.
  // Otherwise, do the recording here.
  bool new_optimum;
  if (algorithms[algorithm].least_squares) {
    double objective_value = residuals_to_single_objective(f);
    current_residuals = f;
    new_optimum = record_function_evaluation(x, objective_value, failed);
    if (new_optimum) {
      memcpy(best_residual_function, f, N_terms * sizeof(double));
    }
  }

  */
}