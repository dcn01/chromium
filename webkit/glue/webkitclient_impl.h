// Copyright (c) 2009 The Chromium Authors. All rights reserved.  Use of this
// source code is governed by a BSD-style license that can be found in the
// LICENSE file.

#ifndef WEBKIT_CLIENT_IMPL_H_
#define WEBKIT_CLIENT_IMPL_H_

#include "WebKitClient.h"

#include "base/timer.h"
#include "webkit/glue/webclipboard_impl.h"

#if defined(OS_WIN)
#include "webkit/glue/webthemeengine_impl_win.h"
#endif

class MessageLoop;

namespace webkit_glue {

class WebKitClientImpl : public WebKit::WebKitClient {
 public:
  WebKitClientImpl();

  // WebKitClient methods (partial implementation):
  virtual WebKit::WebClipboard* clipboard();
  virtual WebKit::WebThemeEngine* themeEngine();
  virtual void decrementStatsCounter(const char* name);
  virtual void incrementStatsCounter(const char* name);
  virtual void traceEventBegin(const char* name, void* id, const char* extra);
  virtual void traceEventEnd(const char* name, void* id, const char* extra);
  virtual WebKit::WebCString loadResource(const char* name);
  virtual double currentTime();
  virtual void setSharedTimerFiredFunction(void (*func)());
  virtual void setSharedTimerFireTime(double fireTime);
  virtual void stopSharedTimer();
  virtual void callOnMainThread(void (*func)());

 private:
  void DoTimeout() {
    if (shared_timer_func_)
      shared_timer_func_();
  }

  WebClipboardImpl clipboard_;
  MessageLoop* main_loop_;
  base::OneShotTimer<WebKitClientImpl> shared_timer_;
  void (*shared_timer_func_)();

#if defined(OS_WIN)
  WebThemeEngineImpl theme_engine_;
#endif
};

}  // namespace webkit_glue

#endif  // WEBKIT_CLIENT_IMPL_H_
