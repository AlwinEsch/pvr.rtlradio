/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#pragma once

#include "utils/log.h"

#include <kodi/c-api/gui/controls/rendering.h>
#include <kodi/gui/Window.h>
#include <kodi/gui/renderHelper.h>

//! Fix stupid macros on Windows where destroy always own codes :(.
#ifdef DEBUG
#undef DEBUG
#endif // DEBUG
#ifdef INFO
#undef INFO
#endif // INFO
#ifdef WARNING
#undef WARNING
#endif // WARNING
#ifdef ERROR
#undef ERROR
#endif // ERROR
#ifdef FATAL
#undef FATAL
#endif // FATAL

namespace RTLRADIO
{
namespace GUI
{

//-----------------------------------------------------------------------------
// Class renderingcontrol
//
// Replacement CRendering control; required since the one provided by Kodi
// cannot be made to work for either of its valid use cases.  If you derive
// from CRendering, it will invoke a virtual member function during construction
// that needs to be overridden (oops).  If you instead opt to use independent
// static callback functions, the required pre and post-rendering code will
// never get invoked, and you can't do that in your callbacks because the
// member variable you need is marked as private to CRendering.
//
// There is also another problem that can occur if the application is being
// shut down while a render control is active; it's calling Stop() after the
// object has been destroyed; just take Stop() out for now, I don't need it

class ATTR_DLL_LOCAL IGUIRenderingControl : public kodi::gui::CAddonGUIControlBase
{
public:
  IGUIRenderingControl(kodi::gui::CWindow* window, int controlId) : CAddonGUIControlBase(window)
  {
    // Find the control handle
    m_controlHandle = m_interface->kodi_gui->window->get_control_render_addon(
        m_interface->kodiBase, m_Window->GetControlHandle(), controlId);

    // Initialize the control callbacks; be advised this implicitly calls the OnCreate callback
    if (m_controlHandle)
      m_interface->kodi_gui->control_rendering->set_callbacks(m_interface->kodiBase,
                                                              m_controlHandle, this, OnCreateCB,
                                                              OnRenderCB, OnStopCB, OnDirtyCB);
    else
      UTILS::log(UTILS::LOGLevel::DEBUG, __src_loc_cur__,
                 "Can't create control class from Kodi !!!");
  }

  ~IGUIRenderingControl() override
  {
    // Destroy the control
    m_interface->kodi_gui->control_rendering->destroy(m_interface->kodiBase, m_controlHandle);
  }

  virtual bool Dirty() { return false; }
  virtual void Render() {}
  virtual void Stop() {}

protected:
  //-------------------------------------------------------------------------
  // Protected Member Variables
  //-------------------------------------------------------------------------

  size_t m_left{}; // Horizontal position of the control
  size_t m_top{}; // Vertical position of the control
  size_t m_width{}; // Width of the control
  size_t m_height{}; // Height of the control

  kodi::HardwareContext m_device{}; // Device to use, only set for DirectX

private:
  static bool OnCreateCB(
      KODI_GUI_CLIENT_HANDLE handle, int x, int y, int w, int h, ADDON_HARDWARE_CONTEXT device)
  {
    assert(handle);
    IGUIRenderingControl* instance = reinterpret_cast<IGUIRenderingControl*>(handle);

    // This is called during object construction, virtual member functions are not available.
    // Store off all of the provided size, position, and device information
    instance->m_left = static_cast<size_t>(x);
    instance->m_top = static_cast<size_t>(y);
    instance->m_width = static_cast<size_t>(w);
    instance->m_height = static_cast<size_t>(h);
    instance->m_device = device;

    // Access the rendering helper instance
    instance->m_renderHelper = kodi::gui::GetRenderHelper();

    return true;
  }

  static bool OnDirtyCB(KODI_GUI_CLIENT_HANDLE handle)
  {
    assert(handle);
    return reinterpret_cast<IGUIRenderingControl*>(handle)->Dirty();
  }

  // OnRender
  //
  // Invoked by the underlying rendering control to render any dirty regions
  static void OnRenderCB(KODI_GUI_CLIENT_HANDLE handle)
  {
    assert(handle);
    IGUIRenderingControl* instance = reinterpret_cast<IGUIRenderingControl*>(handle);

    if (!instance->m_renderHelper)
      return;

    // Render the control
    instance->m_renderHelper->Begin();
    instance->Render();
    instance->m_renderHelper->End();
  }

  // OnStop
  //
  // Invoked by the underlying rendering control to stop the rendering process
  static void OnStopCB(KODI_GUI_CLIENT_HANDLE /*handle*/)
  {
    //
    // Removed implementation due to this being called after the object pointed
    // to by handle has been destroyed if the application is closing. The render
    // helper instance will be released automatically during destruction
    //

    //assert(handle);
    //IGUIRenderingControl* instance = reinterpret_cast<IGUIRenderingControl*>(handle);

    //// Stop rendering
    //instance->Stop();

    //// Reset the rendering helper instance pointer
    //instance->m_renderHelper = nullptr;
  }

  //-------------------------------------------------------------------------
  // Member Variables
  //-------------------------------------------------------------------------

  std::shared_ptr<kodi::gui::IRenderHelper> m_renderHelper; // Helper
};

} // namespace GUI
} // namespace RTLRADIO
