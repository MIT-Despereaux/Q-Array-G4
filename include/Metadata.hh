#ifndef Metadata_h
#define Metadata_h

/** Metadata is a singleton class to store and save metadata about a
    geant4 run.
*/

#include "json.hh"

#include <string>
#include <utility>
#include <functional>
#include "G4String.hh"
#include "G4UImessenger.hh"

#include "MutexWrapper.hh"

class G4UIcommand;
class G4UIcmdWithAString;

class MetadataMessenger : public G4UImessenger
{
public:
  MetadataMessenger();
  ~MetadataMessenger();

  using ParseFunc = std::function<json(G4UIcommand *, const G4String &)>;
  static ParseFunc sDefaultParser;

  template <class Func>
  struct ParseFuncWrapper
  {
    Func _func;
    ParseFuncWrapper(const Func &func) : _func(func) {}
    json operator()(G4UIcommand *, const G4String &_s)
    {
      _func(_s);
      return _s;
    }
  };

  // register a command to change the value of a metadata reference
  void RegisterCommand(G4UIcommand *cmd, const std::string &key = "",
                       ParseFunc parser = sDefaultParser);

  G4String GetCurrentValue(G4UIcommand *command);
  void SetNewValue(G4UIcommand *command, G4String newValue);

private:
  std::map<G4UIcommand *, std::pair<std::string, ParseFunc>> cmds;
  G4UIcommand *setMetaCmd;
};

class Metadata;
typedef MutexWrapper<Metadata> MetaPtr;

class Metadata
{
private:
  // enforce singleton
  Metadata();

public:
  Metadata(const Metadata &right) = delete;
  void operator=(const Metadata &right) = delete;

private:
  json _dataroot;
  MetadataMessenger *_messenger;

public:
  ~Metadata();

  friend MetaPtr GetWrappedSingleton<Metadata>();
  static MetaPtr GetInstance()
  {
    return GetWrappedSingleton<Metadata>();
  }
  static inline json &GetDataRoot() { return GetInstance()->GetJSON(); }
  static inline std::string NullKey() { return "__NULL__"; }

  MetadataMessenger *GetMessenger() { return _messenger; }
  json &GetJSON(const std::string key = "")
  {
    return key.empty() ? _dataroot : _dataroot[key];
  }

  // utility functions to set value without direct access to json object
  template <typename T>
  void Set(const std::string &key, const T &val)
  {
    _dataroot[key] = val;
  }

  template <typename T>
  const T &Get(const std::string &key)
  {
    return _dataroot[key].get<T>();
  }

  const std::string &GetString(const std::string &key)
  {
    return Get<std::string>(key);
  }

  long GetInt(const std::string &key)
  {
    return Get<long>(key);
  }

  bool GetBool(const std::string &key)
  {
    return Get<bool>(key);
  }

  double GetDouble(const std::string &key)
  {
    return Get<double>(key);
  }

  G4ThreeVector GetThreeVector(const std::string &key)
  {
    json &entry = _dataroot[key];
    return G4ThreeVector(entry[0].get<double>(), entry[1].get<double>(),
                         entry[2].get<double>());
  }

  // void SetObjectFromString(const std::string& key, const std::string& val);

  // get additional metadata
  void SaveCommandHistory();

  // save everything
  bool Write(const std::string &key);

  // version with no parameter name
  template <typename T>
  T *AddParamCommand(const G4String &path,
                     const G4String &guidance,
                     MetadataMessenger::ParseFunc parser =
                         MetadataMessenger::sDefaultParser,
                     const std::string &key = "");

  // with parameter name
  template <typename T>
  T *AddParamCommand(const G4String &path,
                     const G4String &guidance,
                     const G4String &parname,
                     MetadataMessenger::ParseFunc parser =
                         MetadataMessenger::sDefaultParser,
                     const std::string &key = "");

  template <typename F>
  G4UIcmdWithAString *AddParamCommand(const G4String &path,
                                      const G4String &guidance,
                                      const G4String &parname,
                                      const F &func,
                                      const std::string &key = "");
};

template <>
inline void Metadata::Set(const std::string &key,
                          const G4ThreeVector &val)
{
  _dataroot[key] = jsonvec{val[0], val[1], val[2]};
}

template <typename T>
inline T *Metadata::AddParamCommand(const G4String &path,
                                    const G4String &guidance,
                                    MetadataMessenger::ParseFunc parser,
                                    const std::string &key)
{
  T *cmd = new T(path, GetMessenger());
  cmd->SetGuidance(guidance);
  GetMessenger()->RegisterCommand(cmd, key.empty() ? path : key, parser);
  return cmd;
}

template <typename T>
inline T *Metadata::AddParamCommand(const G4String &path,
                                    const G4String &guidance,
                                    const G4String &parname,
                                    MetadataMessenger::ParseFunc parser,
                                    const std::string &key)
{
  T *cmd = AddParamCommand<T>(path, guidance, parser, key);
  cmd->SetParameterName(parname, false);
  return cmd;
}

template <typename F>
inline G4UIcmdWithAString *Metadata::AddParamCommand(const G4String &path,
                                                     const G4String &guidance,
                                                     const G4String &parname,
                                                     const F &func,
                                                     const std::string &key)
{
  MetadataMessenger::ParseFuncWrapper<F> wrapper(func);
  return AddParamCommand<G4UIcmdWithAString>(path, guidance, parname,
                                             wrapper, key);
}

#endif
