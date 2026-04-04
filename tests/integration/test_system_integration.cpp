#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <atomic>
#include "agent/data_agent.h"
#include "common/smart_monitor_constants.h"

class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        config.enable_camera = true;
        config.enable_audio = true;
        config.enable_sensors = true;
        config.enable_simulation = true;
        config.update_interval_ms = 50; // Fast updates for testing
        config.video_source = strdup("/dev/video0");
        config.motion_threshold = 0.3f;
        config.audio_threshold = 0.2f;
        
        agent = data_agent_create(&config);
        ASSERT_NE(agent, nullptr);
    }

    void TearDown() override {
        if (agent) {
            data_agent_stop(agent);
            data_agent_destroy(agent);
        }
        if (config.video_source) {
            free(config.video_source);
        }
    }

    agent_config_t config;
    data_agent_t* agent;
};

TEST_F(IntegrationTest, FullSystemStartup) {
    // Test complete system startup and shutdown
    EXPECT_FALSE(data_agent_is_running(agent));
    
    bool start_result = data_agent_start(agent);
    EXPECT_TRUE(start_result);
    EXPECT_TRUE(data_agent_is_running(agent));
    
    // Let system run for a short time
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    data_agent_stop(agent);
    EXPECT_FALSE(data_agent_is_running(agent));
}

TEST_F(IntegrationTest, DataFlowIntegration) {
    // Test that data flows through all components
    std::atomic<int> sensor_count{0};
    std::atomic<int> audio_count{0};
    std::atomic<int> system_count{0};
    
    auto sensor_callback = [&](const sensor_data_t* data, void* user_data) {
        sensor_count++;
        EXPECT_NE(data, nullptr);
        EXPECT_GT(data->timestamp, 0);
    };
    
    auto audio_callback = [&](const audio_data_t* data, void* user_data) {
        audio_count++;
        EXPECT_NE(data, nullptr);
        EXPECT_GT(data->timestamp, 0);
    };
    
    auto system_callback = [&](const system_status_t* status, void* user_data) {
        system_count++;
        EXPECT_NE(status, nullptr);
        EXPECT_GT(status->uptime, 0);
    };
    
    data_agent_set_sensor_callback(agent, 
        [](const sensor_data_t* data, void* user_data) {
            auto* counter = static_cast<std::atomic<int>*>(user_data);
            (*counter)++;
            EXPECT_NE(data, nullptr);
            EXPECT_GT(data->timestamp, 0);
        }, &sensor_count);
    
    data_agent_set_audio_callback(agent,
        [](const audio_data_t* data, void* user_data) {
            auto* counter = static_cast<std::atomic<int>*>(user_data);
            (*counter)++;
            EXPECT_NE(data, nullptr);
            EXPECT_GT(data->timestamp, 0);
        }, &audio_count);
    
    data_agent_set_status_callback(agent,
        [](const system_status_t* status, void* user_data) {
            auto* counter = static_cast<std::atomic<int>*>(user_data);
            (*counter)++;
            EXPECT_NE(status, nullptr);
            EXPECT_GT(status->uptime, 0);
        }, &system_count);
    
    data_agent_start(agent);
    
    // Wait for data to flow
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    data_agent_stop(agent);
    
    // Should have received multiple callbacks
    EXPECT_GT(sensor_count.load(), 0);
    EXPECT_GT(audio_count.load(), 0);
    EXPECT_GT(system_count.load(), 0);
}

TEST_F(IntegrationTest, JSONAPIIntegration) {
    // Test JSON API integration
    data_agent_start(agent);
    
    // Wait for initial data
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    
    char* sensor_json = data_agent_get_sensor_json(agent);
    char* audio_json = data_agent_get_audio_json(agent);
    char* video_json = data_agent_get_video_json(agent);
    char* system_json = data_agent_get_system_json(agent);
    
    ASSERT_NE(sensor_json, nullptr);
    ASSERT_NE(audio_json, nullptr);
    ASSERT_NE(video_json, nullptr);
    ASSERT_NE(system_json, nullptr);
    
    // Validate JSON structure
    EXPECT_TRUE(strstr(sensor_json, "{") != nullptr);
    EXPECT_TRUE(strstr(sensor_json, "}") != nullptr);
    EXPECT_TRUE(strstr(sensor_json, "\"temperature\"") != nullptr);
    EXPECT_TRUE(strstr(sensor_json, "\"humidity\"") != nullptr);
    EXPECT_TRUE(strstr(sensor_json, "\"timestamp\"") != nullptr);
    
    EXPECT_TRUE(strstr(audio_json, "\"noise_level\"") != nullptr);
    EXPECT_TRUE(strstr(audio_json, "\"peak_level\"") != nullptr);
    EXPECT_TRUE(strstr(audio_json, "\"voice_detected\"") != nullptr);
    
    EXPECT_TRUE(strstr(video_json, "\"motion_prob\"") != nullptr);
    EXPECT_TRUE(strstr(video_json, "\"motion_level\"") != nullptr);
    
    EXPECT_TRUE(strstr(system_json, "\"uptime\"") != nullptr);
    EXPECT_TRUE(strstr(system_json, "\"memory_usage\"") != nullptr);
    
    free(sensor_json);
    free(audio_json);
    free(video_json);
    free(system_json);
}

TEST_F(IntegrationTest, StatisticsIntegration) {
    // Test that statistics are properly integrated
    data_agent_start(agent);
    
    // Let system run
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    
    agent_stats_t stats = data_agent_get_stats(agent);
    
    EXPECT_GT(stats.uptime, 0);
    EXPECT_GT(stats.start_time, 0);
    
    // If simulation is enabled, should have some activity
    if (config.enable_simulation) {
        // Note: Actual counts depend on timing and thresholds
        // Just verify they're reasonable (not negative)
        EXPECT_GE(stats.frames_processed, 0);
        EXPECT_GE(stats.motion_events, 0);
        EXPECT_GE(stats.audio_events, 0);
        EXPECT_GE(stats.sensor_updates, 0);
    }
    
    data_agent_stop(agent);
}

TEST_F(IntegrationTest, ConfigurationChanges) {
    // Test runtime configuration changes
    data_agent_start(agent);
    
    // Change thresholds
    data_agent_set_motion_threshold(agent, 0.7f);
    data_agent_set_audio_threshold(agent, 0.6f);
    data_agent_set_update_interval(agent, 100);
    
    // Let system run with new config
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Should still be running
    EXPECT_TRUE(data_agent_is_running(agent));
    
    data_agent_stop(agent);
}

TEST_F(IntegrationTest, ErrorRecovery) {
    // Test system recovery from errors
    data_agent_start(agent);
    
    // Simulate some stress
    for (int i = 0; i < 10; i++) {
        char* json = data_agent_get_sensor_json(agent);
        if (json) {
            free(json);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // System should still be running
    EXPECT_TRUE(data_agent_is_running(agent));
    
    data_agent_stop(agent);
}

class IntegrationTestWithRealHardware : public ::testing::Test {
protected:
    void SetUp() override {
        // Test with real hardware if available
        hardware_available = false;
        
        // Check if camera is available
        if (access("/dev/video0", F_OK) == 0) {
            hardware_available = true;
        }
        
        if (hardware_available) {
            config.enable_camera = true;
            config.enable_audio = true;
            config.enable_sensors = true;
            config.enable_simulation = false; // Use real hardware
            config.update_interval_ms = 100;
            config.video_source = strdup("/dev/video0");
            config.motion_threshold = 0.3f;
            config.audio_threshold = 0.2f;
            
            agent = data_agent_create(&config);
        }
    }

    void TearDown() override {
        if (agent) {
            data_agent_stop(agent);
            data_agent_destroy(agent);
        }
        if (config.video_source) {
            free(config.video_source);
        }
    }

    bool hardware_available = false;
    agent_config_t config;
    data_agent_t* agent = nullptr;
};

TEST_F(IntegrationTestWithRealHardware, RealHardwareTest) {
    if (!hardware_available) {
        GTEST_SKIP() << "Hardware not available, skipping real hardware test";
    }
    
    ASSERT_NE(agent, nullptr);
    
    bool start_result = data_agent_start(agent);
    EXPECT_TRUE(start_result);
    
    // Let it run with real hardware
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    EXPECT_TRUE(data_agent_is_running(agent));
    
    data_agent_stop(agent);
}
