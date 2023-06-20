#ifndef STAR_STORED_FUNCTIONS_HPP
#define STAR_STORED_FUNCTIONS_HPP

#include "StarParametricFunction.hpp"
#include "StarJson.hpp"
#include "StarMultiTable.hpp"
#include "StarThread.hpp"

namespace Star {

STAR_CLASS(StoredFunction);
STAR_CLASS(StoredFunction2);
STAR_CLASS(StoredConfigFunction);
STAR_CLASS(FunctionDatabase);

STAR_EXCEPTION(StoredFunctionException, StarException);

enum class Monotonicity { Flat, Increasing, Decreasing, None };

// Interpolated function from single input to single output read from
// configuration.
class StoredFunction {
public:
  struct SearchResult {
    double targetValue;
    double searchTolerance;

    // Whether or not a solution was found within the given tolerance.
    bool found;
    // The resulting input that was found.
    double solution;
    // The value that is gotten from evaluating the function at the solution.
    double value;
  };

  static double const DefaultSearchTolerance;

  StoredFunction(ParametricFunction<double, double> data);

  Monotonicity monotonicity() const;

  double evaluate(double input) const;

  // Search for the input which would give the target value.  Will only work if
  // the function Monotonicity is Increasing or Decreasing.
  SearchResult search(double targetValue, double valueTolerance = DefaultSearchTolerance) const;

private:
  Monotonicity m_monotonicity;
  ParametricFunction<double, double> m_function;
};

// Interpolated function from two inputs to an output read from configuration.
class StoredFunction2 {
public:
  StoredFunction2(MultiTable2D table);

  double evaluate(double x, double y) const;

private:
  MultiTable2D table;
};

// Function from a single input to some generic configuration.
class StoredConfigFunction {
public:
  StoredConfigFunction(ParametricTable<int, Json> data);

  Json get(double input) const;

private:
  ParametricTable<int, Json> m_data;
};

class FunctionDatabase {
public:
  FunctionDatabase();

  StringList namedFunctions() const;
  StringList namedFunctions2() const;
  StringList namedConfigFunctions() const;

  // If configOrName is a string, loads the named function.  If it is an inline
  // config, reads the inline config.
  StoredFunctionPtr function(Json const& configOrName) const;
  StoredFunction2Ptr function2(Json const& configOrName) const;
  StoredConfigFunctionPtr configFunction(Json const& configOrName) const;

private:
  static ParametricFunction<double, double> parametricFunctionFromConfig(Json descriptor);
  static ParametricTable<int, Json> parametricTableFromConfig(Json descriptor);
  static MultiTable2D multiTable2DFromConfig(Json descriptor);

  StringMap<StoredFunctionPtr> m_functions;
  StringMap<StoredFunction2Ptr> m_functions2;
  StringMap<StoredConfigFunctionPtr> m_configFunctions;
};

}

#endif
