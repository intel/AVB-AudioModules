/*
* INTEL CONFIDENTIAL
* Copyright (c) 2015-2017 Intel Corporation
* All Rights Reserved.
*
* The source code contained or described herein and all documents related to
* the source code ("Material") are owned by Intel Corporation or its
* suppliers or licensors. Title to the Material remains with Intel
* Corporation or its suppliers and licensors. The Material may contain trade
* secrets and proprietary and confidential information of Intel Corporation
* and its suppliers and licensors, and is protected by worldwide copyright
* and trade secret laws and treaty provisions. No part of the Material may be
* used, copied, reproduced, modified, published, uploaded, posted,
* transmitted, distributed, or disclosed in any way without Intel's prior
* express written permission.
*
* No license under any patent, copyright, trade secret or other intellectual
* property right is granted to or conferred upon you by disclosure or
* delivery of the Materials, either expressly, by implication, inducement,
* estoppel or otherwise. Any license under such intellectual property rights
* must be express and approved by Intel in writing.
*
* Unless otherwise agreed by Intel in writing, you may not remove or alter
* this notice or any other notice embedded in Materials by Intel or Intel's
* suppliers or licensors in any way.
 */

#include <internal/audio/common/IasAudioLogging.hpp>
#include <utils/String8.h>
#include <log/log.h>
#include <log/logger.h>
#include <log/log_read.h>
#include <log/logprint.h>
#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <cstdlib>
#include <fcntl.h>
#include <cassert>

using namespace IasAudio;

static const std::string cClassName = "AudioLoggingTest::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(60) + "):"

TEST(audio_logging_wrapper, basic_unit_test)
{
   static const std::string logTag("TST");

   pid_t pid = getpid();
   struct logger_list *logger_list;
   logger_list = android_logger_list_open(
       LOG_ID_MAIN, ANDROID_LOG_RDONLY | ANDROID_LOG_NONBLOCK, 1000, pid);

   DltContext *myLogCtx = IasAudioLogging::registerDltContext(logTag, "my test log context");

   ASSERT_NE(myLogCtx, nullptr);

   uint32_t myTestValue = 666;
   std::string myStringValue("Devil");
   bool myBoolValue = false;
   float myFloatValue = 21.666;
   DLT_LOG_CXX(*myLogCtx, DLT_LOG_ERROR, LOG_PREFIX, "my uint32_t val: ", myTestValue,
               ", my string val: ", myStringValue,
               ", my bool value: ", myBoolValue,
               ", my float value:", myFloatValue);

   static const std::string myTestLog{
               "AudioLoggingTest::virtual "
               "void audio_logging_wrapper_basic_unit_test_Test::TestBody()(60):my uint32_t val: 666, "
               "my string val: Devil, my bool value: 0, my float value:21.666"};

   log_msg log_msg;

   ASSERT_LT(0, android_logger_list_read(logger_list, &log_msg));

   ASSERT_EQ(log_msg.entry.pid, pid);

   AndroidLogFormat *logformat = android_log_format_new();
   AndroidLogEntry entry;
   int processLogBuffer = android_log_processLogBuffer(&log_msg.entry_v1, &entry);

   ASSERT_EQ(0, processLogBuffer);
   EXPECT_EQ(myTestLog.length(), entry.messageLen);
   EXPECT_EQ(myTestLog,  entry.message);
   EXPECT_EQ(ANDROID_LOG_ERROR, entry.priority);
   EXPECT_EQ(logTag, entry.tag);

   android_log_format_free(logformat);
   android_logger_list_close(logger_list);

   delete myLogCtx;
}
