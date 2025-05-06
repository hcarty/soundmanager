/**
 * @file Browser.h
 * @date 3-May-2025
 */

#ifndef __BROWSER_H__
#define __BROWSER_H__

#include <map>
#include <string>
#include <vector>

#include "imgui.h"

#include "soundmanager.h"

struct AudioDirectory
{
  std::string root;
  std::map<std::string, std::unique_ptr<AudioDirectory>> subdirectories{};
  std::vector<std::string> sectionNames{};
  std::map<std::string, orxU64> activeObjects{};

  AudioDirectory(std::string rootPath);

  void ReadAll();

  void Render();

  orxOBJECT *GetActiveObject(std::string name);
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
