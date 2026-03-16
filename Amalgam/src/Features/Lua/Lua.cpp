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
#include <map>

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

#include <filesystem>
namespace fs = std::filesystem;

#define static_lua
#include "minilua.c"

#ifndef LUA_REGISTRYINDEX
#define LUA_REGISTRYINDEX (-10000)
#endif
#ifndef LUA_GLOBALSINDEX
#define LUA_GLOBALSINDEX (-10002)
#endif

#ifndef luaL_optnumber
#define luaL_optnumber(L,n,d) (lua_gettop(L) >= (n) ? lua_tonumber(L, (n)) : (d))
#endif

#ifndef lua_pushlightuserdata
static void lua_pushlightuserdata(lua_State* L, void* p) {
    TValue* top = L->top;
    top->value.p = p;
    top->tt = 2; // LUA_TLIGHTUSERDATA
    L->top++;
}
#endif

// --- Internal Helpers ---
static int lua_absindex(lua_State *L, int i) {
    if (i > 0 || i <= LUA_REGISTRYINDEX)
        return i;
    return lua_gettop(L) + i + 1;
}

int luaL_ref(lua_State *L, int t) {
    int ref;
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return -1;
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

static ImU32 GetImColor(lua_State *L, int idx) {
    if (lua_istable(L, idx)) {
        lua_getfield(L, idx, "r");
        int r = lua_isnumber(L, -1) ? (int)lua_tointeger(L, -1) : 255;
        lua_getfield(L, idx, "g");
        int g = lua_isnumber(L, -1) ? (int)lua_tointeger(L, -1) : 255;
        lua_getfield(L, idx, "b");
        int b = lua_isnumber(L, -1) ? (int)lua_tointeger(L, -1) : 255;
        lua_getfield(L, idx, "a");
        int a = lua_isnumber(L, -1) ? (int)lua_tointeger(L, -1) : 255;
        lua_pop(L, 4);
        return IM_COL32(r, g, b, a);
    }
    else if (lua_isnumber(L, idx)) {
        return (ImU32)lua_tointeger(L, idx);
    }
    return IM_COL32(255, 255, 255, 255);
}

static int lua_print(lua_State* L) {
    int n = lua_gettop(L);
    std::string out = "";
    for (int i = 1; i <= n; i++) {
        const char* s = lua_tostring(L, i);
        if (s) {
            if (i > 1) out += "    ";
            out += s;
        }
    }
    SDK::Output("Lua", out.c_str(), { 255, 255, 255, 255 }, OUTPUT_CONSOLE);
    return 0;
}

static int lua_error_handler(lua_State* L) {
    const char* err = lua_tostring(L, 1);
    if (err) {
        SDK::Output("Lua Error", err, { 255, 50, 50, 255 }, OUTPUT_CONSOLE);
    }
    return 0;
}

// --- Vector Object ---
static int lua_vector_new(lua_State *L) {
    float x = (float)luaL_optnumber(L, 1, 0.0);
    float y = (float)luaL_optnumber(L, 2, 0.0);
    float z = (float)luaL_optnumber(L, 3, 0.0);
    lua_newtable(L);
    lua_pushnumber(L, x); lua_setfield(L, -2, "x");
    lua_pushnumber(L, y); lua_setfield(L, -2, "y");
    lua_pushnumber(L, z); lua_setfield(L, -2, "z");
    luaL_getmetatable(L, "Vector");
    lua_setmetatable(L, -2);
    return 1;
}

static int lua_vector_add(lua_State *L) {
    lua_getfield(L, 1, "x"); float x1 = (float)lua_tonumber(L, -1);
    lua_getfield(L, 1, "y"); float y1 = (float)lua_tonumber(L, -1);
    lua_getfield(L, 1, "z"); float z1 = (float)lua_tonumber(L, -1);
    lua_getfield(L, 2, "x"); float x2 = (float)lua_tonumber(L, -1);
    lua_getfield(L, 2, "y"); float y2 = (float)lua_tonumber(L, -1);
    lua_getfield(L, 2, "z"); float z2 = (float)lua_tonumber(L, -1);
    lua_pop(L, 6);
    lua_pushcfunction(L, lua_vector_new);
    lua_pushnumber(L, x1 + x2);
    lua_pushnumber(L, y1 + y2);
    lua_pushnumber(L, z1 + z2);
    lua_call(L, 3, 1);
    return 1;
}

static int lua_vector_sub(lua_State *L) {
    lua_getfield(L, 1, "x"); float x1 = (float)lua_tonumber(L, -1);
    lua_getfield(L, 1, "y"); float y1 = (float)lua_tonumber(L, -1);
    lua_getfield(L, 1, "z"); float z1 = (float)lua_tonumber(L, -1);
    lua_getfield(L, 2, "x"); float x2 = (float)lua_tonumber(L, -1);
    lua_getfield(L, 2, "y"); float y2 = (float)lua_tonumber(L, -1);
    lua_getfield(L, 2, "z"); float z2 = (float)lua_tonumber(L, -1);
    lua_pop(L, 6);
    lua_pushcfunction(L, lua_vector_new);
    lua_pushnumber(L, x1 - x2);
    lua_pushnumber(L, y1 - y2);
    lua_pushnumber(L, z1 - z2);
    lua_call(L, 3, 1);
    return 1;
}

static int lua_vector_mul(lua_State *L) {
    lua_getfield(L, 1, "x"); float x1 = (float)lua_tonumber(L, -1);
    lua_getfield(L, 1, "y"); float y1 = (float)lua_tonumber(L, -1);
    lua_getfield(L, 1, "z"); float z1 = (float)lua_tonumber(L, -1);
    float m = (float)luaL_checknumber(L, 2);
    lua_pop(L, 3);
    lua_pushcfunction(L, lua_vector_new);
    lua_pushnumber(L, x1 * m);
    lua_pushnumber(L, y1 * m);
    lua_pushnumber(L, z1 * m);
    lua_call(L, 3, 1);
    return 1;
}

// --- Entity Object ---
static CBaseEntity* GetEntityFromLua(lua_State *L, int idx) {
    lua_getfield(L, idx, "__ptr");
    void* ptr = lua_touserdata(L, -1);
    lua_pop(L, 1);
    return (CBaseEntity*)ptr;
}

static int lua_entity_get_index(lua_State *L) {
    auto pEnt = GetEntityFromLua(L, 1);
    if (!pEnt) return 0;
    lua_pushinteger(L, pEnt->entindex());
    return 1;
}

static int lua_entity_get_class_id(lua_State *L) {
    auto pEnt = GetEntityFromLua(L, 1);
    if (!pEnt) return 0;
    lua_pushinteger(L, (int)pEnt->GetClassID());
    return 1;
}

static int lua_entity_get_team(lua_State *L) {
    auto pEnt = GetEntityFromLua(L, 1);
    if (!pEnt) return 0;
    lua_pushinteger(L, pEnt->m_iTeamNum());
    return 1;
}

static int lua_entity_get_health(lua_State *L) {
    auto pEnt = GetEntityFromLua(L, 1);
    if (!pEnt) return 0;
    if (pEnt->IsPlayer()) lua_pushinteger(L, pEnt->As<CBasePlayer>()->m_iHealth());
    else if (pEnt->IsBaseObject()) lua_pushinteger(L, pEnt->As<CBaseObject>()->m_iHealth());
    else lua_pushinteger(L, 0);
    return 1;
}

static int lua_entity_get_max_health(lua_State *L) {
    auto pEnt = GetEntityFromLua(L, 1);
    if (!pEnt) return 0;
    if (pEnt->IsPlayer()) lua_pushinteger(L, pEnt->As<CTFPlayer>()->GetMaxHealth());
    else if (pEnt->IsBaseObject()) lua_pushinteger(L, pEnt->As<CBaseObject>()->m_iMaxHealth());
    else lua_pushinteger(L, 0);
    return 1;
}

static int lua_entity_get_origin(lua_State *L) {
    auto pEnt = GetEntityFromLua(L, 1);
    if (!pEnt) return 0;
    Vec3 origin = pEnt->m_vecOrigin();
    lua_pushcfunction(L, lua_vector_new);
    lua_pushnumber(L, origin.x);
    lua_pushnumber(L, origin.y);
    lua_pushnumber(L, origin.z);
    lua_call(L, 3, 1);
    return 1;
}

static int lua_entity_is_alive(lua_State *L) {
    auto pEnt = GetEntityFromLua(L, 1);
    if (!pEnt) return 0;
    int health = 0;
    if (pEnt->IsPlayer()) health = pEnt->As<CBasePlayer>()->m_iHealth();
    else if (pEnt->IsBaseObject()) health = pEnt->As<CBaseObject>()->m_iHealth();
    lua_pushboolean(L, health > 0);
    return 1;
}

static int lua_entity_is_dormant(lua_State *L) {
    auto pEnt = GetEntityFromLua(L, 1);
    if (!pEnt) return 0;
    lua_pushboolean(L, pEnt->IsDormant());
    return 1;
}

static int lua_entity_get_name(lua_State *L) {
    auto pEnt = GetEntityFromLua(L, 1);
    if (!pEnt || !pEnt->IsPlayer()) return 0;
    player_info_t pi;
    if (I::EngineClient->GetPlayerInfo(pEnt->entindex(), &pi)) {
        lua_pushstring(L, pi.name);
        return 1;
    }
    return 0;
}

static int lua_entity_get_netvar(lua_State* L) {
    auto pEnt = GetEntityFromLua(L, 1);
    if (!pEnt) return 0;
    const char* table = luaL_checkstring(L, 2);
    const char* var = luaL_checkstring(L, 3);
    const char* type = luaL_optstring(L, 4, "int");

    int offset = U::NetVars.GetNetVar(table, var);
    if (!offset) return 0;

    void* addr = (void*)(uintptr_t(pEnt) + offset);
    if (strcmp(type, "int") == 0) lua_pushinteger(L, *(int*)addr);
    else if (strcmp(type, "float") == 0) lua_pushnumber(L, *(float*)addr);
    else if (strcmp(type, "bool") == 0) lua_pushboolean(L, *(bool*)addr);
    else if (strcmp(type, "string") == 0) lua_pushstring(L, (const char*)addr);
    else if (strcmp(type, "vector") == 0) {
        Vec3 v = *(Vec3*)addr;
        lua_pushcfunction(L, lua_vector_new);
        lua_pushnumber(L, v.x);
        lua_pushnumber(L, v.y);
        lua_pushnumber(L, v.z);
        lua_call(L, 3, 1);
    }
    else return 0;

    return 1;
}

static int PushEntity(lua_State *L, CBaseEntity* pEnt) {
    if (!pEnt) { lua_pushnil(L); return 1; }
    lua_newtable(L);
    lua_pushlightuserdata(L, pEnt); lua_setfield(L, -2, "__ptr");
    luaL_getmetatable(L, "Entity");
    lua_setmetatable(L, -2);
    return 1;
}

// --- Entities Table ---
static int lua_entities_get_local(lua_State *L) {
    return PushEntity(L, (CBaseEntity*)H::Entities.GetLocal());
}

static int lua_entities_get_by_index(lua_State *L) {
    int idx = (int)luaL_checkinteger(L, 1);
    return PushEntity(L, I::ClientEntityList->GetClientEntity(idx)->As<CBaseEntity>());
}

static int lua_entities_get_players(lua_State *L) {
    lua_newtable(L);
    int i = 1;
    for (auto pEnt : H::Entities.GetGroup(EntityEnum::PlayerAll)) {
        PushEntity(L, (CBaseEntity*)pEnt);
        lua_rawseti(L, -2, i++);
    }
    return 1;
}

// --- Draw Table ---
static int lua_draw_line(lua_State *L) {
    float x1 = (float)luaL_checknumber(L, 1);
    float y1 = (float)luaL_checknumber(L, 2);
    float x2 = (float)luaL_checknumber(L, 3);
    float y2 = (float)luaL_checknumber(L, 4);
    ImU32 color = GetImColor(L, 5);
    ImGui::GetBackgroundDrawList()->AddLine({ x1, y1 }, { x2, y2 }, color);
    return 0;
}

static int lua_draw_rect(lua_State *L) {
    float x = (float)luaL_checknumber(L, 1);
    float y = (float)luaL_checknumber(L, 2);
    float w = (float)luaL_checknumber(L, 3);
    float h = (float)luaL_checknumber(L, 4);
    ImU32 color = GetImColor(L, 5);
    bool filled = lua_toboolean(L, 6);
    if (filled) ImGui::GetBackgroundDrawList()->AddRectFilled({ x, y }, { x + w, y + h }, color);
    else ImGui::GetBackgroundDrawList()->AddRect({ x, y }, { x + w, y + h }, color);
    return 0;
}

static int lua_draw_text(lua_State *L) {
    float x = (float)luaL_checknumber(L, 1);
    float y = (float)luaL_checknumber(L, 2);
    const char* text = luaL_checkstring(L, 3);
    ImU32 color = GetImColor(L, 4);
    bool center = lua_toboolean(L, 5);
    if (center) {
        ImVec2 size = ImGui::CalcTextSize(text);
        ImGui::GetBackgroundDrawList()->AddText({ x - size.x / 2.f, y }, color, text);
    } else {
        ImGui::GetBackgroundDrawList()->AddText({ x, y }, color, text);
    }
    return 0;
}

// --- Engine Table ---
static int lua_engine_get_local_player(lua_State *L) {
    lua_pushinteger(L, I::EngineClient->GetLocalPlayer());
    return 1;
}

static int lua_engine_is_in_game(lua_State *L) {
    lua_pushboolean(L, I::EngineClient->IsInGame());
    return 1;
}

static int lua_engine_is_connected(lua_State *L) {
    lua_pushboolean(L, I::EngineClient->IsConnected());
    return 1;
}

static int lua_engine_execute_cmd(lua_State *L) {
    const char* cmd = luaL_checkstring(L, 1);
    I::EngineClient->ExecuteClientCmd(cmd);
    return 0;
}

static int lua_engine_get_screen_size(lua_State *L) {
    lua_pushinteger(L, H::Draw.m_nScreenW);
    lua_pushinteger(L, H::Draw.m_nScreenH);
    return 2;
}

static int lua_engine_get_view_angles(lua_State *L) {
    Vec3 va;
    I::EngineClient->GetViewAngles(va);
    lua_pushnumber(L, va.x);
    lua_pushnumber(L, va.y);
    lua_pushnumber(L, va.z);
    return 3;
}

static int lua_engine_set_view_angles(lua_State *L) {
    Vec3 va;
    va.x = (float)luaL_checknumber(L, 1);
    va.y = (float)luaL_checknumber(L, 2);
    va.z = (float)luaL_optnumber(L, 3, 0.0);
    I::EngineClient->SetViewAngles(va);
    return 0;
}

// --- Input Table ---
static int lua_input_is_key_down(lua_State *L) {
    int key = (int)luaL_checkinteger(L, 1);
    lua_pushboolean(L, U::KeyHandler.Down(key));
    return 1;
}

// --- Vars Table ---
static int lua_vars_get(lua_State* L) {
    const char* section = luaL_checkstring(L, 1);
    const char* name = luaL_checkstring(L, 2);
    for (auto v : G::Vars) {
        if (strcmp(v->Section(), section) == 0 && strcmp(v->Name(), name) == 0) {
            if (v->m_iType == typeid(int).hash_code()) lua_pushinteger(L, v->As<int>()->Value);
            else if (v->m_iType == typeid(float).hash_code()) lua_pushnumber(L, v->As<float>()->Value);
            else if (v->m_iType == typeid(bool).hash_code()) lua_pushboolean(L, v->As<bool>()->Value);
            else if (v->m_iType == typeid(std::string).hash_code()) lua_pushstring(L, v->As<std::string>()->Value.c_str());
            else return 0;
            return 1;
        }
    }
    return 0;
}

static int lua_vars_set(lua_State* L) {
    const char* section = luaL_checkstring(L, 1);
    const char* name = luaL_checkstring(L, 2);
    for (auto v : G::Vars) {
        if (strcmp(v->Section(), section) == 0 && strcmp(v->Name(), name) == 0) {
            if (v->m_iType == typeid(int).hash_code()) v->As<int>()->Value = (int)lua_tointeger(L, 3);
            else if (v->m_iType == typeid(float).hash_code()) v->As<float>()->Value = (float)lua_tonumber(L, 3);
            else if (v->m_iType == typeid(bool).hash_code()) v->As<bool>()->Value = lua_toboolean(L, 3);
            else if (v->m_iType == typeid(std::string).hash_code()) v->As<std::string>()->Value = lua_tostring(L, 3);
            return 0;
        }
    }
    return 0;
}

// --- UI Table ---
static int lua_ui_begin(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    bool* pOpen = nullptr;
    bool open = true;
    if (lua_gettop(L) >= 2) {
        open = lua_toboolean(L, 2);
        pOpen = &open;
    }
    bool result = ImGui::Begin(name, pOpen);
    lua_pushboolean(L, result);
    if (pOpen) lua_pushboolean(L, open);
    else lua_pushnil(L);
    return 2;
}

static int lua_ui_end(lua_State* L) {
    ImGui::End();
    return 0;
}

static int lua_ui_text(lua_State* L) {
    const char* text = luaL_checkstring(L, 1);
    ImGui::TextUnformatted(text);
    return 0;
}

static int lua_ui_button(lua_State* L) {
    const char* text = luaL_checkstring(L, 1);
    float w = (float)luaL_optnumber(L, 2, 0);
    float h = (float)luaL_optnumber(L, 3, 0);
    lua_pushboolean(L, ImGui::Button(text, { w, h }));
    return 1;
}

static int lua_ui_checkbox(lua_State* L) {
    const char* text = luaL_checkstring(L, 1);
    bool active = false;
    if (lua_isboolean(L, 2)) active = lua_toboolean(L, 2);
    else if (lua_isnumber(L, 2)) active = lua_tonumber(L, 2) != 0;
    
    bool pressed = ImGui::Checkbox(text, &active);
    lua_pushboolean(L, pressed);
    lua_pushboolean(L, active);
    return 2;
}

static int lua_ui_slider(lua_State* L) {
    const char* text = luaL_checkstring(L, 1);
    float val = 0.0f;
    if (lua_isnumber(L, 2)) val = (float)lua_tonumber(L, 2);
    
    float min = (float)luaL_optnumber(L, 3, 0.0f);
    float max = (float)luaL_optnumber(L, 4, 100.0f);
    
    bool pressed = ImGui::SliderFloat(text, &val, min, max);
    lua_pushboolean(L, pressed);
    lua_pushnumber(L, val);
    return 2;
}

static int lua_ui_same_line(lua_State* L) {
    ImGui::SameLine();
    return 0;
}

// --- Callbacks Table ---
static std::map<std::string, std::map<std::string, int>> m_mCallbacks;

static int lua_callbacks_register(lua_State *L) {
    const char* callback = luaL_checkstring(L, 1);
    const char* id = luaL_checkstring(L, 2);
    if (lua_isfunction(L, 3)) {
        lua_pushvalue(L, 3);
        int ref = luaL_ref(L, LUA_REGISTRYINDEX);
        m_mCallbacks[callback][id] = ref;
    }
    return 0;
}

static int lua_callbacks_unregister(lua_State *L) {
    const char* callback = luaL_checkstring(L, 1);
    const char* id = luaL_checkstring(L, 2);
    if (m_mCallbacks.count(callback) && m_mCallbacks[callback].count(id)) {
        luaL_unref(L, LUA_REGISTRYINDEX, m_mCallbacks[callback][id]);
        m_mCallbacks[callback].erase(id);
    }
    return 0;
}

// --- CCore Implementation ---
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
    if (m_pState) {
        for (auto& cb : m_mCallbacks) {
            for (auto& pair : cb.second) {
                luaL_unref(m_pState, LUA_REGISTRYINDEX, pair.second);
            }
        }
        lua_close(m_pState);
    }
}

void CLua::RegisterFunctions() {
    // Vector Metatable
    luaL_newmetatable(m_pState, "Vector");
    lua_pushcfunction(m_pState, lua_vector_add); lua_setfield(m_pState, -2, "__add");
    lua_pushcfunction(m_pState, lua_vector_sub); lua_setfield(m_pState, -2, "__sub");
    lua_pushcfunction(m_pState, lua_vector_mul); lua_setfield(m_pState, -2, "__mul");
    lua_pushvalue(m_pState, -1); lua_setfield(m_pState, -2, "__index");
    lua_pop(m_pState, 1);

    // Entity Metatable
    luaL_newmetatable(m_pState, "Entity");
    lua_newtable(m_pState);
    lua_pushcfunction(m_pState, lua_entity_get_index); lua_setfield(m_pState, -2, "GetIndex");
    lua_pushcfunction(m_pState, lua_entity_get_class_id); lua_setfield(m_pState, -2, "GetClassID");
    lua_pushcfunction(m_pState, lua_entity_get_team); lua_setfield(m_pState, -2, "GetTeam");
    lua_pushcfunction(m_pState, lua_entity_get_health); lua_setfield(m_pState, -2, "GetHealth");
    lua_pushcfunction(m_pState, lua_entity_get_max_health); lua_setfield(m_pState, -2, "GetMaxHealth");
    lua_pushcfunction(m_pState, lua_entity_get_origin); lua_setfield(m_pState, -2, "GetOrigin");
    lua_pushcfunction(m_pState, lua_entity_is_alive); lua_setfield(m_pState, -2, "IsAlive");
    lua_pushcfunction(m_pState, lua_entity_is_dormant); lua_setfield(m_pState, -2, "IsDormant");
    lua_pushcfunction(m_pState, lua_entity_get_name); lua_setfield(m_pState, -2, "GetName");
    lua_pushcfunction(m_pState, lua_entity_get_netvar); lua_setfield(m_pState, -2, "GetNetVar");
    lua_setfield(m_pState, -2, "__index");
    lua_pop(m_pState, 1);

    // Globals
    lua_pushcfunction(m_pState, lua_vector_new); lua_setglobal(m_pState, "Vector");
    lua_pushcfunction(m_pState, lua_print); lua_setglobal(m_pState, "print");
    lua_pushcfunction(m_pState, lua_error_handler); lua_setglobal(m_pState, "error");

    // Engine
    lua_newtable(m_pState);
    lua_pushcfunction(m_pState, lua_engine_get_local_player); lua_setfield(m_pState, -2, "GetLocalPlayer");
    lua_pushcfunction(m_pState, lua_engine_is_in_game); lua_setfield(m_pState, -2, "IsInGame");
    lua_pushcfunction(m_pState, lua_engine_is_connected); lua_setfield(m_pState, -2, "IsConnected");
    lua_pushcfunction(m_pState, lua_engine_execute_cmd); lua_setfield(m_pState, -2, "ExecuteClientCmd");
    lua_pushcfunction(m_pState, lua_engine_get_screen_size); lua_setfield(m_pState, -2, "GetScreenSize");
    lua_pushcfunction(m_pState, lua_engine_get_view_angles); lua_setfield(m_pState, -2, "GetViewAngles");
    lua_pushcfunction(m_pState, lua_engine_set_view_angles); lua_setfield(m_pState, -2, "SetViewAngles");
    lua_setglobal(m_pState, "engine");

    // UI
    lua_newtable(m_pState);
    lua_pushcfunction(m_pState, lua_ui_begin); lua_setfield(m_pState, -2, "Begin");
    lua_pushcfunction(m_pState, lua_ui_end); lua_setfield(m_pState, -2, "End");
    lua_pushcfunction(m_pState, lua_ui_text); lua_setfield(m_pState, -2, "Text");
    lua_pushcfunction(m_pState, lua_ui_button); lua_setfield(m_pState, -2, "Button");
    lua_pushcfunction(m_pState, lua_ui_checkbox); lua_setfield(m_pState, -2, "Checkbox");
    lua_pushcfunction(m_pState, lua_ui_slider); lua_setfield(m_pState, -2, "Slider");
    lua_pushcfunction(m_pState, lua_ui_same_line); lua_setfield(m_pState, -2, "SameLine");
    lua_setglobal(m_pState, "ui");

    // Entities
    lua_newtable(m_pState);
    lua_pushcfunction(m_pState, lua_entities_get_local); lua_setfield(m_pState, -2, "GetLocalPlayer");
    lua_pushcfunction(m_pState, lua_entities_get_by_index); lua_setfield(m_pState, -2, "GetByIndex");
    lua_pushcfunction(m_pState, lua_entities_get_players); lua_setfield(m_pState, -2, "GetPlayers");
    lua_setglobal(m_pState, "entities");

    // Draw
    lua_newtable(m_pState);
    lua_pushcfunction(m_pState, lua_draw_line); lua_setfield(m_pState, -2, "Line");
    lua_pushcfunction(m_pState, lua_draw_rect); lua_setfield(m_pState, -2, "Rect");
    lua_pushcfunction(m_pState, lua_draw_text); lua_setfield(m_pState, -2, "Text");
    lua_setglobal(m_pState, "draw");

    // Input
    lua_newtable(m_pState);
    lua_pushcfunction(m_pState, lua_input_is_key_down); lua_setfield(m_pState, -2, "IsKeyDown");
    lua_setglobal(m_pState, "input");

    // Vars
    lua_newtable(m_pState);
    lua_pushcfunction(m_pState, lua_vars_get); lua_setfield(m_pState, -2, "Get");
    lua_pushcfunction(m_pState, lua_vars_set); lua_setfield(m_pState, -2, "Set");
    lua_setglobal(m_pState, "vars");

    // Callbacks
    lua_newtable(m_pState);
    lua_pushcfunction(m_pState, lua_callbacks_register); lua_setfield(m_pState, -2, "Register");
    lua_pushcfunction(m_pState, lua_callbacks_unregister); lua_setfield(m_pState, -2, "Unregister");
    lua_setglobal(m_pState, "callbacks");
}

void CLua::RefreshScripts() {
    m_vScripts.clear();
    if (!fs::is_directory(m_sScriptsPath)) fs::create_directories(m_sScriptsPath);
    try {
        for (const auto& entry : fs::directory_iterator(m_sScriptsPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".lua") {
                std::string sName = entry.path().filename().string();
                m_vScripts.push_back(sName);
                if (m_mActiveScripts.find(sName) == m_mActiveScripts.end()) m_mActiveScripts[sName] = false;
            }
        }
    } catch (...) {}
}

void CLua::OpenDirectory() {
    ShellExecuteA(NULL, "open", m_sScriptsPath.c_str(), NULL, NULL, SW_SHOWDEFAULT);
}

void CLua::LoadScript(std::string sName) {
    if (sName.empty() || m_mActiveScripts[sName]) return;
    std::ifstream scriptFile(m_sScriptsPath + "\\" + sName);
    if (!scriptFile.is_open()) return;
    std::stringstream scriptBuffer;
    scriptBuffer << scriptFile.rdbuf();
    m_mActiveScripts[sName] = true;
    RunScript(sName, scriptBuffer.str());
}

void CLua::UnloadScript(std::string sName) {
    if (sName.empty() || !m_mActiveScripts[sName]) return;
    
    // Remove callbacks
    for (auto& cb : m_mCallbacks) {
        if (cb.second.count(sName)) {
            luaL_unref(m_pState, LUA_REGISTRYINDEX, cb.second[sName]);
            cb.second.erase(sName);
        }
    }

    m_mActiveScripts[sName] = false;
    if (m_mScriptEnvs.count(sName)) {
        luaL_unref(m_pState, LUA_REGISTRYINDEX, m_mScriptEnvs[sName]);
        m_mScriptEnvs.erase(sName);
    }
}

void CLua::RunScript(std::string sName, const std::string& sScript) {
    if (!m_pState) return;
    lua_newtable(m_pState);
    lua_pushvalue(m_pState, LUA_GLOBALSINDEX);
    lua_setfield(m_pState, -2, "__index");
    lua_pushvalue(m_pState, -1);
    lua_setmetatable(m_pState, -2);
    m_mScriptEnvs[sName] = luaL_ref(m_pState, LUA_REGISTRYINDEX);

    if (luaL_loadbuffer(m_pState, sScript.c_str(), sScript.size(), sName.c_str())) {
        SDK::Output("Lua Error", lua_tostring(m_pState, -1), { 255, 50, 50, 255 }, OUTPUT_CONSOLE);
        lua_pop(m_pState, 1);
        UnloadScript(sName);
        return;
    }
    lua_rawgeti(m_pState, LUA_REGISTRYINDEX, m_mScriptEnvs[sName]);
    lua_setfenv(m_pState, -2);
    if (lua_pcall(m_pState, 0, 0, 0)) {
        SDK::Output("Lua Error", lua_tostring(m_pState, -1), { 255, 50, 50, 255 }, OUTPUT_CONSOLE);
        lua_pop(m_pState, 1);
        UnloadScript(sName);
        return;
    }

    // Auto-register global functions
    lua_rawgeti(m_pState, LUA_REGISTRYINDEX, m_mScriptEnvs[sName]);
    const char* globals[] = { "OnRender", "CreateMove", "FrameStageNotify" };
    for (const char* g : globals) {
        lua_getfield(m_pState, -1, g);
        if (lua_isfunction(m_pState, -1)) {
            m_mCallbacks[g][sName] = luaL_ref(m_pState, LUA_REGISTRYINDEX);
        }
        else {
            lua_pop(m_pState, 1);
        }
    }
    lua_pop(m_pState, 1);
}

void CLua::OnRender() {
    if (!m_pState) return;
    auto& cbs = m_mCallbacks["OnRender"];
    for (auto& pair : cbs) {
        lua_rawgeti(m_pState, LUA_REGISTRYINDEX, pair.second);
        if (lua_pcall(m_pState, 0, 0, 0)) {
            SDK::Output("Lua Error (OnRender)", lua_tostring(m_pState, -1), { 255, 50, 50, 255 }, OUTPUT_CONSOLE);
            lua_pop(m_pState, 1);
        }
    }
}

void CLua::OnCreateMove(CUserCmd *pCmd) {
    if (!m_pState) return;
    auto& cbs = m_mCallbacks["CreateMove"];
    if (cbs.empty()) return;

    lua_newtable(m_pState);
    lua_pushnumber(m_pState, (double)pCmd->buttons); lua_setfield(m_pState, -2, "buttons");
    lua_pushnumber(m_pState, (double)pCmd->viewangles.x); lua_setfield(m_pState, -2, "viewangles_x");
    lua_pushnumber(m_pState, (double)pCmd->viewangles.y); lua_setfield(m_pState, -2, "viewangles_y");
    lua_pushnumber(m_pState, (double)pCmd->forwardmove); lua_setfield(m_pState, -2, "forwardmove");
    lua_pushnumber(m_pState, (double)pCmd->sidemove); lua_setfield(m_pState, -2, "sidemove");

    for (auto& pair : cbs) {
        lua_rawgeti(m_pState, LUA_REGISTRYINDEX, pair.second);
        lua_pushvalue(m_pState, -2);
        if (lua_pcall(m_pState, 1, 0, 0)) {
            SDK::Output("Lua Error (CreateMove)", lua_tostring(m_pState, -1), { 255, 50, 50, 255 }, OUTPUT_CONSOLE);
            lua_pop(m_pState, 1);
        }
    }

    lua_getfield(m_pState, -1, "buttons"); pCmd->buttons = (int)lua_tonumber(m_pState, -1);
    lua_getfield(m_pState, -1, "viewangles_x"); pCmd->viewangles.x = (float)lua_tonumber(m_pState, -1);
    lua_getfield(m_pState, -1, "viewangles_y"); pCmd->viewangles.y = (float)lua_tonumber(m_pState, -1);
    lua_getfield(m_pState, -1, "forwardmove"); pCmd->forwardmove = (float)lua_tonumber(m_pState, -1);
    lua_getfield(m_pState, -1, "sidemove"); pCmd->sidemove = (float)lua_tonumber(m_pState, -1);
    lua_pop(m_pState, 6);
}

void CLua::OnFrameStageNotify(ClientFrameStage_t curStage) {
    if (!m_pState) return;
    auto& cbs = m_mCallbacks["FrameStageNotify"];
    for (auto& pair : cbs) {
        lua_rawgeti(m_pState, LUA_REGISTRYINDEX, pair.second);
        lua_pushinteger(m_pState, (int)curStage);
        if (lua_pcall(m_pState, 1, 0, 0)) {
            SDK::Output("Lua Error (FrameStageNotify)", lua_tostring(m_pState, -1), { 255, 50, 50, 255 }, OUTPUT_CONSOLE);
            lua_pop(m_pState, 1);
        }
    }
}
