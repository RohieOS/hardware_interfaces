/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "light_hidl_hal_test"

#include <android-base/logging.h>
#include <android/hardware/light/2.0/ILight.h>
#include <android/hardware/light/2.0/types.h>
#include <gtest/gtest.h>
#include <set>
#include <unistd.h>

using ::android::hardware::light::V2_0::Brightness;
using ::android::hardware::light::V2_0::Flash;
using ::android::hardware::light::V2_0::ILight;
using ::android::hardware::light::V2_0::LightState;
using ::android::hardware::light::V2_0::Status;
using ::android::hardware::light::V2_0::Type;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

#define LIGHT_SERVICE_NAME "light"

#define ASSERT_OK(ret) ASSERT_TRUE(ret.isOk())
#define EXPECT_OK(ret) EXPECT_TRUE(ret.isOk())

class LightHidlTest : public ::testing::Test {
public:
    virtual void SetUp() override {
        light = ILight::getService(LIGHT_SERVICE_NAME);

        ASSERT_NE(light, nullptr);
        LOG(INFO) << "Test is remote " << light->isRemote();

        ASSERT_OK(light->getSupportedTypes([this](const hidl_vec<Type> &types) {
            supportedTypes = types;
        }));
    }

    virtual void TearDown() override {}

    sp<ILight> light;
    std::vector<Type> supportedTypes;
};

class LightHidlEnvironment : public ::testing::Environment {
public:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

const static LightState kWhite = {
    .color = 0xFFFFFFFF,
    .flashMode = Flash::TIMED,
    .flashOnMs = 100,
    .flashOffMs = 50,
    .brightnessMode = Brightness::USER,
};

const static LightState kLowPersistance = {
    .color = 0xFF123456,
    .flashMode = Flash::TIMED,
    .flashOnMs = 100,
    .flashOffMs = 50,
    .brightnessMode = Brightness::LOW_PERSISTENCE,
};

const static LightState kOff = {
    .color = 0x00000000,
    .flashMode = Flash::NONE,
    .flashOnMs = 0,
    .flashOffMs = 0,
    .brightnessMode = Brightness::USER,
};

const static std::set<Type> kAllTypes = {
    Type::BACKLIGHT,
    Type::KEYBOARD,
    Type::BUTTONS,
    Type::BATTERY,
    Type::NOTIFICATIONS,
    Type::ATTENTION,
    Type::BLUETOOTH,
    Type::WIFI
};

/**
 * Ensure all lights which are reported as supported work.
 */
TEST_F(LightHidlTest, TestSupported) {
    for (const Type& type: supportedTypes) {
        Return<Status> ret = light->setLight(type, kWhite);
        EXPECT_OK(ret);
        EXPECT_EQ(Status::SUCCESS, static_cast<Status>(ret));
    }

    for (const Type& type: supportedTypes) {
        Return<Status> ret = light->setLight(type, kOff);
        EXPECT_OK(ret);
        EXPECT_EQ(Status::SUCCESS, static_cast<Status>(ret));
    }
}

/**
 * Ensure BRIGHTNESS_NOT_SUPPORTED is returned if LOW_PERSISTANCE is not supported.
 */
TEST_F(LightHidlTest, TestLowPersistance) {
    for (const Type& type: supportedTypes) {
        Return<Status> ret = light->setLight(type, kLowPersistance);
        EXPECT_OK(ret);

        Status status = ret;
        EXPECT_TRUE(Status::SUCCESS == status ||
                    Status::BRIGHTNESS_NOT_SUPPORTED == status);
    }

    for (const Type& type: supportedTypes) {
        Return<Status> ret = light->setLight(type, kOff);
        EXPECT_OK(ret);
        EXPECT_EQ(Status::SUCCESS, static_cast<Status>(ret));
    }
}

/**
 * Ensure lights which are not supported return LIGHT_NOT_SUPPORTED
 */
TEST_F(LightHidlTest, TestUnsupported) {
    std::set<Type> unsupportedTypes = kAllTypes;
    for (const Type& type: supportedTypes) {
        unsupportedTypes.erase(type);
    }

    for (const Type& type: unsupportedTypes) {
        Return<Status> ret = light->setLight(type, kWhite);
        EXPECT_OK(ret);
        EXPECT_EQ(Status::LIGHT_NOT_SUPPORTED, static_cast<Status>(ret));
    }
}

int main(int argc, char **argv) {
    ::testing::AddGlobalTestEnvironment(new LightHidlEnvironment);
    ::testing::InitGoogleTest(&argc, argv);
    int status = RUN_ALL_TESTS();
    LOG(INFO) << "Test result = " << status;
    return status;
}
