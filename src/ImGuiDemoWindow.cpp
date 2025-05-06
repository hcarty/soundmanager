#include "ImGuiDemoWindow.h"

void ImGuiDemoWindow::OnCreate()
{
}

void ImGuiDemoWindow::OnDelete()
{
}

void ImGuiDemoWindow::Update(const orxCLOCK_INFO &_rstInfo)
{
  ImGui::ShowDemoWindow();
}
