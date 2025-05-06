/**
 * @file soundmanager.cpp
 * @date 3-May-2025
 */

#define __SCROLL_IMPL__
#include "soundmanager.h"
#undef __SCROLL_IMPL__

#include "ImGuiDemoWindow.h"
#include "Browser.h"
#include "orxExtensions.h"

#ifdef __orxMSVC__

/* Requesting high performance dedicated GPU on hybrid laptops */
__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;

#endif // __orxMSVC__

/** Update function, it has been registered to be called every tick of the core clock
 */
void soundmanager::Update(const orxCLOCK_INFO &_rstClockInfo)
{
  // Should quit?
  if (orxInput_HasBeenActivated("Quit"))
  {
    // Send close event
    orxEvent_SendShort(orxEVENT_TYPE_SYSTEM, orxSYSTEM_EVENT_CLOSE);
  }
}

/** Camera Update function, it has been registered to be called every tick of the core clock, after physics & objects have been updated
 */
void soundmanager::CameraUpdate(const orxCLOCK_INFO &_rstClockInfo)
{
  // Get MainCamera proxy object
  ScrollObject *poMainCamera = soundmanager::GetInstance().GetObject("MainCamera");
  if (poMainCamera)
  {
    // Update & move the camera here
    // [...]

    // Round its position
    orxVECTOR vPosition;
    poMainCamera->SetPosition(*orxVector_Round(&vPosition, &poMainCamera->GetPosition(vPosition)));
  }
}

/** Init function, it is called when all orx's modules have been initialized
 */
orxSTATUS soundmanager::Init()
{
  // Init extensions
  InitExtensions();

  // Push [Main] as the default config section
  orxConfig_PushSection("Main");

  // Create the viewports
  for (orxS32 i = 0, iCount = orxConfig_GetListCount("ViewportList"); i < iCount; i++)
  {
    orxViewport_CreateFromConfig(orxConfig_GetListString("ViewportList", i));
  }

  // Create the scene
  CreateObject("Scene");

  // Done!
  return orxSTATUS_SUCCESS;
}

/** Run function, it should not contain any game logic
 */
orxSTATUS soundmanager::Run()
{
  // Return orxSTATUS_FAILURE to instruct orx to quit
  return orxSTATUS_SUCCESS;
}

/** Exit function, it is called before exiting from orx
 */
void soundmanager::Exit()
{
  // Exit from extensions
  ExitExtensions();

  // Let orx clean all our mess automatically. :)
}

/** BindObjects function, ScrollObject-derived classes are bound to config sections here
 */
void soundmanager::BindObjects()
{
  BindObject(Browser);
  BindObject(ImGuiDemoWindow);
}

/** Bootstrap function, it is called before config is initialized, allowing for early resource storage definitions
 */
orxSTATUS soundmanager::Bootstrap() const
{
  // Bootstrap extensions
  BootstrapExtensions();

  // Return orxSTATUS_FAILURE to prevent orx from loading the default config file
  return orxSTATUS_SUCCESS;
}

/** Main function
 */
int main(int argc, char **argv)
{
  // Execute our game
  soundmanager::GetInstance().Execute(argc, argv);

  // Done!
  return EXIT_SUCCESS;
}
