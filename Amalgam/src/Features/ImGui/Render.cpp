#include "Render.h"

#include "../../Hooks/Direct3DDevice9.h"
#include "../Lua/Lua.h"
#include "Fonts/CascadiaMono/CascadiaMono.h"
#include "Fonts/MaterialDesign/IconDefinitions.h"
#include "Fonts/MaterialDesign/MaterialIcons.h"
#include "Fonts/Roboto/RobotoBlack.h"
#include "Fonts/Roboto/RobotoMedium.h"
#include "Menu/Menu.h"
#include <imgui/imgui_impl_win32.h>

void CRender::Render(IDirect3DDevice9 *pDevice) {
  using namespace ImGui;

  static std::once_flag initFlag;
  std::call_once(initFlag, [&] { Initialize(pDevice); });

  LoadColors();
  {
    static float flStaticScale = Vars::Menu::Scale.Value;
    float flOldScale = flStaticScale;
    float flNewScale = flStaticScale = Vars::Menu::Scale.Value;
    if (flNewScale != flOldScale) {
      LoadFonts();
      LoadStyle();
    }
  }

  DWORD dwOldRGB;
  pDevice->GetRenderState(D3DRS_SRGBWRITEENABLE, &dwOldRGB);
  pDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, false);
  ImGui_ImplDX9_NewFrame();
  ImGui_ImplWin32_NewFrame();
  NewFrame();

  F::Menu.Render();
  F::Lua.OnRender();

  EndFrame();
  ImGui::Render();
  ImGui_ImplDX9_RenderDrawData(GetDrawData());
  pDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, dwOldRGB);
}

void CRender::LoadColors() {
  using namespace ImGui;

  auto ColorToVec = [](Color_t tColor) -> ImColor {
    return {tColor.r / 255.f, tColor.g / 255.f, tColor.b / 255.f,
            tColor.a / 255.f};
  };

  Accent = ColorToVec(Vars::Menu::Theme::Accent.Value);

  // Modern sleek dark theme
  Background0 = {0.07f, 0.08f, 0.10f, 0.97f};
  Background0p5 = {0.09f, 0.10f, 0.12f, 0.97f};
  Background1 = {0.11f, 0.12f, 0.15f, 1.00f};
  Background1p5 = {0.14f, 0.15f, 0.19f, 1.00f};
  Background1p5L = {0.17f, 0.18f, 0.22f, 1.00f};
  Background2 = {0.21f, 0.23f, 0.28f, 1.00f};

  Inactive = {0.6f, 0.6f, 0.65f, 1.0f};
  Active = {0.95f, 0.95f, 0.95f, 1.0f};

  ImVec4 *colors = GetStyle().Colors;
  colors[ImGuiCol_Border] = Background2;
  colors[ImGuiCol_Button] = {};
  colors[ImGuiCol_ButtonHovered] = {};
  colors[ImGuiCol_ButtonActive] = {};
  colors[ImGuiCol_FrameBg] = Background1p5;
  colors[ImGuiCol_FrameBgHovered] = Background1p5L;
  colors[ImGuiCol_FrameBgActive] = Background1p5;
  colors[ImGuiCol_Header] = {};
  colors[ImGuiCol_HeaderHovered] = {
      Background1p5L.Value.x * 1.1f, Background1p5L.Value.y * 1.1f,
      Background1p5L.Value.z * 1.1f, Background1p5.Value.w}; // divd by 1.1
  colors[ImGuiCol_HeaderActive] = Background1p5;
  colors[ImGuiCol_ModalWindowDimBg] = {Background0.Value.x, Background0.Value.y,
                                       Background0.Value.z, 0.4f};
  colors[ImGuiCol_PopupBg] = Background1p5L;
  colors[ImGuiCol_ResizeGrip] = {};
  colors[ImGuiCol_ResizeGripActive] = {};
  colors[ImGuiCol_ResizeGripHovered] = {};
  colors[ImGuiCol_ScrollbarBg] = {};
  colors[ImGuiCol_Text] = Active;
  colors[ImGuiCol_WindowBg] = {};
}

void CRender::LoadFonts() {
  static bool bHasLoaded = false;

  auto &io = ImGui::GetIO();
  if (bHasLoaded) {
    ImGui_ImplDX9_InvalidateDeviceObjects();
    io.Fonts->ClearFonts();
  }

  ImFontConfig fontConfig;
  fontConfig.OversampleH = 2;
  constexpr ImWchar fontRange[]{
      0x0020, 0x00FF, 0x0400, 0x044F,
      0}; // Basic Latin, Latin Supplement and Cyrillic
#ifndef AMALGAM_CUSTOM_FONTS
  FontSmall =
      io.Fonts->AddFontFromFileTTF(R"(C:\Windows\Fonts\verdana.ttf)",
                                   H::Draw.Scale(11), &fontConfig, fontRange);
  FontRegular =
      io.Fonts->AddFontFromFileTTF(R"(C:\Windows\Fonts\verdana.ttf)",
                                   H::Draw.Scale(13), &fontConfig, fontRange);
  FontBold =
      io.Fonts->AddFontFromFileTTF(R"(C:\Windows\Fonts\verdanab.ttf)",
                                   H::Draw.Scale(13), &fontConfig, fontRange);
  FontLarge =
      io.Fonts->AddFontFromFileTTF(R"(C:\Windows\Fonts\verdana.ttf)",
                                   H::Draw.Scale(14), &fontConfig, fontRange);
  FontMono = io.Fonts->AddFontFromFileTTF(
      R"(C:\Windows\Fonts\cour.ttf)", H::Draw.Scale(16), &fontConfig,
      fontRange); // windows mono font installed by default
#else
  FontSmall = io.Fonts->AddFontFromMemoryCompressedTTF(
      RobotoMedium_compressed_data, RobotoMedium_compressed_size,
      H::Draw.Scale(12), &fontConfig, fontRange);
  FontRegular = io.Fonts->AddFontFromMemoryCompressedTTF(
      RobotoMedium_compressed_data, RobotoMedium_compressed_size,
      H::Draw.Scale(13), &fontConfig, fontRange);
  FontBold = io.Fonts->AddFontFromMemoryCompressedTTF(
      RobotoBlack_compressed_data, RobotoBlack_compressed_size,
      H::Draw.Scale(13), &fontConfig, fontRange);
  FontLarge = io.Fonts->AddFontFromMemoryCompressedTTF(
      RobotoMedium_compressed_data, RobotoMedium_compressed_size,
      H::Draw.Scale(15), &fontConfig, fontRange);
  FontMono = io.Fonts->AddFontFromMemoryCompressedTTF(
      CascadiaMono_compressed_data, CascadiaMono_compressed_size,
      H::Draw.Scale(15), &fontConfig, fontRange);
#endif

  ImFontConfig iconConfig;
  iconConfig.PixelSnapH = true;
  constexpr ImWchar iconRange[]{short(ICON_MIN_MD), short(ICON_MAX_MD), 0};
  IconFont = io.Fonts->AddFontFromMemoryCompressedTTF(
      MaterialIcons_compressed_data, MaterialIcons_compressed_size,
      H::Draw.Scale(16), &iconConfig, iconRange);

  io.Fonts->Build();
  io.ConfigDebugHighlightIdConflicts = false;

  bHasLoaded = true;
}

void CRender::LoadStyle() {
  using namespace ImGui;

  auto &style = GetStyle();
  style.ButtonTextAlign = {0.5f, 0.5f};  // Center button text
  style.WindowTitleAlign = {0.5f, 0.5f}; // Center title text
  style.CellPadding = {H::Draw.Scale(6), H::Draw.Scale(4)};
  style.WindowPadding = {H::Draw.Scale(10), H::Draw.Scale(10)};
  style.WindowRounding = H::Draw.Scale(8);
  style.FramePadding = ImVec2(H::Draw.Scale(8), H::Draw.Scale(5));
  style.FrameRounding = H::Draw.Scale(4);
  style.ItemSpacing = ImVec2(H::Draw.Scale(8), H::Draw.Scale(6));
  style.ItemInnerSpacing = ImVec2(H::Draw.Scale(6), H::Draw.Scale(4));
  style.IndentSpacing = H::Draw.Scale(20);
  style.ScrollbarSize = H::Draw.Scale(10);
  style.ScrollbarRounding = H::Draw.Scale(4);
  style.GrabMinSize = H::Draw.Scale(12);
  style.GrabRounding = H::Draw.Scale(4);
  style.TabRounding = H::Draw.Scale(4);
  style.ChildRounding = H::Draw.Scale(6);
  style.PopupRounding = H::Draw.Scale(6);
  style.WindowBorderSize = 1.0f;
  style.ChildBorderSize = 1.0f;
  style.PopupBorderSize = 1.0f;
  style.FrameBorderSize = 0.0f;
  style.Alpha = 0.98f;
}

void CRender::Initialize(IDirect3DDevice9 *pDevice) {
  // Initialize ImGui and device
  ImGui::CreateContext();
  ImGui_ImplWin32_Init(WndProc::hwWindow);
  ImGui_ImplDX9_Init(pDevice);

  auto &io = ImGui::GetIO();
  // io.IniFilename = nullptr;
  io.LogFilename = nullptr;

  LoadFonts();
  LoadStyle();
}