/**
 * @file Browser.cpp
 * @date 3-May-2025
 */

#include <algorithm>
#include <cctype>
#include <charconv>
#include <optional>
#include <regex>
#include <string>

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

  std::string ToLowercase(const std::string &in)
  {
    std::string out(in);
    auto lc = [](unsigned char c) -> char
    { return std::tolower(c); };
    std::transform(out.cbegin(), out.cend(), out.begin(), lc);
    return out;
  }

  /// @brief Get the loop time offset from the pathname for an Ovani music filename
  /// @param filename The pathname of the file to parse
  /// @return The loop time offset, or std::nullopt if not found
  std::optional<double> GetLoopOffset(const std::string &filename)
  {
    static const std::regex re(R"(\(RT\s+([+-]?\d+(?:\.\d+)?)\))");
    std::smatch match;

    if (!std::regex_search(filename, match, re))
    {
      return std::nullopt;
    }

    const std::string numStr = match[1].str();

    double value = 0.0;
    const auto result = std::from_chars(
        numStr.data(),
        numStr.data() + numStr.size(),
        value);

    if (result.ec != std::errc{} || result.ptr != numStr.data() + numStr.size())
    {
      return std::nullopt;
    }

    return value;
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

AudioDirectory::AudioDirectory(const std::string &rootPath)
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

    auto loopTimeOffset = GetLoopOffset(info.zFullName).value_or(0.0f);

    AudioName audioName(info.zName, info.zPath, info.zFullName, loopTimeOffset);

    // It looks like a requested audio file type, so add it to config
    orxConfig_PushSection(audioName.sectionName.data());
    orxConfig_SetString("OnPrepare", "> @, > Get Runtime <, > Not <, return <");
    orxConfig_SetString("OnCreate", "> @, Set Runtime < true");
    orxConfig_SetString("OnDelete", "> Object.GetName ^, Set Runtime < false, return true");
    orxConfig_SetString("LifeTime", "sound");
    orxConfig_SetString("SoundList", "@");
    orxConfig_SetString("Music", info.zFullName);
    orxConfig_SetFloat("LoopTimeOffset", loopTimeOffset);
    orxConfig_PopSection();
    sectionNames.push_back(audioName);
  }
  orxFile_FindClose(&info);

  std::sort(sectionNames.begin(), sectionNames.end());
}

void AudioDirectory::RenderRowHeader()
{
  ImGui::TableSetupColumn("File name");
  ImGui::TableSetupColumn("Play file");
  ImGui::TableSetupColumn("Stop playback");
  ImGui::TableSetupColumn("Loop");
  ImGui::TableSetupColumn("Select for config export");
  ImGui::TableHeadersRow();
}

void AudioDirectory::RenderRow(const AudioName &audioName)
{
  ImGui::PushID(audioName.sectionName.data());
  ImGui::TableNextColumn();
  ImGui::TextUnformatted(audioName.name.data());

  ImGui::TableNextColumn();
  auto playLabel = std::string{"Play##"} + audioName.sectionName;
  auto object = GetActiveObject(audioName.sectionName);
  orxFLOAT hNumerator = object == orxNULL ? 1.0f : 2.0f;
  ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(hNumerator / 7.0f, 0.6f, 0.6f));
  if (ImGui::Button(playLabel.data()))
  {
    auto object = orxObject_CreateFromConfig(audioName.sectionName.data());
    if (object != orxNULL)
    {
      activeObjects[audioName.sectionName] = orxStructure_GetGUID(orxSTRUCTURE(object));
    }
  }

  ImGui::TableNextColumn();
  auto stopLabel = std::string{"Stop##"} + audioName.sectionName;
  if (ImGui::Button(stopLabel.data()))
  {
    auto object = GetActiveObject(audioName.sectionName);
    if (object != orxNULL)
    {
      orxObject_SetLifeTime(object, 0);
    }
  }

  ImGui::PopStyleColor();

  orxConfig_PushSection(audioName.sectionName.data());
  ImGui::TableNextColumn();
  auto loopLabel = std::string{"Loop##"} + audioName.sectionName;
  bool loop = orxConfig_GetBool("Loop");
  if (ImGui::Checkbox(loopLabel.data(), &loop))
  {
    orxConfig_SetBool("Loop", loop);
  }
  orxConfig_PopSection();

  ImGui::TableNextColumn();
  auto checked = selectedSections.contains(audioName.sectionName);
  if (ImGui::Checkbox("Select", &checked))
  {
    if (checked)
    {
      selectedSections.insert(audioName.sectionName);
    }
    else
    {
      selectedSections.erase(audioName.sectionName);
    }
  }

  ImGui::PopID();
}

void AudioDirectory::Render()
{
  for (const auto &[path, directory] : subdirectories)
  {
    auto hasSearchResults = directory->HasSearchResults();
    if (searchSubstring.empty() || hasSearchResults || searchPathResults.contains(path))
    {
      ImGuiTreeNodeFlags flags = hasSearchResults ? ImGuiTreeNodeFlags_DefaultOpen : 0;
      if (ImGui::CollapsingHeader(path.data(), flags))
      {
        const orxFLOAT indentWidth = 5.0f;
        ImGui::Indent(indentWidth);
        directory->Render();
        ImGui::Unindent(indentWidth);
      }
    }
  }

  std::string tableID = std::string{"Sounds table##"} + root;
  if (ImGui::BeginTable(tableID.data(), 5))
  {
    if (sectionNames.size() > 0)
    {
      RenderRowHeader();
    }

    for (auto audioName : sectionNames)
    {
      if (searchSubstring.empty() || searchNameResults.contains(audioName.sectionName))
      {
        RenderRow(audioName);
      }
    }
    ImGui::EndTable();
  }
}

orxOBJECT *AudioDirectory::GetActiveObject(const std::string &name)
{
  orxOBJECT *object = orxNULL;

  if (activeObjects.contains(name))
  {
    auto guid = activeObjects[name];
    object = orxOBJECT(orxStructure_Get(guid));
  }

  return object;
}

void AudioDirectory::SearchNamesContaining(const std::string &substring)
{
  searchSubstring = substring;
  searchPathResults.clear();
  searchNameResults.clear();

  const std::boyer_moore_searcher searcher(substring.begin(), substring.end());
  const std::string nullChar{'\0'};
  const std::boyer_moore_searcher nullSearch(nullChar.begin(), nullChar.end());

  for (const auto &[path, subdir] : subdirectories)
  {
    subdir->SearchNamesContaining(substring);

    if (substring.empty())
    {
      // Skip if the substring is empty
      continue;
    }

    auto lcPath = ToLowercase(path);
    const auto it = std::search(lcPath.begin(), lcPath.end(), searcher);
    if (it != lcPath.end())
    {
      searchPathResults.insert(path);
    }
  }

  if (substring.empty())
  {
    // Skip searching names if the substring is empty
    return;
  }

  for (const auto &audioName : sectionNames)
  {
    auto lcName = ToLowercase(audioName.name);
    const auto it = std::search(lcName.begin(), lcName.end(), searcher);
    if (it != lcName.end())
    {
      searchNameResults.insert(audioName.sectionName);
    }
  }
}

bool AudioDirectory::HasSearchResults()
{
  if (!(searchNameResults.empty() && searchPathResults.empty()))
  {
    // Local results
    return true;
  }

  for (const auto &[path, directory] : subdirectories)
  {
    if (directory->HasSearchResults())
    {
      // Recursive results
      return true;
    }
  }

  // No results if we make it this far
  return false;
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
    ImGui::Text("Search");
    ImGui::SameLine();
    auto searchUpdated = ImGui::InputText("##", searchBuf, sizeof(searchBuf));

    if (searchUpdated)
    {
      directory->SearchNamesContaining(ToLowercase(std::string(searchBuf)));
    }

    PushConfigSection();
    directory->Render();
    PopConfigSection();
  }
  ImGui::End();

  ImGui::PopID();
}
