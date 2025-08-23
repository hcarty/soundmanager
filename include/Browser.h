/**
 * @file Browser.h
 * @date 3-May-2025
 */

#ifndef __BROWSER_H__
#define __BROWSER_H__

#include <map>
#include <set>
#include <string>
#include <vector>
#include <memory>

#include "imgui.h"

#include "soundmanager.h"

class Export : public ScrollObject
{
public:
protected:
  void OnCreate();
  void OnDelete();
  void Update(const orxCLOCK_INFO &_rstInfo);

private:
};

struct AudioDirectory
{
  std::string root;
  std::map<std::string, std::unique_ptr<AudioDirectory>> subdirectories{};
  std::vector<std::string> sectionNames{};
  std::map<std::string, orxU64> activeObjects{};

  char searchBuf[2048]{"\0"};
  std::set<std::string> searchPathResults{};
  std::set<std::string> searchNameResults{};

  AudioDirectory(const std::string &rootPath);

  void ReadAll();

  /// @brief Setup table header
  void RenderRowHeader();

  /// @brief Render a single table row
  /// @param name Name of the sound/music section to use in this row
  void RenderRow(const std::string &name);

  /// @brief Render full table of entries including the header and rows
  void Render();

  orxOBJECT *GetActiveObject(const std::string &name);

  void SearchNamesContaining(const std::string &substring);

  bool HasSearchResults();
};

/** Browser Class
 */
class Browser : public ScrollObject
{
public:
protected:
  void OnCreate();
  void OnDelete();
  void Update(const orxCLOCK_INFO &_rstInfo);

private:
  std::string rootPath{};

  std::unique_ptr<AudioDirectory> directory;
};

#endif // __BROWSER_H__
