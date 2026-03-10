#pragma once
#include "../../SDK/SDK.h"
#include <filesystem>
#include <shellapi.h>
#include <string>
#include <vector>
#include <windows.h>

typedef struct lua_State lua_State;

#include <unordered_map>

class CLua {
public:
  CLua();
  ~CLua();

  void RefreshScripts();
  void OpenDirectory();
  void LoadScript(std::string sName);
  void UnloadScript(std::string sName);
  void RunScript(std::string sName, const std::string &sScript);
  void RegisterFunctions();
  void OnRender();

  std::vector<std::string> m_vScripts;
  std::unordered_map<std::string, bool> m_mActiveScripts;
  std::unordered_map<std::string, int> m_mScriptEnvs;
  std::string m_sScriptsPath;
  lua_State *m_pState = nullptr;
};

ADD_FEATURE(CLua, Lua);
