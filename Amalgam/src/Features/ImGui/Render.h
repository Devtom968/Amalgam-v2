#pragma once
#define AMALGAM_CUSTOM_FONTS
#include "../../SDK/SDK.h"
#include <imgui/imgui.h>
#include <imgui/imgui_impl_dx9.h>

class CRender {
public:
  void Render(IDirect3DDevice9 *pDevice);
  void Initialize(IDirect3DDevice9 *pDevice);

  void LoadColors();
  void LoadFonts();
  void LoadStyle();

  int Cursor = 2;

  // Colors
  ImColor Accent = {};
  ImColor Background0 = {};
  ImColor Background0p5 = {};
  ImColor Background1 = {};
  ImColor Background1p5 = {};
  ImColor Background1p5L = {};
  ImColor Background2 = {};
  ImColor Inactive = {};
  ImColor Active = {};

  // Fonts
  ImFont *FontSmall = nullptr;
  ImFont *FontRegular = nullptr;
  ImFont *FontBold = nullptr;
  ImFont *FontLarge = nullptr;
  ImFont *FontMono = nullptr;

  ImFont *IconFont = nullptr;
};

ADD_FEATURE(CRender, Render);