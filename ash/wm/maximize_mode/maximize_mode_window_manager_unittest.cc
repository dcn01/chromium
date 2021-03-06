// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/maximize_mode/maximize_mode_window_manager.h"

#include "ash/shell.h"
#include "ash/switchable_windows.h"
#include "ash/test/ash_test_base.h"
#include "ash/test/shell_test_api.h"
#include "ash/wm/mru_window_tracker.h"
#include "ash/wm/window_state.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/test/test_windows.h"
#include "ui/aura/window.h"

namespace ash {

class MaximizeModeWindowManagerTest : public test::AshTestBase {
public:
  MaximizeModeWindowManagerTest() {}
  virtual ~MaximizeModeWindowManagerTest() {}

  // Creates a window. Note: This function will only work with a single root
  // window.
  aura::Window* CreateNonMaximizableWindow(ui::wm::WindowType type,
                                           const gfx::Rect& bounds) {
    return CreateWindowInWatchedContainer(type, bounds, false);
  }

  // Creates a window.
  aura::Window* CreateWindow(ui::wm::WindowType type,
                             const gfx::Rect bounds) {
    return CreateWindowInWatchedContainer(type, bounds, true);
  }

  // Create the Maximized mode window manager.
  ash::internal::MaximizeModeWindowManager* CreateMaximizeModeWindowManager() {
    EXPECT_FALSE(maximize_mode_window_manager());
    Shell::GetInstance()->EnableMaximizeModeWindowManager(true);
    return maximize_mode_window_manager();
  }

  // Destroy the maximized mode window manager.
  void DestroyMaximizeModeWindowManager() {
    Shell::GetInstance()->EnableMaximizeModeWindowManager(false);
    EXPECT_FALSE(maximize_mode_window_manager());
  }

  // Get the maximze window manager.
  ash::internal::MaximizeModeWindowManager* maximize_mode_window_manager() {
    test::ShellTestApi test_api(Shell::GetInstance());
    return test_api.maximize_mode_window_manager();
  }

  // Resize our desktop.
  void ResizeDesktop(int width_delta) {
    aura::Window* container = Shell::GetContainer(
        Shell::GetPrimaryRootWindow(), kSwitchableWindowContainerIds[0]);
    gfx::Rect bounds = container->bounds();
    bounds.set_width(bounds.width() - width_delta);
    container->SetBounds(bounds);
  }

 private:
  // Create a window in one of the containers which are watched by the
  // MaximizeModeWindowManager. Note that this only works with one root window.
  aura::Window* CreateWindowInWatchedContainer(ui::wm::WindowType type,
                                               const gfx::Rect& bounds,
                                               bool can_maximize) {
    aura::Window* window = aura::test::CreateTestWindowWithDelegateAndType(
        NULL, type, 0, bounds, NULL);
    window->SetProperty(aura::client::kCanMaximizeKey, can_maximize);
    aura::Window* container = Shell::GetContainer(
        Shell::GetPrimaryRootWindow(),
        kSwitchableWindowContainerIds[0]);
    container->AddChild(window);
    return window;
  }

  DISALLOW_COPY_AND_ASSIGN(MaximizeModeWindowManagerTest);
};

// Test that creating the object and destroying it without any windows should
// not cause any problems.
TEST_F(MaximizeModeWindowManagerTest, SimpleStart) {
  ash::internal::MaximizeModeWindowManager* manager =
      CreateMaximizeModeWindowManager();
  ASSERT_TRUE(manager);
  EXPECT_EQ(0, manager->GetNumberOfManagedWindows());
  DestroyMaximizeModeWindowManager();
}

// Test that existing windows will handled properly when going into maximized
// mode.
TEST_F(MaximizeModeWindowManagerTest, PreCreateWindows) {
  // Bounds for windows we know can be controlled.
  gfx::Rect rect1(10, 10, 200, 50);
  gfx::Rect rect2(10, 60, 200, 50);
  gfx::Rect rect3(20, 140, 100, 100);
  // Bounds for anything else.
  gfx::Rect rect(80, 90, 100, 110);
  scoped_ptr<aura::Window> w1(CreateWindow(ui::wm::WINDOW_TYPE_NORMAL, rect1));
  scoped_ptr<aura::Window> w2(CreateWindow(ui::wm::WINDOW_TYPE_NORMAL, rect2));
  scoped_ptr<aura::Window> w3(
      CreateNonMaximizableWindow(ui::wm::WINDOW_TYPE_NORMAL, rect3));
  scoped_ptr<aura::Window> w4(CreateWindow(ui::wm::WINDOW_TYPE_PANEL, rect));
  scoped_ptr<aura::Window> w5(CreateWindow(ui::wm::WINDOW_TYPE_POPUP, rect));
  scoped_ptr<aura::Window> w6(CreateWindow(ui::wm::WINDOW_TYPE_CONTROL, rect));
  scoped_ptr<aura::Window> w7(CreateWindow(ui::wm::WINDOW_TYPE_MENU, rect));
  scoped_ptr<aura::Window> w8(CreateWindow(ui::wm::WINDOW_TYPE_TOOLTIP, rect));
  EXPECT_FALSE(wm::GetWindowState(w1.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w2.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w3.get())->IsMaximized());
  EXPECT_EQ(rect1.ToString(), w1->bounds().ToString());
  EXPECT_EQ(rect2.ToString(), w2->bounds().ToString());
  EXPECT_EQ(rect3.ToString(), w3->bounds().ToString());

  // Create the manager and make sure that all qualifying windows were detected
  // and changed.
  ash::internal::MaximizeModeWindowManager* manager =
      CreateMaximizeModeWindowManager();
  ASSERT_TRUE(manager);
  EXPECT_EQ(3, manager->GetNumberOfManagedWindows());
  EXPECT_TRUE(wm::GetWindowState(w1.get())->IsMaximized());
  EXPECT_TRUE(wm::GetWindowState(w2.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w3.get())->IsMaximized());
  EXPECT_NE(rect3.origin().ToString(), w3->bounds().origin().ToString());
  EXPECT_EQ(rect3.size().ToString(), w3->bounds().size().ToString());

  // All other windows should not have been touched.
  EXPECT_FALSE(wm::GetWindowState(w4.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w5.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w6.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w7.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w8.get())->IsMaximized());
  EXPECT_EQ(rect.ToString(), w4->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w5->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w6->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w7->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w8->bounds().ToString());

  // Destroy the manager again and check that the windows return to their
  // previous state.
  DestroyMaximizeModeWindowManager();
  EXPECT_FALSE(wm::GetWindowState(w1.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w2.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w3.get())->IsMaximized());
  EXPECT_EQ(rect1.ToString(), w1->bounds().ToString());
  EXPECT_EQ(rect2.ToString(), w2->bounds().ToString());
  EXPECT_EQ(rect3.ToString(), w3->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w4->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w5->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w6->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w7->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w8->bounds().ToString());
}

// Test that creating windows while a maximizer exists picks them properly up.
TEST_F(MaximizeModeWindowManagerTest, CreateWindows) {
  ash::internal::MaximizeModeWindowManager* manager =
      CreateMaximizeModeWindowManager();
  ASSERT_TRUE(manager);
  EXPECT_EQ(0, manager->GetNumberOfManagedWindows());

  // Create the windows and see that the window manager picks them up.
  // Rects for windows we know can be controlled.
  gfx::Rect rect1(10, 10, 200, 50);
  gfx::Rect rect2(10, 60, 200, 50);
  gfx::Rect rect3(20, 140, 100, 100);
  // One rect for anything else.
  gfx::Rect rect(80, 90, 100, 110);
  scoped_ptr<aura::Window> w1(CreateWindow(ui::wm::WINDOW_TYPE_NORMAL, rect1));
  scoped_ptr<aura::Window> w2(CreateWindow(ui::wm::WINDOW_TYPE_NORMAL, rect2));
  scoped_ptr<aura::Window> w3(
      CreateNonMaximizableWindow(ui::wm::WINDOW_TYPE_NORMAL, rect3));
  scoped_ptr<aura::Window> w4(CreateWindow(ui::wm::WINDOW_TYPE_PANEL, rect));
  scoped_ptr<aura::Window> w5(CreateWindow(ui::wm::WINDOW_TYPE_POPUP, rect));
  scoped_ptr<aura::Window> w6(CreateWindow(ui::wm::WINDOW_TYPE_CONTROL, rect));
  scoped_ptr<aura::Window> w7(CreateWindow(ui::wm::WINDOW_TYPE_MENU, rect));
  scoped_ptr<aura::Window> w8(CreateWindow(ui::wm::WINDOW_TYPE_TOOLTIP, rect));
  EXPECT_TRUE(wm::GetWindowState(w1.get())->IsMaximized());
  EXPECT_TRUE(wm::GetWindowState(w2.get())->IsMaximized());
  EXPECT_EQ(3, manager->GetNumberOfManagedWindows());
  EXPECT_FALSE(wm::GetWindowState(w3.get())->IsMaximized());
  EXPECT_NE(rect3.ToString(), w3->bounds().ToString());

  // All other windows should not have been touched.
  EXPECT_FALSE(wm::GetWindowState(w4.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w5.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w6.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w7.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w8.get())->IsMaximized());
  EXPECT_EQ(rect.ToString(), w4->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w5->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w6->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w7->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w8->bounds().ToString());

  // After the maximize mode was disabled all windows fall back into the mode
  // they were created for.
  DestroyMaximizeModeWindowManager();
  EXPECT_FALSE(wm::GetWindowState(w1.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w2.get())->IsMaximized());
  EXPECT_FALSE(wm::GetWindowState(w3.get())->IsMaximized());
  EXPECT_EQ(rect1.ToString(), w1->bounds().ToString());
  EXPECT_EQ(rect2.ToString(), w2->bounds().ToString());
  EXPECT_EQ(rect3.ToString(), w3->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w4->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w5->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w6->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w7->bounds().ToString());
  EXPECT_EQ(rect.ToString(), w8->bounds().ToString());
}

// Test that windows which got created before the maximizer was created can be
// destroyed while the maximizer is still running.
TEST_F(MaximizeModeWindowManagerTest, PreCreateWindowsDeleteWhileActive) {
  ash::internal::MaximizeModeWindowManager* manager = NULL;
  {
    // Bounds for windows we know can be controlled.
    gfx::Rect rect1(10, 10, 200, 50);
    gfx::Rect rect2(10, 60, 200, 50);
    gfx::Rect rect3(20, 140, 100, 100);
    // Bounds for anything else.
    gfx::Rect rect(80, 90, 100, 110);
    scoped_ptr<aura::Window> w1(
        CreateWindow(ui::wm::WINDOW_TYPE_NORMAL, rect1));
    scoped_ptr<aura::Window> w2(
        CreateWindow(ui::wm::WINDOW_TYPE_NORMAL, rect2));
    scoped_ptr<aura::Window> w3(
        CreateNonMaximizableWindow(ui::wm::WINDOW_TYPE_NORMAL, rect3));

    // Create the manager and make sure that all qualifying windows were
    // detected and changed.
    manager = CreateMaximizeModeWindowManager();
    ASSERT_TRUE(manager);
    EXPECT_EQ(3, manager->GetNumberOfManagedWindows());
  }
  EXPECT_EQ(0, manager->GetNumberOfManagedWindows());
  DestroyMaximizeModeWindowManager();
}

// Test that windows which got created while the maximizer was running can get
// destroyed before the maximizer gets destroyed.
TEST_F(MaximizeModeWindowManagerTest, CreateWindowsAndDeleteWhileActive) {
  ash::internal::MaximizeModeWindowManager* manager =
      CreateMaximizeModeWindowManager();
  ASSERT_TRUE(manager);
  EXPECT_EQ(0, manager->GetNumberOfManagedWindows());
  {
    // Bounds for windows we know can be controlled.
    gfx::Rect rect1(10, 10, 200, 50);
    gfx::Rect rect2(10, 60, 200, 50);
    gfx::Rect rect3(20, 140, 100, 100);
    // Bounds for anything else.
    gfx::Rect rect(80, 90, 100, 110);
    scoped_ptr<aura::Window> w1(
        CreateWindow(ui::wm::WINDOW_TYPE_NORMAL, rect1));
    scoped_ptr<aura::Window> w2(
        CreateWindow(ui::wm::WINDOW_TYPE_NORMAL, rect2));
    scoped_ptr<aura::Window> w3(
        CreateNonMaximizableWindow(ui::wm::WINDOW_TYPE_NORMAL, rect3));

    EXPECT_EQ(3, manager->GetNumberOfManagedWindows());
  }
  EXPECT_EQ(0, manager->GetNumberOfManagedWindows());
  DestroyMaximizeModeWindowManager();
}

// Test that windows which were maximized stay maximized.
TEST_F(MaximizeModeWindowManagerTest, MaximizedShouldRemainMaximized) {
  // Bounds for windows we know can be controlled.
  gfx::Rect rect(10, 10, 200, 50);
  scoped_ptr<aura::Window> window(
      CreateWindow(ui::wm::WINDOW_TYPE_NORMAL, rect));
  wm::GetWindowState(window.get())->Maximize();

  // Create the manager and make sure that the window gets detected.
  ash::internal::MaximizeModeWindowManager* manager =
      CreateMaximizeModeWindowManager();
  ASSERT_TRUE(manager);
  EXPECT_EQ(1, manager->GetNumberOfManagedWindows());
  EXPECT_TRUE(wm::GetWindowState(window.get())->IsMaximized());

  // Destroy the manager again and check that the window will remain maximized.
  DestroyMaximizeModeWindowManager();
  EXPECT_TRUE(wm::GetWindowState(window.get())->IsMaximized());
  wm::GetWindowState(window.get())->Restore();
  EXPECT_EQ(rect.ToString(), window->bounds().ToString());
}

// Test that minimized windows do neither get maximized nor restored upon
// entering or leaving the maximized mode.
TEST_F(MaximizeModeWindowManagerTest, MinimizedWindowBehavior) {
  // Bounds for windows we know can be controlled.
  gfx::Rect rect(10, 10, 200, 50);
  scoped_ptr<aura::Window> initially_minimized_window(
      CreateWindow(ui::wm::WINDOW_TYPE_NORMAL, rect));
  scoped_ptr<aura::Window> initially_normal_window(
      CreateWindow(ui::wm::WINDOW_TYPE_NORMAL, rect));
  scoped_ptr<aura::Window> initially_maximized_window(
      CreateWindow(ui::wm::WINDOW_TYPE_NORMAL, rect));
  wm::GetWindowState(initially_minimized_window.get())->Minimize();
  wm::GetWindowState(initially_maximized_window.get())->Maximize();

  // Create the manager and make sure that the window gets detected.
  ash::internal::MaximizeModeWindowManager* manager =
      CreateMaximizeModeWindowManager();
  ASSERT_TRUE(manager);
  EXPECT_EQ(3, manager->GetNumberOfManagedWindows());
  EXPECT_TRUE(wm::GetWindowState(
      initially_minimized_window.get())->IsMinimized());
  EXPECT_TRUE(wm::GetWindowState(initially_normal_window.get())->IsMaximized());
  EXPECT_TRUE(wm::GetWindowState(
      initially_maximized_window.get())->IsMaximized());
  // Now minimize the second window to check that upon leaving the window
  // remains in its minimized state.
  wm::GetWindowState(initially_normal_window.get())->Minimize();
  wm::GetWindowState(initially_maximized_window.get())->Minimize();
  EXPECT_TRUE(wm::GetWindowState(
      initially_minimized_window.get())->IsMinimized());
  EXPECT_TRUE(wm::GetWindowState(
      initially_normal_window.get())->IsMinimized());
  EXPECT_TRUE(wm::GetWindowState(
      initially_maximized_window.get())->IsMinimized());

  // Destroy the manager again and check that the window will remain minimized.
  DestroyMaximizeModeWindowManager();
  EXPECT_TRUE(wm::GetWindowState(
      initially_minimized_window.get())->IsMinimized());
  EXPECT_TRUE(wm::GetWindowState(initially_normal_window.get())->IsMinimized());
  EXPECT_TRUE(wm::GetWindowState(
      initially_maximized_window.get())->IsMinimized());
}

// Check that resizing the desktop does reposition unmaximizable & managed
// windows.
TEST_F(MaximizeModeWindowManagerTest, DesktopSizeChangeMovesUnmaximizable) {
  gfx::Rect rect(20, 140, 100, 100);
  scoped_ptr<aura::Window> window(
      CreateNonMaximizableWindow(ui::wm::WINDOW_TYPE_NORMAL, rect));
  EXPECT_EQ(rect.ToString(), window->bounds().ToString());

  // Turning on the manager will reposition (but not resize) the window.
  ash::internal::MaximizeModeWindowManager* manager =
      CreateMaximizeModeWindowManager();
  ASSERT_TRUE(manager);
  EXPECT_EQ(1, manager->GetNumberOfManagedWindows());
  gfx::Rect moved_bounds(window->bounds());
  EXPECT_NE(rect.origin().ToString(), moved_bounds.origin().ToString());
  EXPECT_EQ(rect.size().ToString(), moved_bounds.size().ToString());

  // Simulating a desktop resize should move the window again.
  ResizeDesktop(-10);
  gfx::Rect new_moved_bounds(window->bounds());
  EXPECT_NE(rect.origin().ToString(), new_moved_bounds.origin().ToString());
  EXPECT_EQ(rect.size().ToString(), new_moved_bounds.size().ToString());
  EXPECT_NE(moved_bounds.origin().ToString(), new_moved_bounds.ToString());

  // Turning off the mode however should restore to the initial coordinates.
  DestroyMaximizeModeWindowManager();
  EXPECT_EQ(rect.ToString(), window->bounds().ToString());
}

// Check that enabling of the maximize mode does not have an impact on the MRU
// order of windows.
TEST_F(MaximizeModeWindowManagerTest, ModeChangeKeepsMRUOrder) {
  gfx::Rect rect(20, 140, 100, 100);
  scoped_ptr<aura::Window> w1(
      CreateNonMaximizableWindow(ui::wm::WINDOW_TYPE_NORMAL, rect));
  scoped_ptr<aura::Window> w2(CreateWindow(ui::wm::WINDOW_TYPE_NORMAL, rect));
  scoped_ptr<aura::Window> w3(CreateWindow(ui::wm::WINDOW_TYPE_NORMAL, rect));
  scoped_ptr<aura::Window> w4(
      CreateNonMaximizableWindow(ui::wm::WINDOW_TYPE_NORMAL, rect));
  scoped_ptr<aura::Window> w5(CreateWindow(ui::wm::WINDOW_TYPE_NORMAL, rect));

  // The windows should be in the reverse order of creation in the MRU list.
  {
    MruWindowTracker::WindowList windows =
        MruWindowTracker::BuildWindowList(false);
    EXPECT_EQ(w1.get(), windows[4]);
    EXPECT_EQ(w2.get(), windows[3]);
    EXPECT_EQ(w3.get(), windows[2]);
    EXPECT_EQ(w4.get(), windows[1]);
    EXPECT_EQ(w5.get(), windows[0]);
  }

  // Activating the window manager should keep the order.
  ash::internal::MaximizeModeWindowManager* manager =
      CreateMaximizeModeWindowManager();
  ASSERT_TRUE(manager);
  EXPECT_EQ(5, manager->GetNumberOfManagedWindows());
  {
    MruWindowTracker::WindowList windows =
        MruWindowTracker::BuildWindowList(false);
    // We do not test maximization here again since that was done already.
    EXPECT_EQ(w1.get(), windows[4]);
    EXPECT_EQ(w2.get(), windows[3]);
    EXPECT_EQ(w3.get(), windows[2]);
    EXPECT_EQ(w4.get(), windows[1]);
    EXPECT_EQ(w5.get(), windows[0]);
  }

  // Destroying should still keep the order.
  DestroyMaximizeModeWindowManager();
  {
    MruWindowTracker::WindowList windows =
        MruWindowTracker::BuildWindowList(false);
    // We do not test maximization here again since that was done already.
    EXPECT_EQ(w1.get(), windows[4]);
    EXPECT_EQ(w2.get(), windows[3]);
    EXPECT_EQ(w3.get(), windows[2]);
    EXPECT_EQ(w4.get(), windows[1]);
    EXPECT_EQ(w5.get(), windows[0]);
  }
}

}  // namespace ash
