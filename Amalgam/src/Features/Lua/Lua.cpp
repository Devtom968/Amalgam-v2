#define _CRT_SECURE_NO_WARNINGS
#include "WinHeaders.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "../../../include/ImGui/imgui.h"
#include "../../../include/ImGui/imgui_internal.h"
#include "../../SDK/SDK.h"
#include "Lua.h"

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

namespace fs = std::filesystem;

#define static_lua
#include "minilua.c"

#ifndef LUA_REGISTRYINDEX
#define LUA_REGISTRYINDEX (-10000)
#endif
#ifndef LUA_GLOBALSINDEX
#define LUA_GLOBALSINDEX (-10002)
#endif

// lua_absindex and other essential functions for minilua
static int lua_absindex(lua_State *L, int i) {
  if (i > 0 || i <= LUA_REGISTRYINDEX)
    return i;
  return lua_gettop(L) + i + 1;
}

int luaL_ref(lua_State *L, int t) {
  int ref;
  if (lua_isnil(L, -1)) {
    lua_pop(L, 1);
    return -1; // LUA_REFNIL
  }
  t = lua_absindex(L, t);
  ref = (int)lua_objlen(L, t);
  ref++;
  lua_rawseti(L, t, ref);
  return ref;
}

void luaL_unref(lua_State *L, int t, int ref) {
  if (ref >= 0) {
    t = lua_absindex(L, t);
    lua_pushnil(L);
    lua_rawseti(L, t, ref);
  }
}

// Helper to convert Lua color table to ImU32
ImU32 GetImColor(lua_State *L, int idx) {
  if (lua_istable(L, idx)) {
    lua_rawgeti(L, idx, 1);
    int r = (int)lua_tointeger(L, -1);
    lua_rawgeti(L, idx, 2);
    int g = (int)lua_tointeger(L, -1);
    lua_rawgeti(L, idx, 3);
    int b = (int)lua_tointeger(L, -1);
    lua_rawgeti(L, idx, 4);
    int a = lua_isnil(L, -1) ? 255 : (int)lua_tointeger(L, -1);
    lua_pop(L, 4);
    return IM_COL32(r, g, b, a);
  }
  return IM_COL32(255, 255, 255, 255);
}

// Math Bindings
int lua_math_floor(lua_State *L) {
  lua_pushnumber(L, std::floor(luaL_checknumber(L, 1)));
  return 1;
}

int lua_math_sin(lua_State *L) {
  lua_pushnumber(L, std::sin(luaL_checknumber(L, 1)));
  return 1;
}

int lua_math_cos(lua_State *L) {
  lua_pushnumber(L, std::cos(luaL_checknumber(L, 1)));
  return 1;
}

int lua_math_abs(lua_State *L) {
  lua_pushnumber(L, std::abs(luaL_checknumber(L, 1)));
  return 1;
}

int lua_math_sqrt(lua_State *L) {
  lua_pushnumber(L, std::sqrt(luaL_checknumber(L, 1)));
  return 1;
}

int lua_math_min(lua_State *L) {
  int n = lua_gettop(L);
  double m = luaL_checknumber(L, 1);
  for (int i = 2; i <= n; i++) {
    double d = luaL_checknumber(L, i);
    if (d < m)
      m = d;
  }
  lua_pushnumber(L, m);
  return 1;
}

int lua_math_max(lua_State *L) {
  int n = lua_gettop(L);
  double m = luaL_checknumber(L, 1);
  for (int i = 2; i <= n; i++) {
    double d = luaL_checknumber(L, i);
    if (d > m)
      m = d;
  }
  lua_pushnumber(L, m);
  return 1;
}

// Draw Bindings using ImGui DrawList (Thread-safe)
int lua_draw_line(lua_State *L) {
  float x1 = (float)luaL_checknumber(L, 1);
  float y1 = (float)luaL_checknumber(L, 2);
  float x2 = (float)luaL_checknumber(L, 3);
  float y2 = (float)luaL_checknumber(L, 4);
  ImU32 color = GetImColor(L, 5);
  ImGui::GetBackgroundDrawList()->AddLine({x1, y1}, {x2, y2}, color);
  return 0;
}

int lua_draw_rect(lua_State *L) {
  float x = (float)luaL_checknumber(L, 1);
  float y = (float)luaL_checknumber(L, 2);
  float w = (float)luaL_checknumber(L, 3);
  float h = (float)luaL_checknumber(L, 4);
  ImU32 color = GetImColor(L, 5);
  bool filled = lua_toboolean(L, 6);
  if (filled)
    ImGui::GetBackgroundDrawList()->AddRectFilled({x, y}, {x + w, y + h}, color,
                                                  4.f);
  else
    ImGui::GetBackgroundDrawList()->AddRect({x, y}, {x + w, y + h}, color, 4.f);
  return 0;
}

int lua_draw_text(lua_State *L) {
  float x = (float)luaL_checknumber(L, 1);
  float y = (float)luaL_checknumber(L, 2);
  const char *text = luaL_checkstring(L, 3);
  ImU32 color = GetImColor(L, 4);
  ImGui::GetBackgroundDrawList()->AddText({x, y}, color, text);
  return 0;
}

int lua_draw_get_screen_size(lua_State *L) {
  ImVec2 size = ImGui::GetIO().DisplaySize;
  lua_pushnumber(L, size.x);
  lua_pushnumber(L, size.y);
  return 2;
}

// Globals Bindings (Time, FPS)
int lua_globals_get_time(lua_State *L) {
  auto now = std::chrono::system_clock::now();
  std::time_t now_time = std::chrono::system_clock::to_time_t(now);
  std::tm *local_time = std::localtime(&now_time);
  char buffer[80];
  if (local_time)
    std::strftime(buffer, 80, "%H:%M:%S", local_time);
  else
    strcpy(buffer, "00:00:00");
  lua_pushstring(L, buffer);
  return 1;
}

int lua_globals_get_fps(lua_State *L) {
  lua_pushinteger(L, (int)ImGui::GetIO().Framerate);
  return 1;
}

int lua_globals_get_realtime(lua_State *L) {
  lua_pushnumber(L, (double)I::GlobalVars->realtime);
  return 1;
}

// Vars Bindings
int lua_vars_get(lua_State *L) {
  const char *name = luaL_checkstring(L, 1);
  for (auto &var : G::Vars) {
    if (std::string(var->Name()) == name) {
      if (var->m_iType == typeid(bool).hash_code()) {
        lua_pushboolean(L, var->As<bool>()->Value);
        return 1;
      }
      if (var->m_iType == typeid(int).hash_code()) {
        lua_pushinteger(L, var->As<int>()->Value);
        return 1;
      }
      if (var->m_iType == typeid(float).hash_code()) {
        lua_pushnumber(L, var->As<float>()->Value);
        return 1;
      }
      if (var->m_iType == typeid(std::string).hash_code()) {
        lua_pushstring(L, var->As<std::string>()->Value.c_str());
        return 1;
      }
    }
  }
  return 0;
}

int lua_vars_set(lua_State *L) {
  const char *name = luaL_checkstring(L, 1);
  for (auto &var : G::Vars) {
    if (std::string(var->Name()) == name) {
      if (var->m_iType == typeid(bool).hash_code()) {
        var->As<bool>()->Value = lua_toboolean(L, 2);
        return 0;
      }
      if (var->m_iType == typeid(int).hash_code()) {
        var->As<int>()->Value = (int)lua_tointeger(L, 2);
        return 0;
      }
      if (var->m_iType == typeid(float).hash_code()) {
        var->As<float>()->Value = (float)lua_tonumber(L, 2);
        return 0;
      }
      if (var->m_iType == typeid(std::string).hash_code()) {
        var->As<std::string>()->Value = lua_tostring(L, 2);
        return 0;
      }
    }
  }
  return 0;
}

// ImGui Bindings
int lua_ui_begin(lua_State *L) {
  const char *name = luaL_checkstring(L, 1);
  lua_pushboolean(L, ImGui::Begin(name));
  return 1;
}

int lua_ui_end(lua_State *L) {
  ImGui::End();
  return 0;
}

int lua_ui_text(lua_State *L) {
  const char *text = luaL_checkstring(L, 1);
  ImGui::Text("%s", text);
  return 0;
}

int lua_ui_button(lua_State *L) {
  const char *label = luaL_checkstring(L, 1);
  lua_pushboolean(L, ImGui::Button(label));
  return 1;
}

int lua_ui_checkbox(lua_State *L) {
  const char *label = luaL_checkstring(L, 1);
  bool val = lua_toboolean(L, 2);
  if (ImGui::Checkbox(label, &val)) {
    lua_pushboolean(L, true);
    lua_pushboolean(L, val);
    return 2;
  }
  lua_pushboolean(L, false);
  return 1;
}

int lua_ui_slider(lua_State *L) {
  const char *label = luaL_checkstring(L, 1);
  float val = (float)luaL_checknumber(L, 2);
  float min = (float)luaL_checknumber(L, 3);
  float max = (float)luaL_checknumber(L, 4);
  if (ImGui::SliderFloat(label, &val, min, max)) {
    lua_pushboolean(L, true);
    lua_pushnumber(L, val);
    return 2;
  }
  lua_pushboolean(L, false);
  return 1;
}

int lua_ui_same_line(lua_State *L) {
  ImGui::SameLine();
  return 0;
}

// Engine Bindings
int lua_engine_get_local_player(lua_State *L) {
  lua_pushinteger(L, I::EngineClient->GetLocalPlayer());
  return 1;
}

int lua_engine_get_view_angles(lua_State *L) {
  Vec3 va = I::EngineClient->GetViewAngles();
  lua_pushnumber(L, va.x);
  lua_pushnumber(L, va.y);
  lua_pushnumber(L, va.z);
  return 3;
}

int lua_engine_set_view_angles(lua_State *L) {
  float x = (float)luaL_checknumber(L, 1);
  float y = (float)luaL_checknumber(L, 2);
  float z = (float)lua_gettop(L) >= 3 ? (float)luaL_checknumber(L, 3) : 0.f;
  QAngle va = {x, y, z};
  I::EngineClient->SetViewAngles(va);
  return 0;
}

int lua_engine_is_in_game(lua_State *L) {
  lua_pushboolean(L, I::EngineClient->IsInGame());
  return 1;
}

int lua_engine_is_connected(lua_State *L) {
  lua_pushboolean(L, I::EngineClient->IsConnected());
  return 1;
}

int lua_engine_execute_cmd(lua_State *L) {
  const char *cmd = luaL_checkstring(L, 1);
  I::EngineClient->ExecuteClientCmd(cmd);
  return 0;
}

int lua_print(lua_State *L) {
  int n = lua_gettop(L);
  std::string s;
  for (int i = 1; i <= n; i++) {
    const char *str = lua_tolstring(L, i, nullptr);
    if (str)
      s += str;
    if (i < n)
      s += " ";
  }
  SDK::Output("Lua", s.c_str(), {100, 200, 255, 255});
  return 0;
}

CLua::CLua() {
  m_sScriptsPath = fs::current_path().string() + "\\Amalgam V2\\Scripts\\";
  m_pState = luaL_newstate();
  if (m_pState) {
    luaL_openlibs(m_pState);
    RegisterFunctions();
    RefreshScripts();
  }
}

CLua::~CLua() {
  if (m_pState)
    lua_close(m_pState);
}

void CLua::RefreshScripts() {
  m_vScripts.clear();

  if (!fs::is_directory(m_sScriptsPath))
    fs::create_directories(m_sScriptsPath);

  try {
    for (const auto &entry : fs::directory_iterator(m_sScriptsPath)) {
      if (entry.is_regular_file() && entry.path().extension() == ".lua") {
        std::string sName = entry.path().filename().string();
        m_vScripts.push_back(sName);
        if (m_mActiveScripts.find(sName) == m_mActiveScripts.end())
          m_mActiveScripts[sName] = false;
      }
    }
  } catch (...) {
  }
}

void CLua::OpenDirectory() {
  ShellExecuteA(NULL, "open", m_sScriptsPath.c_str(), NULL, NULL,
                SW_SHOWDEFAULT);
}

void CLua::LoadScript(std::string sName) {
  if (sName.empty() || m_mActiveScripts[sName])
    return;

  std::ifstream scriptFile(m_sScriptsPath + "\\" + sName);
  if (!scriptFile.is_open()) {
    SDK::Output("Lua", (std::string("Failed to open script: ") + sName).c_str(),
                {255, 100, 100, 255});
    return;
  }

  std::stringstream scriptBuffer;
  scriptBuffer << scriptFile.rdbuf();

  m_mActiveScripts[sName] = true;
  RunScript(sName, scriptBuffer.str());
}

void CLua::UnloadScript(std::string sName) {
  if (sName.empty() || !m_mActiveScripts[sName])
    return;

  m_mActiveScripts[sName] = false;
  SDK::Output("Lua", (std::string("Unloaded script: ") + sName).c_str(),
              {255, 200, 100, 255});
}

void CLua::RunScript(std::string sName, const std::string &sScript) {
  if (!m_pState)
    return;

  // 1. Create a new table for the script environment
  lua_newtable(m_pState);

  // 2. Set its __index to _G (the global table)
  lua_pushvalue(m_pState, LUA_GLOBALSINDEX);
  lua_setfield(m_pState, -2, "__index");
  lua_pushvalue(m_pState, -1); // Duplicate the new table
  lua_setmetatable(m_pState, -2);

  // 3. Store the environment in the registry for later use
  lua_pushvalue(m_pState, -1);
  m_mScriptEnvs[sName] = luaL_ref(m_pState, LUA_REGISTRYINDEX);

  // 4. Load the script
  if (luaL_loadbuffer(m_pState, sScript.c_str(), sScript.size(),
                      sName.c_str())) {
    SDK::Output("Lua", lua_tolstring(m_pState, -1, nullptr),
                {255, 100, 100, 255});
    lua_pop(m_pState, 1);
    return;
  }

  // 5. Set the script's environment to our new table
  lua_pushvalue(m_pState, -2); // push the table
  lua_setfenv(m_pState, -2);   // pops table

  // 6. Run the script!
  if (lua_pcall(m_pState, 0, 0, 0)) {
    SDK::Output("Lua", lua_tolstring(m_pState, -1, nullptr),
                {255, 100, 100, 255});
    lua_pop(m_pState, 1);
  }
}

void CLua::OnRender() {
  if (!m_pState)
    return;

  for (auto it = m_mActiveScripts.begin(); it != m_mActiveScripts.end(); ++it) {
    if (!it->second)
      continue;

    int envIdx = m_mScriptEnvs[it->first];
    if (envIdx <= 0)
      continue;

    // Access the script's environment table from registry
    lua_rawgeti(m_pState, LUA_REGISTRYINDEX, envIdx);
    if (lua_istable(m_pState, -1)) {
      lua_getfield(m_pState, -1, "OnRender");
      if (lua_isfunction(m_pState, -1)) {
        if (lua_pcall(m_pState, 0, 0, 0)) {
          SDK::Output("Lua", lua_tolstring(m_pState, -1, nullptr),
                      {255, 100, 100, 255});
          lua_pop(m_pState, 1);
        }
      } else {
        lua_pop(m_pState, 1); // pop nil
      }
    }
    lua_pop(m_pState, 1); // pop env table
  }
}

void CLua::RegisterFunctions() {
  if (!m_pState)
    return;

  // Global print
  lua_pushcclosure(m_pState, lua_print, 0);
  lua_setfield(m_pState, LUA_GLOBALSINDEX, "print");

  // Math Table
  lua_newtable(m_pState);
  {
    lua_pushcfunction(m_pState, lua_math_floor);
    lua_setfield(m_pState, -2, "floor");
    lua_pushcfunction(m_pState, lua_math_sin);
    lua_setfield(m_pState, -2, "sin");
    lua_pushcfunction(m_pState, lua_math_cos);
    lua_setfield(m_pState, -2, "cos");
    lua_pushcfunction(m_pState, lua_math_abs);
    lua_setfield(m_pState, -2, "abs");
    lua_pushcfunction(m_pState, lua_math_sqrt);
    lua_setfield(m_pState, -2, "sqrt");
    lua_pushcfunction(m_pState, lua_math_min);
    lua_setfield(m_pState, -2, "min");
    lua_pushcfunction(m_pState, lua_math_max);
    lua_setfield(m_pState, -2, "max");
    lua_pushnumber(m_pState, 3.14159265358979323846);
    lua_setfield(m_pState, -2, "pi");
  }
  lua_setfield(m_pState, LUA_GLOBALSINDEX, "math");

  // Draw Table
  lua_newtable(m_pState);
  {
    lua_pushcfunction(m_pState, lua_draw_line);
    lua_setfield(m_pState, -2, "Line");
    lua_pushcfunction(m_pState, lua_draw_rect);
    lua_setfield(m_pState, -2, "Rect");
    lua_pushcfunction(m_pState, lua_draw_text);
    lua_setfield(m_pState, -2, "Text");
    lua_pushcfunction(m_pState, lua_draw_get_screen_size);
    lua_setfield(m_pState, -2, "GetScreenSize");
  }
  lua_setfield(m_pState, LUA_GLOBALSINDEX, "draw");

  // Globals Table
  lua_newtable(m_pState);
  {
    lua_pushcfunction(m_pState, lua_globals_get_time);
    lua_setfield(m_pState, -2, "GetTime");
    lua_pushcfunction(m_pState, lua_globals_get_fps);
    lua_setfield(m_pState, -2, "GetFPS");
    lua_pushcfunction(m_pState, lua_globals_get_realtime);
    lua_setfield(m_pState, -2, "GetRealtime");
  }
  lua_setfield(m_pState, LUA_GLOBALSINDEX, "globals");

  // Vars Table
  lua_newtable(m_pState);
  {
    lua_pushcfunction(m_pState, lua_vars_get);
    lua_setfield(m_pState, -2, "get");
    lua_pushcfunction(m_pState, lua_vars_set);
    lua_setfield(m_pState, -2, "set");
  }
  lua_setfield(m_pState, LUA_GLOBALSINDEX, "vars");

  // UI Table
  lua_newtable(m_pState);
  {
    lua_pushcfunction(m_pState, lua_ui_begin);
    lua_setfield(m_pState, -2, "Begin");
    lua_pushcfunction(m_pState, lua_ui_end);
    lua_setfield(m_pState, -2, "End");
    lua_pushcfunction(m_pState, lua_ui_text);
    lua_setfield(m_pState, -2, "Text");
    lua_pushcfunction(m_pState, lua_ui_button);
    lua_setfield(m_pState, -2, "Button");
    lua_pushcfunction(m_pState, lua_ui_checkbox);
    lua_setfield(m_pState, -2, "Checkbox");
    lua_pushcfunction(m_pState, lua_ui_slider);
    lua_setfield(m_pState, -2, "Slider");
    lua_pushcfunction(m_pState, lua_ui_same_line);
    lua_setfield(m_pState, -2, "SameLine");
  }
  lua_setfield(m_pState, LUA_GLOBALSINDEX, "ui");

  // Engine Table
  lua_newtable(m_pState);
  {
    lua_pushcfunction(m_pState, lua_engine_get_local_player);
    lua_setfield(m_pState, -2, "GetLocalPlayer");
    lua_pushcfunction(m_pState, lua_engine_get_view_angles);
    lua_setfield(m_pState, -2, "GetViewAngles");
    lua_pushcfunction(m_pState, lua_engine_set_view_angles);
    lua_setfield(m_pState, -2, "SetViewAngles");
    lua_pushcfunction(m_pState, lua_engine_is_in_game);
    lua_setfield(m_pState, -2, "IsInGame");
    lua_pushcfunction(m_pState, lua_engine_is_connected);
    lua_setfield(m_pState, -2, "IsConnected");
    lua_pushcfunction(m_pState, lua_engine_execute_cmd);
    lua_setfield(m_pState, -2, "ExecuteClientCmd");
  }
  lua_setfield(m_pState, LUA_GLOBALSINDEX, "engine");
}
