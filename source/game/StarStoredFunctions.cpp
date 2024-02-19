#include "StarStoredFunctions.hpp"
#include "StarAssets.hpp"
#include "StarRoot.hpp"

namespace Star {

double const StoredFunction::DefaultSearchTolerance = 0.001;

StoredFunction::StoredFunction(ParametricFunction<double, double> data) {
  if (data.empty())
    throw StoredFunctionException("StoredFunction constructor called on function with no data points");

  m_function = std::move(data);

  // Determine whether the function is monotonically increasing, monotonically
  // decreasing, totally flat (technically both monotonically increasing and
  // decreasing) or neither.
  m_monotonicity = Monotonicity::Flat;
  for (size_t i = 0; i < m_function.size() - 1; ++i) {
    if (m_function.value(i) < m_function.value(i + 1)) {
      if (m_monotonicity == Monotonicity::Flat)
        m_monotonicity = Monotonicity::Increasing;
      else if (m_monotonicity == Monotonicity::Decreasing)
        m_monotonicity = Monotonicity::None;
    } else if (m_function.value(i + 1) < m_function.value(i)) {
      if (m_monotonicity == Monotonicity::Flat)
        m_monotonicity = Monotonicity::Decreasing;
      else if (m_monotonicity == Monotonicity::Increasing)
        m_monotonicity = Monotonicity::None;
    }
    if (m_monotonicity == Monotonicity::None)
      break;
  }
}

Monotonicity StoredFunction::monotonicity() const {
  return m_monotonicity;
}

double StoredFunction::evaluate(double value) const {
  return m_function.interpolate(value);
}

StoredFunction::SearchResult StoredFunction::search(double targetValue, double valueTolerance) const {
  double minIndex = m_function.index(0);
  double minValue = m_function.value(0);

  double maxIndex = m_function.index(m_function.size() - 1);
  double maxValue = m_function.value(m_function.size() - 1);

  if (maxValue < minValue) {
    std::swap(minIndex, maxIndex);
    std::swap(minValue, maxValue);
  }

  double index;
  double value;

  if (targetValue < minValue) {
    index = minIndex;
    value = minValue;
  } else if (targetValue > maxValue) {
    index = maxIndex;
    value = maxValue;
  } else {
    index = (minIndex + maxIndex) / 2;
    value = m_function.interpolate(index);

    int searchDepth = 0;

    while ((std::fabs(targetValue - value) > valueTolerance) && (searchDepth < 64)) {
      searchDepth++;
      if (value < targetValue) {
        minIndex = index;
        minValue = value;
      } else if (value > targetValue) {
        maxIndex = index;
        maxValue = value;
      }

      double newIndex = (minIndex + maxIndex) / 2;
      double newValue = m_function.interpolate(newIndex);

      // If at any point we move outside of the established upper and lower
      // bound
      // the function is not monotonic increasing or decreasing, and binary
      // search
      // can not be used so we have to bail out.
      if (newValue > maxValue || newValue < minValue)
        throw StarException("StoredFunction is not monotonic.");

      index = newIndex;
      value = newValue;
    }
  }

  SearchResult result;
  result.targetValue = targetValue;
  result.searchTolerance = valueTolerance;
  result.found = std::fabs(targetValue - value) <= valueTolerance;
  result.solution = index;
  result.value = value;

  return result;
}

StoredFunction2::StoredFunction2(MultiTable2D table) : table(std::move(table)) {}

double StoredFunction2::evaluate(double x, double y) const {
  return table.interpolate({x, y});
}

StoredConfigFunction::StoredConfigFunction(ParametricTable<int, Json> data) {
  m_data = std::move(data);
}

Json StoredConfigFunction::get(double value) const {
  return m_data.get(value);
}

FunctionDatabase::FunctionDatabase() {
  auto assets = Root::singleton().assets();

  auto functions = assets->scanExtension("functions");
  auto sndFunctions = assets->scanExtension("2functions");
  auto configFunctions = assets->scanExtension("configfunctions");

  assets->queueJsons(functions);
  assets->queueJsons(sndFunctions);
  assets->queueJsons(configFunctions);

  for (auto file : functions) {
    for (auto const& functionPair : assets->json(file).iterateObject()) {
      if (m_functions.contains(functionPair.first))
        throw StarException(strf("Named Function '{}' defined twice, second time from {}", functionPair.first, file));
      m_functions[functionPair.first] = make_shared<StoredFunction>(parametricFunctionFromConfig(functionPair.second));
    }
  }

  for (auto file : sndFunctions) {
    for (auto const& functionPair : assets->json(file).iterateObject()) {
      if (m_functions2.contains(functionPair.first))
        throw StarException(
            strf("Named 2-ary Function '{}' defined twice, second time from {}", functionPair.first, file));
      m_functions2[functionPair.first] = make_shared<StoredFunction2>(multiTable2DFromConfig(functionPair.second));
    }
  }

  for (auto file : configFunctions) {
    for (auto const& tablePair : assets->json(file).iterateObject()) {
      if (m_configFunctions.contains(tablePair.first))
        throw StarException(
            strf("Named config function '{}' defined twice, second time from {}", tablePair.first, file));
      m_configFunctions[tablePair.first] =
          make_shared<StoredConfigFunction>(parametricTableFromConfig(tablePair.second));
    }
  }
}

StringList FunctionDatabase::namedFunctions() const {
  return m_functions.keys();
}

StringList FunctionDatabase::namedFunctions2() const {
  return m_functions2.keys();
}

StringList FunctionDatabase::namedConfigFunctions() const {
  return m_configFunctions.keys();
}

StoredFunctionPtr FunctionDatabase::function(Json const& configOrName) const {
  if (configOrName.type() == Json::Type::String)
    return m_functions.get(configOrName.toString());
  else
    return make_shared<StoredFunction>(parametricFunctionFromConfig(configOrName));
}

StoredFunction2Ptr FunctionDatabase::function2(Json const& configOrName) const {
  if (configOrName.type() == Json::Type::String)
    return m_functions2.get(configOrName.toString());
  else
    return make_shared<StoredFunction2>(multiTable2DFromConfig(configOrName));
}

StoredConfigFunctionPtr FunctionDatabase::configFunction(Json const& configOrName) const {
  if (configOrName.type() == Json::Type::String)
    return m_configFunctions.get(configOrName.toString());
  else
    return make_shared<StoredConfigFunction>(parametricTableFromConfig(configOrName));
}

ParametricFunction<double, double> FunctionDatabase::parametricFunctionFromConfig(Json descriptor) {
  try {
    String interpolationModeString = descriptor.getString(0);
    String boundModeString = descriptor.getString(1);

    List<pair<double, double>> points;
    for (size_t i = 2; i < descriptor.size(); ++i) {
      auto pointPair = descriptor.get(i);
      if (pointPair.size() != 2)
        throw StoredFunctionException("Each point must be a list of size 2");
      points.append({pointPair.getDouble(0), pointPair.getDouble(1)});
    }

    InterpolationMode interpolationMode;
    if (interpolationModeString.equalsIgnoreCase("HalfStep")) {
      interpolationMode = InterpolationMode::HalfStep;
    } else if (interpolationModeString.equalsIgnoreCase("Linear")) {
      interpolationMode = InterpolationMode::Linear;
    } else if (interpolationModeString.equalsIgnoreCase("Cubic")) {
      interpolationMode = InterpolationMode::Cubic;
    } else {
      throw StoredFunctionException(strf("Unrecognized InterpolationMode '{}'", interpolationModeString));
    }

    BoundMode boundMode;
    if (boundModeString.equalsIgnoreCase("Clamp")) {
      boundMode = BoundMode::Clamp;
    } else if (boundModeString.equalsIgnoreCase("Extrapolate")) {
      boundMode = BoundMode::Extrapolate;
    } else if (boundModeString.equalsIgnoreCase("Wrap")) {
      boundMode = BoundMode::Wrap;
    } else {
      throw StoredFunctionException(strf("Unrecognized BoundMode '{}'", boundModeString));
    }

    return ParametricFunction<double, double>(points, interpolationMode, boundMode);
  } catch (StarException const& e) {
    throw StoredFunctionException("Error parsing StoredFunction descriptor", e);
  }
}

ParametricTable<int, Json> FunctionDatabase::parametricTableFromConfig(Json descriptor) {
  try {
    List<pair<int, Json>> points;
    for (size_t i = 0; i < descriptor.size(); ++i) {
      auto pointPair = descriptor.get(i);
      if (pointPair.size() != 2)
        throw StoredFunctionException("Each point must be a list of size 2");
      points.append({pointPair.getInt(0), pointPair.get(1)});
    }

    return ParametricTable<int, Json>(points);
  } catch (StarException const& e) {
    throw StoredFunctionException("Error parsing StoredConfigFunction descriptor", e);
  }
}

MultiTable2D FunctionDatabase::multiTable2DFromConfig(Json descriptor) {
  try {
    String interpolationModeString = descriptor.getString(0);
    String boundModeString = descriptor.getString(1);

    List<double> xaxis;
    List<double> yaxis;
    MultiArray2D points;

    auto grid = descriptor.getArray(2);

    for (size_t y = 0; y < grid.size(); ++y) {
      auto row = grid[y].toArray();
      if (y == 0) {
        for (size_t x = 0; x < row.size(); ++x) {
          if (x > 0)
            xaxis.append(row[x].toFloat());
        }
        points.resize({row.size() - 1, grid.size() - 1});
      } else {
        yaxis.append(row[0].toFloat());
        auto cells = row[1].toArray();
        if (cells.size() != xaxis.size())
          throw StarException("Number of sample points doesn't match axis size.");
        for (size_t x = 0; x < cells.size(); x++)
          points.set({x, y - 1}, cells[x].toFloat());
      }
    }

    InterpolationMode interpolationMode;
    if (interpolationModeString.equalsIgnoreCase("HalfStep")) {
      interpolationMode = InterpolationMode::HalfStep;
    } else if (interpolationModeString.equalsIgnoreCase("Linear")) {
      interpolationMode = InterpolationMode::Linear;
    } else if (interpolationModeString.equalsIgnoreCase("Cubic")) {
      interpolationMode = InterpolationMode::Cubic;
    } else {
      throw StoredFunctionException(strf("Unrecognized InterpolationMode '{}'", interpolationModeString));
    }

    BoundMode boundMode;
    if (boundModeString.equalsIgnoreCase("Clamp")) {
      boundMode = BoundMode::Clamp;
    } else if (boundModeString.equalsIgnoreCase("Extrapolate")) {
      boundMode = BoundMode::Extrapolate;
    } else if (boundModeString.equalsIgnoreCase("Wrap")) {
      boundMode = BoundMode::Wrap;
    } else {
      throw StoredFunctionException(strf("Unrecognized BoundMode '{}'", boundModeString));
    }

    MultiTable2D table;
    table.setRange(0, std::vector<double>(xaxis.begin(), xaxis.end()));
    table.setRange(1, std::vector<double>(yaxis.begin(), yaxis.end()));
    table.setInterpolationMode(interpolationMode);
    table.setBoundMode(boundMode);
    table.array() = points;

    return table;
  } catch (StarException const& e) {
    throw StoredFunctionException("Error parsing function2 descriptor", e);
  }
}

}
