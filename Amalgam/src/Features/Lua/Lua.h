#pragma once
#include "../../SDK/SDK.h"
#include <filesystem>
#include <string>
#include <vector>

class CLua {
public:
  void RefreshScripts();
  void OpenDirectory();
  void LoadScript(std::string sName);

  std::vector<std::string> m_vScripts;
  std::string m_sScriptsPath;

  CLua();
};

ADD_FEATURE(CLua, Lua);
