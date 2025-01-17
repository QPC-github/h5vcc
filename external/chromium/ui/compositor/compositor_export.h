// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_COMPOSITOR_COMPOSITOR_EXPORT_H_
#define UI_COMPOSITOR_COMPOSITOR_EXPORT_H_

#if defined(COMPONENT_BUILD)
#if defined(_MSC_VER)

#if defined(COMPOSITOR_IMPLEMENTATION)
#define COMPOSITOR_EXPORT __declspec(dllexport)
#else
#define COMPOSITOR_EXPORT __declspec(dllimport)
#endif  // defined(COMPOSITOR_IMPLEMENTATION)

#else  // defined(WIN32)
#if defined(COMPOSITOR_IMPLEMENTATION)
#define COMPOSITOR_EXPORT __attribute__((visibility("default")))
#else
#define COMPOSITOR_EXPORT
#endif
#endif

#else  // defined(COMPONENT_BUILD)
#define COMPOSITOR_EXPORT
#endif

#endif  // UI_COMPOSITOR_COMPOSITOR_EXPORT_H_
