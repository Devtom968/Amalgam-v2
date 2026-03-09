#include "Lua.h"
#include <filesystem>
#include <fstream>
#include <shellapi.h>
#include <sstream>
#include <windows.h>

CLua::CLua() {
  // m_sScriptsPath initialization removed from constructor.
  // It is expected to be initialized elsewhere or handled by lazy
  // initialization in methods that use it.
}

void CLua::RefreshScripts() {
  m_vScripts.clear();

  // Lazy initialization/creation of the scripts directory
  // Assuming m_sScriptsPath is initialized before this point, or
  // default-constructed and then set. If m_sScriptsPath is empty, this check
  // might not behave as expected. For a robust solution, m_sScriptsPath itself
  // should be lazily initialized if it's not set in the constructor. For now,
  // we'll assume m_sScriptsPath is set by the time RefreshScripts is called.
  if (m_sScriptsPath.empty()) {
    m_sScriptsPath =
        std::filesystem::current_path().string() + "\\Amalgam V2\\Scripts\\";
  }

  if (!std::filesystem::exists(m_sScriptsPath))
    std::filesystem::create_directories(m_sScriptsPath);

  try {
    for (const auto &entry :
         std::filesystem::directory_iterator(m_sScriptsPath)) {
      if (entry.is_regular_file() && entry.path().extension() == ".lua") {
        m_vScripts.push_back(entry.path().filename().string());
      }
    }
    SDK::Output("Lua",
                (std::string("Refreshed scripts. Found: ") +
                 std::to_string(m_vScripts.size()))
                    .c_str(),
                {100, 200, 255, 255});
  } catch (...) {
  }
}

void CLua::OpenDirectory() {
  // Ensure m_sScriptsPath is initialized before use
  if (m_sScriptsPath.empty()) {
    m_sScriptsPath =
        std::filesystem::current_path().string() + "\\Amalgam V2\\Scripts\\";
  }
  ShellExecuteA(NULL, "open", m_sScriptsPath.c_str(), NULL, NULL,
                SW_SHOWDEFAULT);
}

void CLua::LoadScript(std::string sName) {
  if (sName.empty())
    return;

  // Ensure m_sScriptsPath is initialized before use
  if (m_sScriptsPath.empty()) {
    m_sScriptsPath =
        std::filesystem::current_path().string() + "\\Amalgam V2\\Scripts\\";
  }

  std::ifstream t(m_sScriptsPath + "\\" + sName);
  if (!t.is_open()) {
    SDK::Output("Lua", (std::string("Failed to open script: ") + sName).c_str(),
                {255, 100, 100, 255});
    return;
  }

  std::stringstream buffer;
  buffer << t.rdbuf();
  std::string sContent = buffer.str();

  // Temporary test: Output script content to console
  SDK::Output("Lua", (std::string("Executing script: ") + sName).c_str(),
              {100, 255, 100, 255});
  SDK::Output("Script Content", sContent.c_str(), {200, 200, 200, 255});
}
