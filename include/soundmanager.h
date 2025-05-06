/**
 * @file soundmanager.h
 * @date 3-May-2025
 */

#ifndef __soundmanager_H__
#define __soundmanager_H__

#include "Scroll/Scroll.h"

/** Game Class
 */
class soundmanager : public Scroll<soundmanager>
{
public:


private:

                orxSTATUS       Bootstrap() const;

                void            Update(const orxCLOCK_INFO &_rstInfo);
                void            CameraUpdate(const orxCLOCK_INFO &_rstInfo);

                orxSTATUS       Init();
                orxSTATUS       Run();
                void            Exit();
                void            BindObjects();


private:
};

#endif // __soundmanager_H__
