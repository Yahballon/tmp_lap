// SPDX-License-Identifier: GPL-3.0-or-later
#include "engine/ps5_palette_texture.hpp"
#include "gfx/gl_batch.hpp"
#include <GL/glcorearb.h>
namespace zcps5::engine {
bool Ps5PaletteTexture::initialize(hui::gfx::GlBatch& batch, PalettedSurface& frame) {
  if (texture_ != 0 || frame.width() == 0 || frame.height() == 0) return false;
  width_ = frame.width();
  height_ = frame.height();
  texture_ = batch.create_texture(width_, height_, frame.rgba().data());
  if (texture_ == 0) { width_ = height_ = 0; return false; }
  glBindTexture(GL_TEXTURE_2D, texture_);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  return true;
}
bool Ps5PaletteTexture::refresh(PalettedSurface& frame) {
  if (texture_ == 0 || frame.width() != width_ || frame.height() != height_) return false;
  const auto& rgba = frame.rgba();
  glBindTexture(GL_TEXTURE_2D, texture_);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_,
                  GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
  return true;
}
void Ps5PaletteTexture::release() {
  if (texture_) {
    const GLuint id = static_cast<GLuint>(texture_);
    glDeleteTextures(1, &id);
  }
  texture_ = 0;
  width_ = height_ = 0;
}
} // namespace zcps5::engine
