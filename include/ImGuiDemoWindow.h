#pragma once

#include "imgui.h"

#include "soundmanager.h"

class ImGuiDemoWindow : public ScrollObject
{
public:
protected:
  void OnCreate();
  void OnDelete();
  void Update(const orxCLOCK_INFO &_rstInfo);

private:
};
