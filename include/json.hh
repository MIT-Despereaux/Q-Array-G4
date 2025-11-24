#ifndef JSON_h
#define JSON_h

#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <sstream>

class json;
typedef std::vector<json> jsonvec;
typedef std::map<std::string, json> jsonmap;

class json
{
public:
  enum TYPE
  {
    null,
    BOOL,
    INT,
    FLOAT,
    STRING,
    LIST,
    OBJ
  };

  // constructors
  json(TYPE type = null) : _type(type) {}

  json(const std::string &value) : _type(STRING), _sval(value) {}
  json(const char *value) : _type(STRING), _sval(value) {}
  json(std::nullptr_t) : _type(null) {}
  json(bool value) : _type(BOOL), _bval(value) {}
  json(int value) : _type(INT), _ival(value) {}
  json(long value) : _type(INT), _ival(value) {}
  json(double value) : _type(FLOAT), _dval(value) {}
  json(const jsonvec &value) : _type(LIST), _vval(value) {}
  json(const jsonmap &value) : _type(OBJ), _mval(value) {}

  json(const json &right)
  {
    *this = right;
  }

  json &operator=(const json &right)
  {
    _type = right._type;
    _bval = right._bval;
    _ival = right._ival;
    _dval = right._dval;
    _sval = right._sval;
    _vval = right._vval;
    _mval = right._mval;
    return *this;
  }

  template <typename T>
  json &operator=(const T &right)
  {
    to_json(*this, right);
    return *this;
  }

  // size
  size_t size() const
  {
    switch (_type)
    {
    case LIST:
      return _vval.size();
    case OBJ:
      return _mval.size();
    default:
      return 0;
    };
    return 0;
  }

  // index access
  json &operator[](int index)
  {
    return _vval[index];
  }

  const json &operator[](int index) const
  {
    return _vval[index];
  }

  json &operator[](const char *key)
  {
    return operator[](std::string(key));
  }

  json &operator[](const std::string &key)
  {
    checktype(OBJ);
    return _mval[key];
  }

  void push_back(const json &o)
  {
    checktype(LIST);
    _vval.push_back(o);
  }

  std::string print(int indent = 0) const;

  template <typename T>
  const T &get() const;

  template <typename T>
  const T &getdefault(const std::string &key,
                      const T &def) const
  {
    const auto &iter = _mval.find(key);
    return iter == _mval.end() ? def : iter->second;
  }

  operator bool() const
  {
    switch (_type)
    {
    case (BOOL):
      return _bval;
    case (INT):
      return _ival;
    case (FLOAT):
      return _dval;
    case (STRING):
      return !_sval.empty();
    default:
      return size();
    };
    // shouldn't actually get here...
    return false;
  };

  const jsonvec &getlist() const { return _vval; }
  const jsonmap &getobj() const { return _mval; }

private:
  TYPE _type = null;
  bool _bval = false;
  long _ival = 0;
  double _dval = 0;
  std::string _sval = "";
  jsonvec _vval;
  jsonmap _mval;

  void checktype(TYPE t)
  {
    if (_type == null)
      _type = t;
    if (_type != t)
    {
      std::stringstream err;
      err << "Cannot convert json of type " << _type << " to type " << t;
      throw std::logic_error(err.str());
    }
  }
};

inline std::ostream &operator<<(std::ostream &os, const json &js)
{
  return os << js.print();
}

inline std::ostream &operator<<(std::ostream &os, json::TYPE t)
{
  switch (t)
  {
  case (json::null):
    os << "null";
    break;
  case (json::BOOL):
    os << "BOOL";
    break;
  case (json::INT):
    os << "INT";
    break;
  case (json::FLOAT):
    os << "FLOAT";
    break;
  case (json::STRING):
    os << "STRING";
    break;
  case (json::LIST):
    os << "LIST";
    break;
  case (json::OBJ):
    os << "OBJ";
    break;
  }
  return os;
}

inline std::string json::print(int indent) const
{
  std::string childprefix("\n");
  childprefix.append(2 * (indent + 1), ' ');
  std::stringstream os;
  switch (_type)
  {
  case (null):
    os << "null";
    break;
  case (BOOL):
    os << _bval;
    break;
  case (INT):
    os << _ival;
    break;
  case (FLOAT):
    os << _dval;
    break;
  case (STRING):
    os << '"';
    for (char c : _sval)
      os << (c == '"' ? '\'' : c);
    os << '"';
    break;
  case (LIST):
    os << "[";
    for (const json &sub : _vval)
      os << childprefix << sub.print(indent + 1) << ',';
    if (!_vval.empty())
    {
      os.seekp(-1, os.cur); // remove the trailing comma
      os << '\n'
         << std::string(2 * indent, ' ');
    }
    os << ']';
    break;
  case (OBJ):
    os << "{";
    for (auto &item : _mval)
    {
      os << childprefix << '"' << item.first << "\": "
         << item.second.print(indent + 1) << ',';
    }
    if (!_mval.empty())
    {
      os.seekp(-1, os.cur); // remove the trailing comma
      os << '\n'
         << std::string(2 * indent, ' ');
    }
    os << '}';
    break;
  }
  return os.str();
}

template <>
inline const bool &json::get() const { return _bval; }

template <>
inline const long &json::get() const { return _ival; }

template <>
inline const double &json::get() const { return _dval; }

template <>
inline const std::string &json::get() const { return _sval; }

template <>
inline const jsonvec &json::get() const { return _vval; }

template <>
inline const jsonmap &json::get() const { return _mval; }

template <typename T>
inline void to_json(json &j, const T &t) { j = json(t); }

#endif
