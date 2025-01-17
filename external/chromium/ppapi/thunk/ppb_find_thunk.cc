// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// From dev/ppb_find_dev.idl modified Mon Nov 26 10:29:49 2012.

#include "ppapi/c/dev/ppb_find_dev.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/shared_impl/tracked_callback.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppb_instance_api.h"
#include "ppapi/thunk/resource_creation_api.h"
#include "ppapi/thunk/thunk.h"

namespace ppapi {
namespace thunk {

namespace {

void NumberOfFindResultsChanged(PP_Instance instance,
                                int32_t total,
                                PP_Bool final_result) {
  EnterInstance enter(instance);
  if (enter.succeeded())
    enter.functions()->NumberOfFindResultsChanged(instance,
                                                  total,
                                                  final_result);
}

void SelectedFindResultChanged(PP_Instance instance, int32_t index) {
  EnterInstance enter(instance);
  if (enter.succeeded())
    enter.functions()->SelectedFindResultChanged(instance, index);
}

const PPB_Find_Dev_0_3 g_ppb_find_dev_thunk_0_3 = {
  &NumberOfFindResultsChanged,
  &SelectedFindResultChanged,
};

}  // namespace

const PPB_Find_Dev_0_3* GetPPB_Find_Dev_0_3_Thunk() {
  return &g_ppb_find_dev_thunk_0_3;
}

}  // namespace thunk
}  // namespace ppapi
