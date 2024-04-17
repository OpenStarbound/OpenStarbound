#pragma once

#include <vector>

#include "StarUnicode.hpp"

namespace Star {

struct JsonStream {
  virtual ~JsonStream() {}

  virtual void beginObject() = 0;
  virtual void objectKey(char32_t const*, size_t) = 0;
  virtual void endObject() = 0;

  virtual void beginArray() = 0;
  virtual void endArray() = 0;

  virtual void putString(char32_t const*, size_t) = 0;
  virtual void putDouble(char32_t const*, size_t) = 0;
  virtual void putInteger(char32_t const*, size_t) = 0;
  virtual void putBoolean(bool) = 0;
  virtual void putNull() = 0;

  virtual void putWhitespace(char32_t const*, size_t) = 0;
  virtual void putColon() = 0;
  virtual void putComma() = 0;
};

// Will parse JSON and output to a given JsonStream.  Parses an *extension* to
// the JSON format that includes comments.
template <typename InputIterator>
class JsonParser {
public:
  JsonParser(JsonStream& stream)
    : m_line(0), m_column(0), m_stream(stream) {}
  virtual ~JsonParser() {}

  // Does not throw.  On error, returned iterator will not be equal to end, and
  // error() will be non-null.  Set fragment to true to parse any JSON type
  // rather than just object or array.
  InputIterator parse(InputIterator begin, InputIterator end, bool fragment = false) {
    init(begin, end);

    try {
      white();
      if (fragment)
        value();
      else
        top();
      white();
    } catch (ParsingException const&) {
    }

    return m_current;
  }

  // Human readable parsing error, does not include line or column info.
  char const* error() const {
    if (m_error.empty())
      return nullptr;
    else
      return m_error.c_str();
  }

  size_t line() const {
    return m_line + 1;
  }

  size_t column() const {
    return m_column + 1;
  }

private:
  typedef std::basic_string<char32_t> CharArray;

  // Thrown internally to abort parsing.
  class ParsingException {};

  void top() {
    switch (m_char) {
      case '{':
        object();
        break;
      case '[':
        array();
        break;
      default:
        error("expected JSON object or array at top level");
        return;
    }
  }

  void value() {
    switch (m_char) {
      case '{':
        object();
        break;
      case '[':
        array();
        break;
      case '"':
        string();
        break;
      case '-':
        number();
        break;
      case 0:
        error("unexpected end of stream parsing value");
        return;
      default:
        m_char >= '0' && m_char <= '9' ? number() : word();
        break;
    }
  }

  void object() {
    if (m_char != '{')
      error("bad object, should be '{'");

    next();
    m_stream.beginObject();

    white();
    if (m_char == '}') {
      next();
      m_stream.endObject();
      return;
    }

    while (true) {
      CharArray s = parseString();
      m_stream.objectKey(s.c_str(), s.length());

      white();
      if (m_char != ':')
        error("bad object, should be ':'");
      next();
      m_stream.putColon();
      white();

      value();

      white();
      if (m_char == '}') {
        next();
        m_stream.endObject();
        return;
      } else if (m_char == ',') {
        next();
        m_stream.putComma();
        white();
      } else if (m_char == 0) {
        error("unexpected end of stream parsing object.");
      } else {
        error("bad object, should be '}' or ','");
      }
    }
  }

  void array() {
    if (m_char == '[') {
      next();
      m_stream.beginArray();
      white();
      if (m_char == ']') {
        next();
        m_stream.endArray();
      } else {
        while (true) {
          value();
          white();
          if (m_char == ']') {
            next();
            m_stream.endArray();
            break;
          } else if (m_char == ',') {
            next();
            m_stream.putComma();
            white();
          } else if (m_char == 0) {
            error("unexpected end of stream parsing array.");
          } else {
            error("bad array, should be ',' or ']'");
          }
        }
      }
    } else {
      error("bad array");
    }
  }

  void string() {
    CharArray s = parseString();
    m_stream.putString(s.c_str(), s.length());
  }

  void number() {
    std::basic_string<char32_t> buffer;
    bool isDouble = false;

    if (m_char == '-') {
      buffer += '-';
      next();
    }

    if (m_char == '0') {
      buffer += '0';
      next();
    } else if (m_char > '0' && m_char <= '9') {
      while (m_char >= '0' && m_char <= '9') {
        buffer += m_char;
        next();
      }
    } else {
      error("bad number, must start with digit");
    }

    if (m_char == '.') {
      isDouble = true;
      buffer += '.';
      next();
      while (m_char >= '0' && m_char <= '9') {
        buffer += m_char;
        next();
      }
    }

    if (m_char == 'e' || m_char == 'E') {
      isDouble = true;
      buffer += m_char;
      next();
      if (m_char == '-' || m_char == '+') {
        buffer += m_char;
        next();
      }
      while (m_char >= '0' && m_char <= '9') {
        buffer += m_char;
        next();
      }
    }

    if (isDouble) {
      try {
        m_stream.putDouble(buffer.c_str(), buffer.length());
      } catch (std::exception const& e) {
        error(std::string("Bad double: ") + e.what());
      }
    } else {
      try {
        m_stream.putInteger(buffer.c_str(), buffer.length());
      } catch (std::exception const& e) {
        error(std::string("Bad integer: ") + e.what());
      }
    }
  }

  // true, false, or null
  void word() {
    switch (m_char) {
      case 't':
        next();
        check('r');
        check('u');
        check('e');
        m_stream.putBoolean(true);
        break;
      case 'f':
        next();
        check('a');
        check('l');
        check('s');
        check('e');
        m_stream.putBoolean(false);
        break;
      case 'n':
        next();
        check('u');
        check('l');
        check('l');
        m_stream.putNull();
        break;
      default:
        error("unexpected character parsing word");
        return;
    }
  }

  CharArray parseString() {
    if (m_char != '"')
      error("bad string, should be '\"'");
    next();

    CharArray str;

    while (true) {
      if (m_char == '\\') {
        next();
        if (m_char == 'u') {
          std::string hexString;
          next();
          for (int i = 0; i < 4; ++i) {
            hexString.push_back(m_char);
            next();
          }
          char32_t codepoint = hexStringToUtf32(hexString);
          if (isUtf16LeadSurrogate(codepoint)) {
            check('\\');
            check('u');
            hexString.clear();
            for (int i = 0; i < 4; ++i) {
              hexString.push_back(m_char);
              next();
            }
            codepoint = hexStringToUtf32(hexString, codepoint);
          }
          str += codepoint;
        } else {
          switch (m_char) {
            case '"':
              str += '"';
              break;
            case '\\':
              str += '\\';
              break;
            case '/':
              str += '/';
              break;
            case 'b':
              str += '\b';
              break;
            case 'f':
              str += '\f';
              break;
            case 'n':
              str += '\n';
              break;
            case 'r':
              str += '\r';
              break;
            case 't':
              str += '\t';
              break;
            default:
              error("bad string escape character");
              break;
          }
          next();
        }
      } else if (m_char == '\"') {
        next();
        return str;
      } else if (m_char == 0) {
        error("unexpected end of stream reading string!");
      } else {
        str += m_char;
        next();
      }
    }
    error("parser bug");
    return {};
  }

  // Checks current char then moves on to the next one
  void check(char32_t c) {
    if (m_char == 0)
      error("unexpected end of stream parsing word");
    if (m_char != c)
      error("unexpected character in word");
    next();
  }

  void init(InputIterator begin, InputIterator end) {
    m_current = begin;
    m_end = end;
    m_line = 0;
    m_column = 0;

    if (m_current != m_end)
      m_char = *m_current;
    else
      m_char = 0;
  }

  // Consumes next character.
  void next() {
    if (m_current == m_end)
      return;

    if (m_char == '\n') {
      ++m_line;
      m_column = 0;
    } else {
      ++m_column;
    }
    ++m_current;

    if (m_current != m_end)
      m_char = *m_current;
    else
      m_char = 0;
  }

  // Will skip whitespace and comments between tokens.
  void white() {
    CharArray buffer;
    while (m_current != m_end) {
      if (m_char == '/') {
        // Always consume '/' found in whitespace, because that is never valid
        // JSON (other than comments)
        buffer += m_char;
        next();
        if (m_current != m_end && m_char == '/') {
          // eat "/"
          buffer += m_char;
          next();

          // Read '//' style comments up until eol/eof.
          while (m_current != m_end && m_char != '\n') {
            buffer += m_char;
            next();
          }
        } else if (m_current != m_end && m_char == '*') {
          // eat "*"
          buffer += m_char;
          next();

          // Read '/*' style comments up until '*/'.
          while (m_current != m_end) {
            if (m_char == '*') {
              buffer += m_char;
              next();
              if (m_char == '/') {
                buffer += m_char;
                next();
                break;
              }
            } else {
              buffer += m_char;
              next();
              if (m_current == m_end)
                error("/* comment has no matching */");
            }
          }
        } else {
          // The only allowed characters following / in whitespace are / and *
          error("/ character in whitespace is not follwed by '/' or '*', invalid comment");
          return;
        }
      } else if (isSpace(m_char)) {
        buffer += m_char;
        next();
      } else {
        if (buffer.size() != 0)
          m_stream.putWhitespace(buffer.c_str(), buffer.length());
        return;
      }
    }
    if (buffer.size() != 0)
      m_stream.putWhitespace(buffer.c_str(), buffer.length());
  }

  void error(std::string msg) {
    m_error = std::move(msg);
    throw ParsingException();
  }

  bool isSpace(char32_t c) {
    // Only whitespace allowed by JSON
    return c == 0x20 || // space
        c == 0x09 || // horizontal tab
        c == 0x0a || // newline
        c == 0x0d || // carriage return
        c == 0xfeff; // BOM or ZWNBSP
  }

  char32_t m_char;
  InputIterator m_current;
  InputIterator m_end;
  size_t m_line;
  size_t m_column;
  std::string m_error;
  JsonStream& m_stream;
};

// Write JSON through JsonStream interface.
template <typename OutputIterator>
class JsonWriter : public JsonStream {
public:
  JsonWriter(OutputIterator out, unsigned pretty = 0)
    : m_out(out), m_pretty(pretty) {}

  void beginObject() {
    startValue();
    pushState(Object);
    write('{');
  }

  void objectKey(char32_t const* s, size_t len) {
    if (currentState() == ObjectElement) {
      if (m_pretty)
        write('\n');
      indent();
    } else {
      pushState(ObjectElement);
      if (m_pretty)
        write('\n');
      indent();
    }

    write('"');
    char32_t c = *s;
    while (c && len) {
      write(c);
      c = *++s;
      --len;
    }
    write('"');
    if (m_pretty)
      write(' ');
  }

  void endObject() {
    if (currentState() == ObjectElement) {
      if (m_pretty)
        write('\n');
      indent();
    }
    popState(Object);
    write('}');
  }

  void beginArray() {
    startValue();
    pushState(Array);
    write('[');
  }

  void endArray() {
    popState(Array);
    write(']');
  }

  void putString(char32_t const* s, size_t len) {
    startValue();

    write('"');
    char32_t c = *s;
    while (c && (len > 0)) {
      if (!isPrintable(c)) {
        switch (c) {
          case '"':
            write('\\');
            write('"');
            break;
          case '\\':
            write('\\');
            write('\\');
            break;
          case '\b':
            write('\\');
            write('b');
            break;
          case '\f':
            write('\\');
            write('f');
            break;
          case '\n':
            write('\\');
            write('n');
            break;
          case '\r':
            write('\\');
            write('r');
            break;
          case '\t':
            write('\\');
            write('t');
            break;
          default:
            auto hex = hexStringFromUtf32(c);
            if (hex.size() == 4) {
              write('\\');
              write('u');
              for (auto c : hex) {
                write(c);
              }
            } else if (hex.size() == 8) {
              write('\\');
              write('u');
              for (auto c : hex.substr(0, 4)) {
                write(c);
              }
              write('\\');
              write('u');
              for (auto c : hex.substr(4)) {
                write(c);
              }
            } else {
              throw UnicodeException("Internal Error: Received invalid unicode hex from hexStringFromUtf32.");
            }
            break;
        }
      } else {
        write(c);
      }
      c = *++s;
      --len;
    }
    write('"');
  }

  void putDouble(char32_t const* s, size_t len) {
    startValue();
    for (size_t i = 0; i < len; ++i)
      write(s[i]);
  }

  void putInteger(char32_t const* s, size_t len) {
    startValue();
    for (size_t i = 0; i < len; ++i)
      write(s[i]);
  }

  void putBoolean(bool b) {
    startValue();
    if (b) {
      write('t');
      write('r');
      write('u');
      write('e');
    } else {
      write('f');
      write('a');
      write('l');
      write('s');
      write('e');
    }
  }

  void putNull() {
    startValue();
    write('n');
    write('u');
    write('l');
    write('l');
  }

  void putWhitespace(char32_t const* s, size_t len) {
    // If m_pretty is true, extra spurious whitespace will be inserted.
    for (size_t i = 0; i < len; ++i)
      write(s[i]);
  }

  void putColon() {
    write(':');
    if (m_pretty)
      write(' ');
  }

  void putComma() {
    write(',');
  }

private:
  enum State {
    Top,
    Object,
    ObjectElement,
    Array,
    ArrayElement
  };

  // Handles separating array elements if currently adding to an array
  void startValue() {
    if (currentState() == ArrayElement) {
      if (m_pretty)
        write(' ');
    } else if (currentState() == Array) {
      pushState(ArrayElement);
    }
  }

  void indent() {
    for (unsigned i = 0; i < m_state.size() / 2; ++i) {
      for (unsigned j = 0; j < m_pretty; ++j) {
        write(' ');
      }
    }
  }

  // Push state onto stack.
  void pushState(State state) {
    m_state.push_back(state);
  }

  // Pop state stack down to given state.
  void popState(State state) {
    while (true) {
      if (m_state.empty())
        return;

      State last = currentState();
      m_state.pop_back();
      if (last == state)
        return;
    }
  }

  State currentState() {
    if (m_state.empty())
      return Top;
    else
      return *prev(m_state.end());
  }

  void write(char32_t c) {
    *m_out = c;
    ++m_out;
  }

  // Only chars that are unescaped according to JSON spec.
  bool isPrintable(char32_t c) {
    return (c >= 0x20 && c <= 0x21) || (c >= 0x23 && c <= 0x5b) || (c >= 0x5d && c <= 0x10ffff);
  }

  OutputIterator m_out;
  unsigned m_pretty;
  std::vector<State> m_state;
};

}
