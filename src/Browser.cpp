/**
 * @file Browser.cpp
 * @date 3-May-2025
 */

#include <algorithm>

#include "Browser.h"

namespace
{
  static std::set<std::string> selectedSections{};

  void SetConfigSections(std::set<std::string> &sections)
  {
    selectedSections.clear();
    selectedSections.insert(sections.cbegin(), sections.cend());
  }

  orxBOOL ConfigSaveCallback(const orxSTRING _zSectionName, const orxSTRING _zKeyName, const orxSTRING _zFileName, orxBOOL _bUseEncryption)
  {
    static const std::set<std::string> KEYS_TO_DROP = {"OnPrepare", "OnCreate", "OnDelete"};
    auto sectionOk = selectedSections.contains(_zSectionName);
    return _zKeyName == orxNULL ? sectionOk : sectionOk && !KEYS_TO_DROP.contains(_zKeyName);
  }

  bool IsDirectory(orxFILE_INFO &info)
  {
    return info.u32Flags & orxFILE_KU32_FLAG_INFO_DIRECTORY;
  }

  bool IsValidDirectory(orxFILE_INFO &info)
  {
    return IsDirectory(info) && !(info.u32Flags & orxFILE_KU32_FLAG_INFO_HIDDEN) && !(orxString_SearchCharIndex(info.zName, '.', 0) == 0);
  }

  bool IsValidSoundFile(orxFILE_INFO &info)
  {
    auto extension = orxString_GetExtension(info.zName);
    return !IsDirectory(info) && extension != orxSTRING_EMPTY && orxConfig_GetBool(extension);
  }
}

void Export::OnCreate()
{
}

void Export::OnDelete()
{
}

void Export::Update(const orxCLOCK_INFO &_rstInfo)
{
  if (ImGui::Begin("Export"))
  {
    PushConfigSection();
    auto filename = orxConfig_GetString("Target");
    PopConfigSection();

    if (!selectedSections.empty() && filename != orxSTRING_EMPTY)
    {
      if (ImGui::Button("Save"))
      {
        orxConfig_Save(filename, orxFALSE, ConfigSaveCallback);
      }
    }
  }
  ImGui::End();
}

AudioDirectory::AudioDirectory(std::string rootPath)
{
  root = rootPath;
  ReadAll();
}

void AudioDirectory::ReadAll()
{
  std::string rootPattern = std::string{root} + (root.ends_with("/") ? "*" : "/*");
  orxFILE_INFO info{};
  auto status = orxFile_FindFirst(rootPattern.data(), &info);
  for (; status == orxSTATUS_SUCCESS; status = orxFile_FindNext(&info))
  {
    if (IsValidDirectory(info))
    {
      // Recurse into the directory
      subdirectories[info.zFullName] = std::make_unique<AudioDirectory>(info.zFullName);

      // Continue to next file in the search
      continue;
    }

    if (!IsValidSoundFile(info))
    {
      // Skip anything unsupported
      continue;
    }

    // It looks like a requested audio file type, so add it to config
    orxConfig_PushSection(info.zName);
    orxConfig_SetString("OnPrepare", "> @, > Get Runtime <, > Not <, return <");
    orxConfig_SetString("OnCreate", "> @, Set Runtime < true");
    orxConfig_SetString("OnDelete", "> Object.GetName ^, Set Runtime < false, return true");
    orxConfig_SetString("LifeTime", "sound");
    orxConfig_SetString("SoundList", "@");
    orxConfig_SetString("Music", info.zFullName);
    orxConfig_PopSection();
    sectionNames.push_back(std::string{info.zName});
  }
  orxFile_FindClose(&info);

  std::sort(sectionNames.begin(), sectionNames.end());
}

void AudioDirectory::Render()
{
  for (const auto &[path, directory] : subdirectories)
  {
    if (ImGui::CollapsingHeader(path.data()))
    {
      const orxFLOAT indentWidth = 5.0f;
      ImGui::Indent(indentWidth);
      directory->Render();
      ImGui::Unindent(indentWidth);
    }
  }

  std::string tableID = std::string{"Sounds table##"} + root;
  if (ImGui::BeginTable(tableID.data(), 4))
  {
    if (sectionNames.size() > 0)
    {
      ImGui::TableSetupColumn("File name");
      ImGui::TableSetupColumn("Play file");
      ImGui::TableSetupColumn("Stop playback");
      ImGui::TableSetupColumn("Select for config export");
      ImGui::TableHeadersRow();
    }

    for (auto name : sectionNames)
    {
      ImGui::PushID(name.data());
      ImGui::TableNextColumn();
      ImGui::TextUnformatted(name.data());

      ImGui::TableNextColumn();
      auto playLabel = std::string{"Play##"} + name;
      auto object = GetActiveObject(name);
      orxFLOAT hNumerator = object == orxNULL ? 1.0f : 2.0f;
      ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hNumerator / 7.0f, 0.6f, 0.6f));
      if (ImGui::Button(playLabel.data()))
      {
        auto object = orxObject_CreateFromConfig(name.data());
        if (object != orxNULL)
        {
          activeObjects[name] = orxStructure_GetGUID(orxSTRUCTURE(object));
        }
      }

      ImGui::TableNextColumn();
      auto stopLabel = std::string{"Stop##"} + name;
      if (ImGui::Button(stopLabel.data()))
      {
        auto object = GetActiveObject(name);
        if (object != orxNULL)
        {
          orxObject_SetLifeTime(object, 0);
        }
      }

      ImGui::PopStyleColor();

      ImGui::TableNextColumn();
      auto checked = selectedSections.contains(name);
      if (ImGui::Checkbox("Select", &checked))
      {
        if (checked)
        {
          selectedSections.insert(name);
        }
        else
        {
          selectedSections.erase(name);
        }
      }

      ImGui::PopID();
    }
    ImGui::EndTable();
  }
}

orxOBJECT *AudioDirectory::GetActiveObject(std::string name)
{
  orxOBJECT *object = orxNULL;

  if (activeObjects.contains(name))
  {
    auto guid = activeObjects[name];
    object = orxOBJECT(orxStructure_Get(guid));
  }

  return object;
}

void Browser::OnCreate()
{
  // Supported file formats/extensions
  const auto formatKey = "FormatList";
  for (orxS32 i = 0; i < orxConfig_GetListCount(formatKey); i++)
  {
    auto extension = orxConfig_GetListString(formatKey, i);
    orxConfig_SetBool(extension, orxTRUE);
  }
  rootPath = std::string{orxConfig_GetString("Root")};
  directory = std::make_unique<AudioDirectory>(rootPath);
}

void Browser::OnDelete()
{
}

void Browser::Update(const orxCLOCK_INFO &_rstInfo)
{
  std::string id = std::to_string(GetGUID());
  ImGui::PushID(id.data());

  std::string name = std::string{"Sounds: "} + rootPath;

  if (ImGui::Begin(name.data()))
  {
    PushConfigSection();
    directory->Render();
    PopConfigSection();
  }
  ImGui::End();

  ImGui::PopID();
}
