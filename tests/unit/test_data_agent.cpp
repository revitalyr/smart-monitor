#include <gtest/gtest.h>
#include "agent/data_agent.h"
#include "common/smart_monitor_constants.h"

class DataAgentTest : public ::testing::Test {
protected:
    void SetUp() override {
        config.enable_camera = true;
        config.enable_audio = true;
        config.enable_sensors = true;
        config.enable_simulation = true;
        config.update_interval_ms = 100;
        config.video_source = strdup("/dev/video0");
        config.motion_threshold = 0.3f;
        config.audio_threshold = 0.2f;
        
        agent = data_agent_create(&config);
        ASSERT_NE(agent, nullptr);
    }

    void TearDown() override {
        if (agent) {
            data_agent_destroy(agent);
        }
        if (config.video_source) {
            free(config.video_source);
        }
    }

    agent_config_t config;
    data_agent_t* agent;
};

TEST_F(DataAgentTest, CreateAndDestroy) {
    EXPECT_NE(agent, nullptr);
    EXPECT_FALSE(data_agent_is_running(agent));
}

TEST_F(DataAgentTest, StartAndStop) {
    bool start_result = data_agent_start(agent);
    EXPECT_TRUE(start_result);
    EXPECT_TRUE(data_agent_is_running(agent));
    
    data_agent_stop(agent);
    EXPECT_FALSE(data_agent_is_running(agent));
}

TEST_F(DataAgentTest, GetStatistics) {
    agent_stats_t stats = data_agent_get_stats(agent);
    
    EXPECT_EQ(stats.frames_processed, 0);
    EXPECT_EQ(stats.motion_events, 0);
    EXPECT_EQ(stats.audio_events, 0);
    EXPECT_EQ(stats.sensor_updates, 0);
    EXPECT_GT(stats.start_time, 0);
}

TEST_F(DataAgentTest, ResetStatistics) {
    data_agent_start(agent);
    
    // Let it run briefly
    usleep(200000); // 200ms
    
    agent_stats_t stats_before = data_agent_get_stats(agent);
    data_agent_reset_stats(agent);
    agent_stats_t stats_after = data_agent_get_stats(agent);
    
    EXPECT_EQ(stats_after.frames_processed, 0);
    EXPECT_EQ(stats_after.motion_events, 0);
    EXPECT_EQ(stats_after.audio_events, 0);
    EXPECT_EQ(stats_after.sensor_updates, 0);
    EXPECT_GT(stats_after.start_time, 0);
}

TEST_F(DataAgentTest, SetThresholds) {
    data_agent_set_motion_threshold(agent, 0.5f);
    data_agent_set_audio_threshold(agent, 0.4f);
    data_agent_set_update_interval(agent, 200);
    
    // These should not crash
    SUCCEED();
}

TEST_F(DataAgentTest, JSONGeneration) {
    char* sensor_json = data_agent_get_sensor_json(agent);
    char* audio_json = data_agent_get_audio_json(agent);
    char* video_json = data_agent_get_video_json(agent);
    char* system_json = data_agent_get_system_json(agent);
    
    EXPECT_NE(sensor_json, nullptr);
    EXPECT_NE(audio_json, nullptr);
    EXPECT_NE(video_json, nullptr);
    EXPECT_NE(system_json, nullptr);
    
    // Check that JSON strings are not empty
    EXPECT_GT(strlen(sensor_json), 0);
    EXPECT_GT(strlen(audio_json), 0);
    EXPECT_GT(strlen(video_json), 0);
    EXPECT_GT(strlen(system_json), 0);
    
    // Check basic JSON structure
    EXPECT_TRUE(strstr(sensor_json, "\"temperature\"") != nullptr);
    EXPECT_TRUE(strstr(audio_json, "\"noise_level\"") != nullptr);
    EXPECT_TRUE(strstr(video_json, "\"motion_prob\"") != nullptr);
    EXPECT_TRUE(strstr(system_json, "\"uptime\"") != nullptr);
    
    free(sensor_json);
    free(audio_json);
    free(video_json);
    free(system_json);
}

// Callback test helpers
static bool sensor_callback_called = false;
static bool audio_callback_called = false;
static bool system_callback_called = false;

static void test_sensor_callback(const sensor_data_t* data, void* user_data) {
    sensor_callback_called = true;
    EXPECT_NE(data, nullptr);
    EXPECT_GT(data->timestamp, 0);
}

static void test_audio_callback(const audio_data_t* data, void* user_data) {
    audio_callback_called = true;
    EXPECT_NE(data, nullptr);
    EXPECT_GT(data->timestamp, 0);
}

static void test_system_callback(const system_status_t* status, void* user_data) {
    system_callback_called = true;
    EXPECT_NE(status, nullptr);
    EXPECT_GT(status->uptime, 0);
}

TEST_F(DataAgentTest, CallbackRegistration) {
    // Reset flags
    sensor_callback_called = false;
    audio_callback_called = false;
    system_callback_called = false;
    
    // Register callbacks
    data_agent_set_sensor_callback(agent, test_sensor_callback, nullptr);
    data_agent_set_audio_callback(agent, test_audio_callback, nullptr);
    data_agent_set_status_callback(agent, test_system_callback, nullptr);
    
    // Start agent to trigger callbacks
    data_agent_start(agent);
    
    // Wait for callbacks
    usleep(300000); // 300ms
    
    data_agent_stop(agent);
    
    // Callbacks should be called if simulation is enabled
    if (config.enable_simulation) {
        EXPECT_TRUE(sensor_callback_called);
        EXPECT_TRUE(audio_callback_called);
        EXPECT_TRUE(system_callback_called);
    }
}

class DataAgentConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        memset(&config, 0, sizeof(config));
    }

    agent_config_t config;
};

TEST_F(DataAgentConfigTest, CreateWithNullConfig) {
    data_agent_t* agent = data_agent_create(nullptr);
    EXPECT_EQ(agent, nullptr);
}

TEST_F(DataAgentConfigTest, CreateWithDisabledModules) {
    config.enable_camera = false;
    config.enable_audio = false;
    config.enable_sensors = false;
    config.enable_simulation = false;
    
    data_agent_t* agent = data_agent_create(&config);
    EXPECT_NE(agent, nullptr);
    
    // Should start even with all modules disabled
    bool start_result = data_agent_start(agent);
    EXPECT_TRUE(start_result);
    
    data_agent_destroy(agent);
}
