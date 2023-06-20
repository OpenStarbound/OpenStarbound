#include "StarLogging.hpp"

namespace Star {

EnumMap<LogLevel> const LogLevelNames{
  {LogLevel::Debug, "Debug"},
  {LogLevel::Info, "Info"},
  {LogLevel::Warn, "Warn"},
  {LogLevel::Error, "Error"}
};

LogSink::LogSink()
  : m_level(LogLevel::Info) {}

LogSink::~LogSink() {}

void LogSink::setLevel(LogLevel level) {
  m_level = level;
}

LogLevel LogSink::level() {
  return m_level;
}

void StdoutLogSink::log(char const* msg, LogLevel level) {
  MutexLocker locker(m_logMutex);
  coutf("[%s] %s\n", LogLevelNames.getRight(level), msg);
}

FileLogSink::FileLogSink(String const& filename, LogLevel level, bool truncate) {
  if (truncate)
    m_output = File::open(filename, IOMode::Write | IOMode::Append | IOMode::Truncate);
  else
    m_output = File::open(filename, IOMode::Write | IOMode::Append);
  setLevel(level);
}

void FileLogSink::log(char const* msg, LogLevel level) {
  MutexLocker locker(m_logMutex);
  auto line = strf("[%s] [%s] %s\n", Time::printCurrentDateAndTime("<hours>:<minutes>:<seconds>.<millis>"), LogLevelNames.getRight(level), msg);
  m_output->write(line.data(), line.size());
}

void Logger::addSink(LogSinkPtr s) {
  MutexLocker locker(s_mutex);
  s_sinks.insert(s);
}

void Logger::removeSink(LogSinkPtr s) {
  MutexLocker locker(s_mutex);
  s_sinks.erase(s);
}

LogSinkPtr Logger::stdoutSink() {
  MutexLocker locker(s_mutex);
  return s_stdoutSink;
}

void Logger::removeStdoutSink() {
  MutexLocker locker(s_mutex);
  s_sinks.erase(s_stdoutSink);
}

void Logger::log(LogLevel level, char const* msg) {
  MutexLocker locker(s_mutex);

  for (auto const& l : s_sinks) {
    if (l->level() <= level)
      l->log(msg, level);
  }
}

shared_ptr<StdoutLogSink> Logger::s_stdoutSink = make_shared<StdoutLogSink>();
HashSet<LogSinkPtr> Logger::s_sinks{s_stdoutSink};
Mutex Logger::s_mutex;

String LogMap::getValue(String const& key) {
  MutexLocker locker(s_logMapMutex);
  return s_logMap.value(key);
}

void LogMap::setValue(String const& key, String const& value) {
  MutexLocker locker(s_logMapMutex);
  s_logMap[key] = value;
}

Map<String, String> LogMap::getValues() {
  MutexLocker locker(s_logMapMutex);
  return Map<String, String>::from(s_logMap);
}

void LogMap::clear() {
  MutexLocker locker(s_logMapMutex);
  s_logMap.clear();
}

HashMap<String, String> LogMap::s_logMap;
Mutex LogMap::s_logMapMutex;

size_t const SpatialLogger::MaximumLines;
size_t const SpatialLogger::MaximumPoints;
size_t const SpatialLogger::MaximumText;

void SpatialLogger::logPoly(char const* space, PolyF const& poly, Vec4B const& color) {
  MutexLocker locker(s_mutex);
  auto& lines = s_lines[space];

  for (size_t i = 0; i < poly.sides(); ++i) {
    auto side = poly.side(i);
    lines.append(Line{side.min(), side.max(), color});
  }

  while (lines.size() > MaximumLines)
    lines.removeFirst();
}

void SpatialLogger::logLine(char const* space, Line2F const& line, Vec4B const& color) {
  MutexLocker locker(s_mutex);
  auto& lines = s_lines[space];

  lines.append(Line{line.min(), line.max(), color});

  while (lines.size() > MaximumLines)
    lines.removeFirst();
}

void SpatialLogger::logLine(char const* space, Vec2F const& begin, Vec2F const& end, Vec4B const& color) {
  MutexLocker locker(s_mutex);
  auto& lines = s_lines[space];

  lines.append(Line{begin, end, color});

  while (lines.size() > MaximumLines)
    lines.removeFirst();
}

void SpatialLogger::logPoint(char const* space, Vec2F const& position, Vec4B const& color) {
  MutexLocker locker(s_mutex);
  auto& points = s_points[space];

  points.append(Point{position, color});

  while (points.size() > MaximumPoints)
    points.removeFirst();
}

void SpatialLogger::logText(char const* space, String text, Vec2F const& position, Vec4B const& color) {
  MutexLocker locker(s_mutex);
  auto& texts = s_logText[space];

  texts.append(LogText{text, position, color});

  while (texts.size() > MaximumText)
    texts.removeFirst();
}

Deque<SpatialLogger::Line> SpatialLogger::getLines(char const* space, bool andClear) {
  MutexLocker locker(s_mutex);
  if (andClear)
    return take(s_lines[space]);
  else
    return s_lines[space];
}

Deque<SpatialLogger::Point> SpatialLogger::getPoints(char const* space, bool andClear) {
  MutexLocker locker(s_mutex);
  if (andClear)
    return take(s_points[space]);
  else
    return s_points[space];
}

Deque<SpatialLogger::LogText> SpatialLogger::getText(char const* space, bool andClear) {
  MutexLocker locker(s_mutex);
  if (andClear)
    return take(s_logText[space]);
  else
    return s_logText[space];
}

void SpatialLogger::clear() {
  MutexLocker locker(s_mutex);
  s_lines.clear();
  s_points.clear();
  s_logText.clear();
}

Mutex SpatialLogger::s_mutex;
StringMap<Deque<SpatialLogger::Line>> SpatialLogger::s_lines;
StringMap<Deque<SpatialLogger::Point>> SpatialLogger::s_points;
StringMap<Deque<SpatialLogger::LogText>> SpatialLogger::s_logText;
}
