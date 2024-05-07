/*
 *  Copyright (C) 2024 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: MIT
 *  See LICENSE.md for more information.
 */

#include "control_signalmeter.h"

#include "instance_pvr/signalmeter.h"

#include <glm/gtc/type_ptr.hpp>

#if !defined(TARGET_WINDOWS) && !defined(TARGET_ANDROID) && !defined(TARGET_DARWIN)
#include <EGL/egl.h>
#endif

namespace RTLRADIO
{
namespace GUI
{

// is_platform_opengles (local)
//
// Helper function to determine if the platform is full OpenGL or OpenGL ES
static bool is_platform_opengles(void)
{
#if defined(TARGET_WINDOWS)
  return true;
#elif defined(TARGET_ANDROID)
  return true;
#elif defined(TARGET_DARWIN_OSX)
  return false;
#else
  return (eglQueryAPI() == EGL_OPENGL_ES_API);
#endif
}

CFFTSignalMeterControl::CFFTSignalMeterControl(kodi::gui::CWindow* window, int controlid)
  : IGUIRenderingControl(window, controlid)
{
  // Store some handy floating point copies of the width and height for rendering
  m_widthf = static_cast<GLfloat>(m_width);
  m_heightf = static_cast<GLfloat>(m_height);

  // VERTEX SHADER
  std::string const vertexshader(
      R"(
uniform mat4 u_modelViewProjectionMatrix;

#ifdef GL_ES
attribute vec2 a_position;
#else
in vec2 a_position;
#endif

void main()
{
  gl_Position = u_modelViewProjectionMatrix * vec4(a_position, 0.0, 1.0);
}
)");

  // FRAGMENT SHADER
  std::string const fragmentshader(
      R"(
#ifdef GL_ES
precision mediump float;
#else
precision highp float;
#endif

uniform vec4 u_color;

#ifndef GL_ES
out vec4 FragColor;
#endif

void main()
{
#ifdef GL_ES
  gl_FragColor = u_color;
#else
  FragColor = u_color;
#endif
}
)");

  // Compile and link the shader programs during construction
  if (is_platform_opengles())
    CompileAndLink("#version 100\n", vertexshader, "#version 100\n", fragmentshader);
  else
    CompileAndLink("#version 150\n", vertexshader, "#version 150\n", fragmentshader);

  // Allocate the buffer to hold the scaled FFT data
  m_fft = std::unique_ptr<glm::vec2[]>(new glm::vec2[m_width]);

  // Initialize the cut points
  m_fftlowcut = m_ffthighcut = -1;

  // Create the model/view/projection matrix based on width and height
  m_modelProjMat = glm::ortho(0.0f, m_widthf, m_heightf, 0.0f);

  // Create the vertex buffer object
  glGenBuffers(1, &m_vertexVBO);
}

CFFTSignalMeterControl::~CFFTSignalMeterControl()
{
  // Delete the vertex buffer object
  glDeleteBuffers(1, &m_vertexVBO);
}

bool CFFTSignalMeterControl::Dirty()
{
  return m_dirty;
}

void CFFTSignalMeterControl::Render()
{
  // Ensure that the shader was compiled properly before continuing
  assert(ShaderOK());
  if (!ShaderOK())
    return;

  // Enable blending
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Enable the shader program
  EnableShader();

  // Set the model/view/projection matrix
  glUniformMatrix4fv(m_uModelProjMatrix, 1, GL_FALSE, glm::value_ptr(m_modelProjMat));

  // Bind the vertex buffer object
  glBindBuffer(GL_ARRAY_BUFFER, m_vertexVBO);

  // Enable the vertex array
  glEnableVertexAttribArray(m_aPosition);

  // Set the vertex attribute pointer type
  glVertexAttribPointer(m_aPosition, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2),
                        BUFFER_OFFSET(offsetof(glm::vec2, x)));

#ifndef HAS_ANGLE
  // Background
  glm::vec2 backgroundrect[4] = {
      {0.0f, 0.0f}, {0.0f, m_heightf}, {m_widthf, 0.0f}, {m_widthf, m_heightf}};
  render_rect(glm::vec3(0.0f, 0.0f, 0.0f), backgroundrect);
#endif

  // 0dB level
  GLfloat zerodb = db_to_height(0.0f);
  glm::vec2 zerodbline[2] = {{0.0f, zerodb}, {m_widthf, zerodb}};
  render_line(glm::vec4(1.0f, 1.0f, 0.0f, 0.75f), zerodbline);

  // -6dB increment levels
  for (int index = -6; index >= static_cast<int>(FFT_MINDB); index -= 6)
  {

    GLfloat y = db_to_height(static_cast<float>(index));
    glm::vec2 dbline[2] = {{0.0f, y}, {m_widthf, y}};
    render_line(glm::vec4(1.0f, 1.0f, 1.0f, 0.2f), dbline);
  }

  // Power range
  glm::vec2 powerrect[4] = {
      {0.0f, m_power}, {m_widthf, m_power}, {0.0f, m_noise}, {m_widthf, m_noise}};
  render_rect(glm::vec4(0.0f, 1.0f, 0.0f, 0.1f), powerrect);
  glm::vec2 powerline[2] = {{0.0f, m_power}, {m_widthf, m_power}};
  render_line(glm::vec4(0.0f, 1.0f, 0.0f, 0.75f), powerline);

  // Noise range
  glm::vec2 noiserect[4] = {
      {0.0f, m_noise}, {m_widthf, m_noise}, {0.0f, m_heightf}, {m_widthf, m_heightf}};
  render_rect(glm::vec4(1.0f, 0.0f, 0.0f, 0.15f), noiserect);
  glm::vec2 noiseline[2] = {{0.0f, m_noise}, {m_width, m_noise}};
  render_line(glm::vec4(1.0f, 0.0f, 0.0f, 0.75f), noiseline);

  // Center frequency
  glm::vec2 centerline[2] = {{m_widthf / 2.0f, 0.0f}, {m_widthf / 2.0f, m_heightf}};
  render_line(glm::vec4(1.0f, 1.0f, 0.0f, 0.75f), centerline);

  // Low cut
  glm::vec2 lowcutline[2] = {{static_cast<GLfloat>(m_fftlowcut), 0.0f},
                             {static_cast<GLfloat>(m_fftlowcut), m_heightf}};
  render_line(glm::vec4(1.0f, 1.0f, 1.0f, 0.4f), lowcutline);

  // High cut
  glm::vec2 highcutline[2] = {{static_cast<GLfloat>(m_ffthighcut), 0.0f},
                              {static_cast<GLfloat>(m_ffthighcut), m_heightf}};
  render_line(glm::vec4(1.0f, 1.0f, 1.0f, 0.4f), highcutline);

  // FFT
  glm::vec3 fftcolor(0.5f, 0.5f, 0.5f);
  if (m_overload)
    fftcolor = glm::vec3(1.0f, 0.0f, 0.0f);
  else if (m_signallock)
  {

    // If there is also a lock on the multiplex use a green line
    if (m_muxlock)
      fftcolor = glm::vec3(0.2823f, 0.7333f, 0.0901f); // Kelly Green (#4CBB17)
    else
      fftcolor = glm::vec3(1.0f, 1.0f, 1.0f);
  }

  render_line_strip(fftcolor, m_fft.get(), m_width);

  glDisableVertexAttribArray(m_aPosition); // Disable the vertex array
  glBindBuffer(GL_ARRAY_BUFFER, 0); // Unbind the VBO

  DisableShader(); // Disable the shader program
  glDisable(GL_BLEND); // Disable blending

  // Render state is clean until the next update from the meter instance
  m_dirty = false;
}

void CFFTSignalMeterControl::OnCompiledAndLinked(void)
{
  m_aPosition = glGetAttribLocation(ProgramHandle(), "a_position");
  m_uColor = glGetUniformLocation(ProgramHandle(), "u_color");
  m_uModelProjMatrix = glGetUniformLocation(ProgramHandle(), "u_modelViewProjectionMatrix");
}

inline GLfloat CFFTSignalMeterControl::db_to_height(float db) const
{
  return m_heightf * ((db - FFT_MAXDB) / (FFT_MINDB - FFT_MAXDB));
}

size_t CFFTSignalMeterControl::height(void) const
{
  return static_cast<size_t>(m_height);
}

void CFFTSignalMeterControl::render_line(glm::vec3 color, glm::vec2 vertices[2]) const
{
  return render_line(glm::vec4(color, 1.0f), vertices);
}

void CFFTSignalMeterControl::render_line(glm::vec4 color, glm::vec2 vertices[2]) const
{
  // Set the specific color value before drawing the line
  glUniform4f(m_uColor, color.r, color.g, color.b, color.a);

  glm::vec2 p(vertices[1].x - vertices[0].x, vertices[1].y - vertices[0].y);
  p = glm::normalize(p);

  // Pre-calculate the required deltas for the line thickness
#if defined(WIN32) && defined(HAS_ANGLE)
  GLfloat const dx = (m_linewidthf);
  GLfloat const dy = (m_lineheightf);
#else
  GLfloat const dx = (m_linewidthf / 2.0f);
  GLfloat const dy = (m_lineheightf / 2.0f);
#endif

  glm::vec2 const p1(-p.y, p.x);
  glm::vec2 const p2(p.y, -p.x);

  glm::vec2 strip[] = {

      {vertices[0].x + p1.x * dx, vertices[0].y + p1.y * dy},
      {vertices[0].x + p2.x * dx, vertices[0].y + p2.y * dy},
      {vertices[1].x + p1.x * dx, vertices[1].y + p1.y * dy},
      {vertices[1].x + p2.x * dx, vertices[1].y + p2.y * dy}};

  glBufferData(GL_ARRAY_BUFFER, (sizeof(glm::vec2) * 4), &strip[0], GL_STATIC_DRAW);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void CFFTSignalMeterControl::render_line_strip(glm::vec3 color,
                                               glm::vec2 vertices[],
                                               size_t numvertices) const
{
  return render_line_strip(glm::vec4(color, 1.0f), vertices, numvertices);
}

void CFFTSignalMeterControl::render_line_strip(glm::vec4 color,
                                               glm::vec2 vertices[],
                                               size_t numvertices) const
{
  // Set the specific color value before drawing the line
  glUniform4f(m_uColor, color.r, color.g, color.b, color.a);

  // Each point in the line strip will be represented by six vertices in the triangle strip
  std::unique_ptr<glm::vec2[]> strip(new glm::vec2[numvertices * 6]);
  size_t strippos = 0;

  // Pre-calculate the required deltas for the line thickness
#if defined(WIN32) && defined(HAS_ANGLE)
  GLfloat const dx = (m_linewidthf);
  GLfloat const dy = (m_lineheightf);
#else
  GLfloat const dx = (m_linewidthf / 2.0f);
  GLfloat const dy = (m_lineheightf / 2.0f);
#endif

  for (size_t index = 0; index < numvertices - 1; index++)
  {
    glm::vec2 const& a = vertices[index];
    glm::vec2 const& b = vertices[index + 1];

    glm::vec2 p(b.x - a.x, b.y - a.y);
    p = glm::normalize(p);

    glm::vec2 const p1(-p.y, p.x);
    glm::vec2 const p2(p.y, -p.x);

    strip[strippos++] = a;
    strip[strippos++] = b;
    strip[strippos++] = glm::vec2(a.x + p1.x * dx, a.y + p1.y * dy);
    strip[strippos++] = glm::vec2(a.x + p2.x * dx, a.y + p2.y * dy);
    strip[strippos++] = glm::vec2(b.x + p1.x * dx, b.y + p1.y * dy);
    strip[strippos++] = glm::vec2(b.x + p2.x * dx, b.y + p2.y * dy);
  }

  glBufferData(GL_ARRAY_BUFFER, (sizeof(glm::vec2) * strippos), strip.get(), GL_STATIC_DRAW);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(strippos));
}

void CFFTSignalMeterControl::render_rect(glm::vec3 color, glm::vec2 vertices[4]) const
{
  return render_rect(glm::vec4(color, 1.0f), vertices);
}

void CFFTSignalMeterControl::render_rect(glm::vec4 color, glm::vec2 vertices[4]) const
{
  // Set the specific color value before drawing the line
  glUniform4f(m_uColor, color.r, color.g, color.b, color.a);

  // Render the rectangle as a 4-vertex GL_TRIANGLE_STRIP primitive
  glBufferData(GL_ARRAY_BUFFER, (sizeof(glm::vec2) * 4), &vertices[0], GL_STATIC_DRAW);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void CFFTSignalMeterControl::render_triangle(glm::vec3 color, glm::vec2 vertices[3]) const
{
  return render_triangle(glm::vec4(color, 1.0f), vertices);
}

void CFFTSignalMeterControl::render_triangle(glm::vec4 color, glm::vec2 vertices[3]) const
{
  // Set the specific color value before drawing the line
  glUniform4f(m_uColor, color.r, color.g, color.b, color.a);

  // Render the triangle as a 3-vertex GL_TRIANGLES primitive
  glBufferData(GL_ARRAY_BUFFER, (sizeof(glm::vec2) * 3), &vertices[0], GL_STATIC_DRAW);
  glDrawArrays(GL_TRIANGLES, 0, 3);
}

void CFFTSignalMeterControl::Update(struct SignalStatus const& status,
                                    bool signallock,
                                    bool muxlock)
{
  // Power and noise values are supplied as dB and need to be scaled to the viewport
  m_power = db_to_height(status.power);
  m_noise = db_to_height(status.noise);

  // The low and high cuts are provided as indexes into the plot data
  m_fftlowcut = status.lowcut;
  m_ffthighcut = status.highcut;

  // The length of the fft data should match the width of the control, but watch for overrruns
  assert(status.plotsize == m_width);
  size_t length = std::min(status.plotsize, static_cast<size_t>(m_width));

  // The FFT data merely needs to be converted into an X,Y vertex to be used by the renderer
  for (size_t index = 0; index < length; index++)
    m_fft[index] = glm::vec2(static_cast<float>(index), static_cast<float>(status.plotdata[index]));

  // The FFT line strip will be shown in a different color based on this information
  m_overload = status.overload;
  m_signallock = signallock;
  m_muxlock = muxlock;

  // In the event of an FFT data underrun, flat-line the remainder of the data points
  if (m_width > status.plotsize)
  {

    for (size_t index = length; index < m_width; index++)
      m_fft[index] = glm::vec2(static_cast<float>(index), m_heightf);
  }

  m_dirty = true; // Scene needs to be re-rendered
}

size_t CFFTSignalMeterControl::width(void) const
{
  return static_cast<size_t>(m_width);
}

} // namespace GUI
} // namespace RTLRADIO
