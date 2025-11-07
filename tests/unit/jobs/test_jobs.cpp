// Copyright (c) 2025 Calvin Min
// SPDX-License-Identifier: MIT
#include <databricks/core/config.h>
#include <databricks/jobs/jobs.h>
#include <gtest/gtest.h>

// Test fixture for Jobs tests
class JobsTest : public ::testing::Test {
protected:
    databricks::AuthConfig auth;

    void SetUp() override {
        auth.host = "https://test.databricks.com";
        auth.set_token("test_token");
        auth.timeout_seconds = 30;
    }
};

// Test: Jobs client construction
TEST_F(JobsTest, ConstructorCreatesValidClient) {
    ASSERT_NO_THROW({ databricks::Jobs jobs(auth); });
}

// Test: Job struct initialization and default values
TEST(JobStructTest, DefaultInitialization) {
    databricks::Job job;

    EXPECT_EQ(job.job_id, 0);
    EXPECT_EQ(job.name, "");
    EXPECT_EQ(job.creator_user_name, "");
    EXPECT_EQ(job.created_time, 0);
    EXPECT_TRUE(job.settings.empty());
}

// Test: Job struct from valid JSON
TEST(JobStructTest, ParseValidJson) {
    std::string json = R"({
        "job_id": 123456789,
        "name": "Test Job",
        "creator_user_name": "user@example.com",
        "created_time": 1609459200000,
        "settings": {
            "timeout_seconds": 3600,
            "max_retries": 3
        }
    })";

    databricks::Job job = databricks::Job::from_json(json);

    EXPECT_EQ(job.job_id, 123456789);
    EXPECT_EQ(job.name, "Test Job");
    EXPECT_EQ(job.creator_user_name, "user@example.com");
    EXPECT_EQ(job.created_time, 1609459200000);
    EXPECT_FALSE(job.settings.empty());
    EXPECT_TRUE(job.settings.count("raw") > 0);
}

// Test: Job struct handles large job IDs (uint64_t)
TEST(JobStructTest, HandlesLargeJobIds) {
    std::string json = R"({
        "job_id": 18446744073709551615,
        "name": "Large ID Job",
        "creator_user_name": "user@example.com",
        "created_time": 1609459200000
    })";

    databricks::Job job = databricks::Job::from_json(json);

    // Should not overflow with uint64_t
    EXPECT_EQ(job.job_id, 18446744073709551615ULL);
    EXPECT_EQ(job.name, "Large ID Job");
}

// Test: Job struct handles missing optional fields
TEST(JobStructTest, HandlesMissingFields) {
    std::string json = R"({
        "job_id": 123,
        "name": "Minimal Job"
    })";

    ASSERT_NO_THROW({
        databricks::Job job = databricks::Job::from_json(json);
        EXPECT_EQ(job.job_id, 123);
        EXPECT_EQ(job.name, "Minimal Job");
        EXPECT_EQ(job.creator_user_name, "");
        EXPECT_EQ(job.created_time, 0);
    });
}

// Test: Job struct rejects invalid JSON
TEST(JobStructTest, RejectsInvalidJson) {
    std::string invalid_json = "not valid json";

    EXPECT_THROW({ databricks::Job::from_json(invalid_json); }, std::runtime_error);
}

// Test: JobRun struct initialization and default values
TEST(JobRunStructTest, DefaultInitialization) {
    databricks::JobRun run;

    EXPECT_EQ(run.run_id, 0);
    EXPECT_EQ(run.job_id, 0);
    EXPECT_EQ(run.state, "");
    EXPECT_EQ(run.start_time, 0);
    EXPECT_EQ(run.end_time, 0);
    EXPECT_EQ(run.result_state, "");
}

// Test: JobRun struct from valid JSON
TEST(JobRunStructTest, ParseValidJson) {
    std::string json = R"({
        "run_id": 987654321,
        "job_id": 123456789,
        "start_time": 1609459200000,
        "end_time": 1609462800000,
        "state": {
            "life_cycle_state": "TERMINATED",
            "result_state": "SUCCESS"
        }
    })";

    databricks::JobRun run = databricks::JobRun::from_json(json);

    EXPECT_EQ(run.run_id, 987654321);
    EXPECT_EQ(run.job_id, 123456789);
    EXPECT_EQ(run.start_time, 1609459200000);
    EXPECT_EQ(run.end_time, 1609462800000);
    EXPECT_EQ(run.state, "TERMINATED");
    EXPECT_EQ(run.result_state, "SUCCESS");
}

// Test: JobRun struct handles running job (no end_time)
TEST(JobRunStructTest, HandlesRunningJob) {
    std::string json = R"({
        "run_id": 987654321,
        "job_id": 123456789,
        "start_time": 1609459200000,
        "state": {
            "life_cycle_state": "RUNNING"
        }
    })";

    databricks::JobRun run = databricks::JobRun::from_json(json);

    EXPECT_EQ(run.run_id, 987654321);
    EXPECT_EQ(run.state, "RUNNING");
    EXPECT_EQ(run.end_time, 0);      // Running jobs have no end time
    EXPECT_EQ(run.result_state, ""); // No result yet
}

// Test: JobRun struct handles different lifecycle states
TEST(JobRunStructTest, HandlesDifferentStates) {
    std::vector<std::string> states = {"PENDING", "RUNNING", "TERMINATING", "TERMINATED", "SKIPPED", "INTERNAL_ERROR"};

    for (const auto& state : states) {
        std::string json = R"({"run_id": 1, "job_id": 1, "state": {"life_cycle_state": ")" + state + R"("}})";

        databricks::JobRun run = databricks::JobRun::from_json(json);
        EXPECT_EQ(run.state, state) << "Failed for state: " << state;
    }
}

// Test: JobRun struct handles different result states
TEST(JobRunStructTest, HandlesDifferentResultStates) {
    std::vector<std::string> results = {"SUCCESS", "FAILED", "TIMEDOUT", "CANCELED"};

    for (const auto& result : results) {
        std::string json = R"({
            "run_id": 1,
            "job_id": 1,
            "state": {
                "life_cycle_state": "TERMINATED",
                "result_state": ")" +
                           result + R"("
            }
        })";

        databricks::JobRun run = databricks::JobRun::from_json(json);
        EXPECT_EQ(run.result_state, result) << "Failed for result: " << result;
    }
}

// Test: JobRun struct handles large IDs (uint64_t)
TEST(JobRunStructTest, HandlesLargeIds) {
    std::string json = R"({
        "run_id": 18446744073709551615,
        "job_id": 18446744073709551614,
        "start_time": 1609459200000,
        "state": {"life_cycle_state": "RUNNING"}
    })";

    databricks::JobRun run = databricks::JobRun::from_json(json);

    EXPECT_EQ(run.run_id, 18446744073709551615ULL);
    EXPECT_EQ(run.job_id, 18446744073709551614ULL);
}

// Test: JobRun struct rejects invalid JSON
TEST(JobRunStructTest, RejectsInvalidJson) {
    std::string invalid_json = "invalid json";

    EXPECT_THROW({ databricks::JobRun::from_json(invalid_json); }, std::runtime_error);
}

// Test: Jobs client can be constructed with minimal config
TEST_F(JobsTest, MinimalConfigConstruction) {
    databricks::AuthConfig minimal_auth;
    minimal_auth.host = "https://minimal.databricks.com";
    minimal_auth.set_token("token");

    ASSERT_NO_THROW({ databricks::Jobs jobs(minimal_auth); });
}

// Test: Multiple Jobs clients can coexist
TEST_F(JobsTest, MultipleClientsCanCoexist) {
    databricks::AuthConfig auth1;
    auth1.host = "https://workspace1.databricks.com";
    auth1.set_token("token1");

    databricks::AuthConfig auth2;
    auth2.host = "https://workspace2.databricks.com";
    auth2.set_token("token2");

    ASSERT_NO_THROW({
        databricks::Jobs jobs1(auth1);
        databricks::Jobs jobs2(auth2);
        // Both should coexist without issues
    });
}
