// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/capturer_fake_ascii.h"

#include "ui/gfx/rect.h"

namespace remoting {

static const int kWidth = 32;
static const int kHeight = 20;
static const int kBytesPerPixel = 1;

CapturerFakeAscii::CapturerFakeAscii()
    : current_buffer_(0),
      pixel_format_(media::VideoFrame::ASCII) {
}

CapturerFakeAscii::~CapturerFakeAscii() {
}

void CapturerFakeAscii::ScreenConfigurationChanged() {
  width_ = kWidth;
  height_ = kHeight;
  bytes_per_row_ = width_ * kBytesPerPixel;
  pixel_format_ = media::VideoFrame::ASCII;

  // Create memory for the buffers.
  int buffer_size = height_ * bytes_per_row_;
  for (int i = 0; i < kNumBuffers; i++) {
    buffers_[i].reset(new uint8[buffer_size]);
  }
}

media::VideoFrame::Format CapturerFakeAscii::pixel_format() const {
  return pixel_format_;
}

void CapturerFakeAscii::ClearInvalidRects() {
  helper.ClearInvalidRects();
}

void CapturerFakeAscii::InvalidateRects(const InvalidRects& inval_rects) {
  helper.InvalidateRects(inval_rects);
}

void CapturerFakeAscii::InvalidateScreen(const gfx::Size& size) {
  helper.InvalidateScreen(size);
}

void CapturerFakeAscii::InvalidateFullScreen() {
  helper.InvalidateFullScreen();
}

void CapturerFakeAscii::CaptureInvalidRects(
    CaptureCompletedCallback* callback) {
  scoped_ptr<CaptureCompletedCallback> callback_deleter(callback);

  GenerateImage();
  DataPlanes planes;
  planes.data[0] = buffers_[current_buffer_].get();
  current_buffer_ = (current_buffer_ + 1) % kNumBuffers;
  planes.strides[0] = bytes_per_row_;
  scoped_refptr<CaptureData> capture_data(new CaptureData(
      planes, gfx::Size(width_, height_), pixel_format_));

  helper.set_size_most_recent(capture_data->size());

  callback->Run(capture_data);
}

const gfx::Size& CapturerFakeAscii::size_most_recent() const {
  return helper.size_most_recent();
}

void CapturerFakeAscii::GenerateImage() {
  for (int y = 0; y < height_; ++y) {
    uint8* row = buffers_[current_buffer_].get() + bytes_per_row_ * y;
    for (int x = 0; x < bytes_per_row_; ++x) {
      if (y == 0 || x == 0 || x == (width_ - 1) || y == (height_ - 1)) {
        row[x] = '*';
      } else {
        row[x] = ' ';
      }
    }
  }
}

}  // namespace remoting
