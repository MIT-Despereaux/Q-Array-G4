#include <uuid/uuid.h>
#include <sstream>
#include <stdexcept>
#include <fstream>

#include "Metadata.hh"
#include "globals.hh"

#include "G4UIcommand.hh"
#include "G4UIparameter.hh"
#include "G4Version.hh"

#include "G4UIcmdWithAString.hh"
#include "G4UIcmdWithABool.hh"
#include "G4UIcmdWithAnInteger.hh"
#include "G4UIcmdWithADouble.hh"
#include "G4UIcmdWith3Vector.hh"
#include "G4UIcmdWithADoubleAndUnit.hh"
#include "G4UIcmdWith3VectorAndUnit.hh"
#include "G4UImanager.hh"

#include "MutexWrapper.hh"

// #include "version.h"

using ParseFunc = MetadataMessenger::ParseFunc;

Metadata::Metadata() : _messenger(new MetadataMessenger)
{
  // generate a uuid for this invocation
  // char uuid_str[40];
  // uuid_t uuid;
  // uuid_generate(uuid);
  // uuid_unparse(uuid, uuid_str);
  // G4cout << "The UUID for this run is " << uuid_str << G4endl;
  // _dataroot["uuid"] = uuid_str;

  // determine the version of geant we're running
  _dataroot["G4VERSION_NUMBER"] = G4VERSION_NUMBER;
  _dataroot["G4Version"] = G4Version;
  // _dataroot["G4Date"] = G4Date;
  // G4Version looks like "$Name: <version> <[MT]>$".
  // remove the surrounding '$' and MT to make it easier to parse in DB queries
  // std::size_t end = G4Version.rfind(' ');
  // _dataroot["geant4version"] = G4Version.substr(7, end - 7);

  // get the version of this software
  // G4cout << "Software version " << GITVERSION << " / " << GEOMVERSION << G4endl;
  // _dataroot["gitversion"] = GITVERSION;
  // _dataroot["geomversion"] = GEOMVERSION;

  // make sure the command history contains everything
  G4UImanager *ui = G4UImanager::GetUIpointer();
  ui->SetMaxHistSize(1000);
  // set an alias for our uuid to be used in filenames
  // std::stringstream alias;
  // alias << "uuid " << uuid_str;
  // ui->SetAlias(alias.str().c_str());
}

Metadata::~Metadata()
{
}

/*
MutexWrapper<Metadata> Metadata::GetInstance()
{
  static Metadata _instance;
  static G4Mutex _mutex = G4MUTEX_INITIALIZER;
  return MutexWrapper<Metadata>(&_instance, &_mutex);
}
*/
/*
void Metadata::SetObjectFromString(const std::string& key,
                                      const std::string& val)
{
  _dataroot[key] = json::parse(val);
}
*/

void Metadata::SaveCommandHistory()
{
  G4UImanager *ui = G4UImanager::GetUIpointer();
  G4int nlines = ui->GetNumberOfHistory();
  json &history = _dataroot["commandhistory"];
  // reset it
  history = json(json::LIST);
  for (G4int i = 0; i < nlines; ++i)
    history.push_back(ui->GetPreviousCommand(i));
}

MetadataMessenger::MetadataMessenger() : G4UImessenger("/", "Generic metadata parameters directory")
{
  setMetaCmd = new G4UIcommand("/QR/setMetadata", this);
  setMetaCmd->SetGuidance("Set metadata <key> to <value>");
  setMetaCmd->SetParameter(new G4UIparameter("key", 's', false));
  setMetaCmd->SetParameter(new G4UIparameter("value", 's', false));
}

MetadataMessenger::~MetadataMessenger()
{
}

void MetadataMessenger::RegisterCommand(G4UIcommand *cmd,
                                        const std::string &key,
                                        ParseFunc parser)
{
  std::string fullkey = key.empty() ? cmd->GetCommandPath() : key;
  cmds[cmd] = {key, parser};
}

namespace CLHEP
{
  // convert three vector to json
  void to_json(json &j, const Hep3Vector &v)
  {
    for (size_t i = 0; i < 3; ++i)
      j.push_back(v[i]);
  }
  void from_json(const json &j, Hep3Vector &v)
  {
    v[0] = j[0].get<double>();
    v[1] = j[1].get<double>();
    v[2] = j[2].get<double>();
  }
}

json vectojson(const G4ThreeVector &vec)
{
  json j;
  CLHEP::to_json(j, vec);
  return j;
}

json _parsecmd(G4UIcommand *cmd, const G4String &newValue)
{
  // Helper function that takes in a cmd and a string and returns a json object
  // this is terrible c++, but it works...
  if (dynamic_cast<G4UIcmdWithAString *>(cmd))
  {
    return newValue;
  }
  else if (dynamic_cast<G4UIcmdWithABool *>(cmd))
  {
    return G4UIcommand::ConvertToBool(newValue);
  }
  else if (dynamic_cast<G4UIcmdWithAnInteger *>(cmd))
  {
    return G4UIcommand::ConvertToInt(newValue);
  }
  else if (dynamic_cast<G4UIcmdWithADouble *>(cmd))
  {
    return G4UIcommand::ConvertToDouble(newValue);
  }
  else if (dynamic_cast<G4UIcmdWith3Vector *>(cmd))
  {
    return vectojson(G4UIcommand::ConvertTo3Vector(newValue));
  }
  else if (dynamic_cast<G4UIcmdWithADoubleAndUnit *>(cmd))
  {
    return G4UIcommand::ConvertToDimensionedDouble(newValue);
  }
  else if (dynamic_cast<G4UIcmdWith3VectorAndUnit *>(cmd))
  {
    return vectojson(G4UIcommand::ConvertToDimensioned3Vector(newValue));
  }
  else
  {
    G4ExceptionDescription msg;
    msg << "MetadataMessenger::SetNewValue Unknown type for command "
        << cmd->GetCommandPath();
    G4Exception("Metadata::SetNewValue", "1", FatalException, msg);
  }
  return json();
}

ParseFunc MetadataMessenger::sDefaultParser(_parsecmd);

void MetadataMessenger::SetNewValue(G4UIcommand *cmd, G4String newValue)
{
  /*
  G4cout<<"MetadataMessenger::SetNewValue got command "
        <<cmd->GetCommandPath()<<" with value "<<newValue<<G4endl;
  */
  auto meta = Metadata::GetInstance();

  std::string key = "";
  json val;
  // ParseFunc parser = sDefaultParser; //< Where is this used?
  if (cmd == setMetaCmd)
  {
    // string should contain key name and new value
    std::size_t space = newValue.find(' ');
    key = newValue.substr(0, space);
    val = newValue.substr(space + 1);
  }
  else
  {
    auto entry = cmds.find(cmd);
    if (entry == cmds.end())
    {
      G4cerr << "MetadataMessenger::SetNewValue command not found" << G4endl;
      return;
    }
    key = entry->second.first;
    val = entry->second.second(cmd, newValue);
  }
  if (key != Metadata::NullKey())
    meta->Set(key, val);
  // G4cout<<"MetadataMessenger::SetNewValue metadata[\""<<key<<"\"] = "
  //       <<meta->GetJSON(key)<<G4endl;

  // Note that the commands are not actually invoked
  // Actual commands are done manually by the user
}

// TODO: fix this if broken
G4String MetadataMessenger::GetCurrentValue(G4UIcommand *cmd)
{
  auto entry = cmds.find(cmd);
  if (entry == cmds.end())
  {
    G4cerr << "MetadataMessenger::SetNewValue command not found" << G4endl;
    return "not found";
  }

  const std::string &key = entry->second.first;
  json &value = Metadata::GetDataRoot()[key];

  // this probably doesn't work...
  G4String result = value.get<std::string>();
  return result;
}

bool Metadata::Write(const std::string &filename)
{
  SaveCommandHistory();
  std::ofstream fout(filename.c_str());
  return bool(fout << _dataroot);
}
