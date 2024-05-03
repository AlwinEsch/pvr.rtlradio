/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#pragma once

#include "props.h"
#include "renderingcontrol_i.h"

#include <glm/glm.hpp>
#include <kodi/gui/Window.h>
#include <kodi/gui/controls/Rendering.h>
#include <kodi/gui/gl/GL.h>
#include <kodi/gui/gl/Shader.h>

namespace RTLRADIO
{

struct SignalStatus;

namespace GUI
{

// Class CFFTSignalMeterControl
//
// Implements the FFT rendering control
class CFFTSignalMeterControl : public IGUIRenderingControl, public kodi::gui::gl::CShaderProgram
{
public:
  // Constructor / Destructor
  //
  CFFTSignalMeterControl(kodi::gui::CWindow* window, int controlid);
  ~CFFTSignalMeterControl() override;

  // Member Functions
  //
  size_t height(void) const;
  size_t width(void) const;
  void Update(struct SignalStatus const& status, bool signallock, bool muxlockupdate);

  // FFT_BANDWIDTH
  //
  // Bandwidth of the FFT display
  static constexpr uint32_t const FFT_BANDWIDTH = (400 KHz);

  // FFT_MAXDB
  //
  // Maximum decibel value supported by the FFT
  static constexpr float const FFT_MAXDB = 4.0f;

  // FFT_MINDB
  //
  // Minimum decibel level supported by the FFT
  static constexpr float const FFT_MINDB = -72.0f;

private:
  CFFTSignalMeterControl(CFFTSignalMeterControl const&) = delete;
  CFFTSignalMeterControl& operator=(CFFTSignalMeterControl const&) = delete;

  // CFFTSignalMeterControl overrides
  //
  bool Dirty() override;
  void Render() override;

  // CShaderProgram overrides
  //
  void OnCompiledAndLinked(void) override;

  // Private Member Functions
  //
  GLfloat db_to_height(float db) const;
  void render_line(glm::vec3 color, glm::vec2 vertices[2]) const;
  void render_line(glm::vec4 color, glm::vec2 vertices[2]) const;
  void render_line_strip(glm::vec3 color, glm::vec2 vertices[], size_t numvertices) const;
  void render_line_strip(glm::vec4 color, glm::vec2 vertices[], size_t numvertices) const;
  void render_rect(glm::vec3 color, glm::vec2 vertices[4]) const;
  void render_rect(glm::vec4 color, glm::vec2 vertices[4]) const;
  void render_triangle(glm::vec3 color, glm::vec2 vertices[3]) const;
  void render_triangle(glm::vec4 color, glm::vec2 vertices[3]) const;

  GLfloat m_widthf; // Width as GLfloat
  GLfloat m_heightf; // Height as GLfloat
  GLfloat m_linewidthf = 1.25f; // Line width factor
  GLfloat m_lineheightf = 1.25f; // Line height factor

  GLuint m_vertexVBO; // Vertex buffer object
  glm::mat4 m_modelProjMat; // Model/Projection matrix

  bool m_dirty = false; // Dirty flag
  GLfloat m_power = 0.0f; // Power level
  GLfloat m_noise = 0.0f; // Noise level
  bool m_overload = false; // Overload flag
  bool m_signallock = false; // Signal lock flag
  bool m_muxlock = false; // Multiplex lock flag

  std::unique_ptr<glm::vec2[]> m_fft; // FFT vertices
  int m_fftlowcut; // FFT low cut index
  int m_ffthighcut; // FFT high cut index

  GLint m_aPosition = -1; // Attribute location
  GLint m_uColor = -1; // Attribute location
  GLint m_uModelProjMatrix = -1; // Uniform location
};

} // namespace GUI
} // namespace RTLRADIO
