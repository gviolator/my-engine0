// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#ifdef _WIN32
    #include "my/platform/windows/windows_headers.h"
#endif

#ifdef Yield
    #undef Yield
#endif

#ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wdeprecated-copy"
#endif

#include <gmock/gmock.h>
#include <gtest/gtest-param-test.h>
#include <gtest/gtest.h>

#ifdef __clang__
    #pragma clang diagnostic pop
#endif

#include <string>
#include <string_view>

#include "my/diag/assert.h"
#include "my/dispatch/class_descriptor_builder.h"
#include "my/meta/common_attributes.h"
