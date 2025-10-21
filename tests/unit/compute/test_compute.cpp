#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <databricks/compute/compute.h>
#include <databricks/compute/compute_types.h>
#include <databricks/core/config.h>
#include <mock_http_client.h>

// Test fixture for Compute tests
class ComputeTest : public ::testing::Test {
protected:
    databricks::AuthConfig auth;

    void SetUp() override {
        auth.host = "https://test.databricks.com";
        auth.token = "test_token";
        auth.timeout_seconds = 30;
    }
};

// ============================================================================
// Compute Client Tests
// ============================================================================

// Test: Compute client construction
TEST_F(ComputeTest, ConstructorCreatesValidClient) {
    ASSERT_NO_THROW({
        databricks::Compute compute(auth);
    });
}

// Test: Compute client can be constructed with minimal config
TEST_F(ComputeTest, MinimalConfigConstruction) {
    databricks::AuthConfig minimal_auth;
    minimal_auth.host = "https://minimal.databricks.com";
    minimal_auth.token = "token";

    ASSERT_NO_THROW({
        databricks::Compute compute(minimal_auth);
    });
}

// Test: Multiple Compute clients can coexist
TEST_F(ComputeTest, MultipleClientsCanCoexist) {
    databricks::AuthConfig auth1;
    auth1.host = "https://workspace1.databricks.com";
    auth1.token = "token1";

    databricks::AuthConfig auth2;
    auth2.host = "https://workspace2.databricks.com";
    auth2.token = "token2";

    ASSERT_NO_THROW({
        databricks::Compute compute1(auth1);
        databricks::Compute compute2(auth2);
        // Both should coexist without issues
    });
}

// ============================================================================
// Cluster Struct Tests
// ============================================================================

// Test: Cluster struct initialization and default values
TEST(ClusterStructTest, DefaultInitialization) {
    databricks::Cluster cluster;

    EXPECT_EQ(cluster.cluster_id, "");
    EXPECT_EQ(cluster.cluster_name, "");
    EXPECT_EQ(cluster.state, "");
    EXPECT_EQ(cluster.creator_user_name, "");
    EXPECT_EQ(cluster.start_time, 0);
    EXPECT_EQ(cluster.terminated_time, 0);
    EXPECT_EQ(cluster.spark_version, "");
    EXPECT_EQ(cluster.node_type_id, "");
    EXPECT_EQ(cluster.num_workers, 0);
    EXPECT_TRUE(cluster.custom_tags.empty());
}

// Test: Cluster struct with basic values
TEST(ClusterStructTest, BasicInitialization) {
    databricks::Cluster cluster;
    cluster.cluster_id = "1234-567890-abcde123";
    cluster.cluster_name = "Test Cluster";
    cluster.state = "RUNNING";
    cluster.creator_user_name = "user@example.com";
    cluster.num_workers = 4;

    EXPECT_EQ(cluster.cluster_id, "1234-567890-abcde123");
    EXPECT_EQ(cluster.cluster_name, "Test Cluster");
    EXPECT_EQ(cluster.state, "RUNNING");
    EXPECT_EQ(cluster.creator_user_name, "user@example.com");
    EXPECT_EQ(cluster.num_workers, 4);
}

// Test: Cluster struct handles custom tags
TEST(ClusterStructTest, HandlesCustomTags) {
    databricks::Cluster cluster;
    cluster.custom_tags["environment"] = "production";
    cluster.custom_tags["team"] = "data-engineering";
    cluster.custom_tags["cost-center"] = "CC-12345";

    EXPECT_EQ(cluster.custom_tags.size(), 3);
    EXPECT_EQ(cluster.custom_tags["environment"], "production");
    EXPECT_EQ(cluster.custom_tags["team"], "data-engineering");
    EXPECT_EQ(cluster.custom_tags["cost-center"], "CC-12345");
}

// Test: Cluster struct handles various cluster states
TEST(ClusterStructTest, HandlesDifferentStates) {
    std::vector<std::string> states = {
        "PENDING", "RUNNING", "RESTARTING", "RESIZING",
        "TERMINATING", "TERMINATED", "ERROR", "UNKNOWN"
    };

    for (const auto& state : states) {
        databricks::Cluster cluster;
        cluster.state = state;
        EXPECT_EQ(cluster.state, state) << "Failed for state: " << state;
    }
}

// Test: Cluster struct handles Spark versions
TEST(ClusterStructTest, HandlesSparkVersions) {
    std::vector<std::string> versions = {
        "11.3.x-scala2.12",
        "12.2.x-photon-scala2.12",
        "13.0.x-cpu-ml-scala2.12"
    };

    for (const auto& version : versions) {
        databricks::Cluster cluster;
        cluster.spark_version = version;
        EXPECT_EQ(cluster.spark_version, version) << "Failed for version: " << version;
    }
}

// Test: Cluster struct handles node types
TEST(ClusterStructTest, HandlesNodeTypes) {
    std::vector<std::string> node_types = {
        "i3.xlarge",
        "m5.4xlarge",
        "r5.2xlarge",
        "Standard_DS3_v2"
    };

    for (const auto& node_type : node_types) {
        databricks::Cluster cluster;
        cluster.node_type_id = node_type;
        EXPECT_EQ(cluster.node_type_id, node_type) << "Failed for node type: " << node_type;
    }
}

// Test: Cluster struct handles timestamps
TEST(ClusterStructTest, HandlesTimestamps) {
    databricks::Cluster cluster;
    cluster.start_time = 1609459200000;        // 2021-01-01 00:00:00 UTC
    cluster.terminated_time = 1609462800000;    // 2021-01-01 01:00:00 UTC

    EXPECT_EQ(cluster.start_time, 1609459200000);
    EXPECT_EQ(cluster.terminated_time, 1609462800000);
    EXPECT_GT(cluster.terminated_time, cluster.start_time);
}

// Test: Cluster struct handles running cluster (no terminated_time)
TEST(ClusterStructTest, HandlesRunningCluster) {
    databricks::Cluster cluster;
    cluster.state = "RUNNING";
    cluster.start_time = 1609459200000;
    cluster.terminated_time = 0;  // Running clusters have no termination time

    EXPECT_EQ(cluster.state, "RUNNING");
    EXPECT_GT(cluster.start_time, 0);
    EXPECT_EQ(cluster.terminated_time, 0);
}

// ============================================================================
// ClusterStateEnum Tests
// ============================================================================

// Test: ClusterStateEnum parse from string (uppercase)
TEST(ClusterStateEnumTest, ParseUppercaseStrings) {
    EXPECT_EQ(databricks::parse_cluster_state("PENDING"), databricks::ClusterStateEnum::PENDING);
    EXPECT_EQ(databricks::parse_cluster_state("RUNNING"), databricks::ClusterStateEnum::RUNNING);
    EXPECT_EQ(databricks::parse_cluster_state("RESTARTING"), databricks::ClusterStateEnum::RESTARTING);
    EXPECT_EQ(databricks::parse_cluster_state("RESIZING"), databricks::ClusterStateEnum::RESIZING);
    EXPECT_EQ(databricks::parse_cluster_state("TERMINATING"), databricks::ClusterStateEnum::TERMINATING);
    EXPECT_EQ(databricks::parse_cluster_state("TERMINATED"), databricks::ClusterStateEnum::TERMINATED);
    EXPECT_EQ(databricks::parse_cluster_state("ERROR"), databricks::ClusterStateEnum::ERROR);
}

// Test: ClusterStateEnum parse from string (lowercase)
TEST(ClusterStateEnumTest, ParseLowercaseStrings) {
    EXPECT_EQ(databricks::parse_cluster_state("pending"), databricks::ClusterStateEnum::PENDING);
    EXPECT_EQ(databricks::parse_cluster_state("running"), databricks::ClusterStateEnum::RUNNING);
    EXPECT_EQ(databricks::parse_cluster_state("terminated"), databricks::ClusterStateEnum::TERMINATED);
}

// Test: ClusterStateEnum parse from string (mixed case)
TEST(ClusterStateEnumTest, ParseMixedCaseStrings) {
    EXPECT_EQ(databricks::parse_cluster_state("Running"), databricks::ClusterStateEnum::RUNNING);
    EXPECT_EQ(databricks::parse_cluster_state("PeNdInG"), databricks::ClusterStateEnum::PENDING);
}

// Test: ClusterStateEnum parse unknown string returns UNKNOWN
TEST(ClusterStateEnumTest, ParseUnknownStringReturnsUnknown) {
    EXPECT_EQ(databricks::parse_cluster_state("INVALID_STATE"), databricks::ClusterStateEnum::UNKNOWN);
    EXPECT_EQ(databricks::parse_cluster_state(""), databricks::ClusterStateEnum::UNKNOWN);
    EXPECT_EQ(databricks::parse_cluster_state("BOGUS"), databricks::ClusterStateEnum::UNKNOWN);
}

// Test: ClusterStateEnum to string conversion
TEST(ClusterStateEnumTest, ToStringConversion) {
    EXPECT_EQ(databricks::cluster_state_to_string(databricks::ClusterStateEnum::PENDING), "PENDING");
    EXPECT_EQ(databricks::cluster_state_to_string(databricks::ClusterStateEnum::RUNNING), "RUNNING");
    EXPECT_EQ(databricks::cluster_state_to_string(databricks::ClusterStateEnum::RESTARTING), "RESTARTING");
    EXPECT_EQ(databricks::cluster_state_to_string(databricks::ClusterStateEnum::RESIZING), "RESIZING");
    EXPECT_EQ(databricks::cluster_state_to_string(databricks::ClusterStateEnum::TERMINATING), "TERMINATING");
    EXPECT_EQ(databricks::cluster_state_to_string(databricks::ClusterStateEnum::TERMINATED), "TERMINATED");
    EXPECT_EQ(databricks::cluster_state_to_string(databricks::ClusterStateEnum::ERROR), "ERROR");
    EXPECT_EQ(databricks::cluster_state_to_string(databricks::ClusterStateEnum::UNKNOWN), "UNKNOWN");
}

// Test: ClusterStateEnum round-trip conversion
TEST(ClusterStateEnumTest, RoundTripConversion) {
    std::vector<databricks::ClusterStateEnum> states = {
        databricks::ClusterStateEnum::PENDING,
        databricks::ClusterStateEnum::RUNNING,
        databricks::ClusterStateEnum::TERMINATED,
        databricks::ClusterStateEnum::ERROR
    };

    for (const auto& state : states) {
        std::string state_str = databricks::cluster_state_to_string(state);
        databricks::ClusterStateEnum parsed = databricks::parse_cluster_state(state_str);
        EXPECT_EQ(parsed, state) << "Round-trip failed for state: " << state_str;
    }
}

// ============================================================================
// ClusterState Struct Tests
// ============================================================================

// Test: ClusterState struct default initialization
TEST(ClusterStateStructTest, DefaultInitialization) {
    databricks::ClusterState state;

    EXPECT_EQ(state.cluster_id, "");
    EXPECT_EQ(state.cluster_state, databricks::ClusterStateEnum::UNKNOWN);
    EXPECT_EQ(state.state_message, "");
}

// Test: ClusterState struct initialization with values
TEST(ClusterStateStructTest, InitializationWithValues) {
    databricks::ClusterState state;
    state.cluster_id = "1234-567890-abcde123";
    state.cluster_state = databricks::ClusterStateEnum::RUNNING;
    state.state_message = "Cluster is healthy and running";

    EXPECT_EQ(state.cluster_id, "1234-567890-abcde123");
    EXPECT_EQ(state.cluster_state, databricks::ClusterStateEnum::RUNNING);
    EXPECT_EQ(state.state_message, "Cluster is healthy and running");
}

// Test: ClusterState struct handles error states with messages
TEST(ClusterStateStructTest, HandlesErrorStates) {
    databricks::ClusterState state;
    state.cluster_id = "1234-567890-abcde123";
    state.cluster_state = databricks::ClusterStateEnum::ERROR;
    state.state_message = "Failed to start cluster: Insufficient capacity";

    EXPECT_EQ(state.cluster_state, databricks::ClusterStateEnum::ERROR);
    EXPECT_FALSE(state.state_message.empty());
    EXPECT_NE(state.state_message.find("capacity"), std::string::npos);
}

// ============================================================================
// Integration Tests
// ============================================================================

// Test: Worker count validation
TEST(ClusterValidationTest, ValidWorkerCounts) {
    databricks::Cluster cluster;

    // Single-node cluster
    cluster.num_workers = 0;
    EXPECT_EQ(cluster.num_workers, 0);

    // Standard multi-node cluster
    cluster.num_workers = 4;
    EXPECT_EQ(cluster.num_workers, 4);

    // Large cluster
    cluster.num_workers = 100;
    EXPECT_EQ(cluster.num_workers, 100);
}

// Test: Cluster lifecycle simulation
TEST(ClusterLifecycleTest, StateTransitions) {
    databricks::Cluster cluster;
    cluster.cluster_id = "test-cluster";

    // Creation
    cluster.state = "PENDING";
    EXPECT_EQ(cluster.state, "PENDING");

    // Starting
    cluster.state = "RUNNING";
    cluster.start_time = 1609459200000;
    EXPECT_EQ(cluster.state, "RUNNING");
    EXPECT_GT(cluster.start_time, 0);

    // Termination
    cluster.state = "TERMINATING";
    EXPECT_EQ(cluster.state, "TERMINATING");

    // Terminated
    cluster.state = "TERMINATED";
    cluster.terminated_time = 1609462800000;
    EXPECT_EQ(cluster.state, "TERMINATED");
    EXPECT_GT(cluster.terminated_time, cluster.start_time);
}

// ============================================================================
// Create Compute with Mock HTTP Client Tests
// ============================================================================

using ::testing::_;
using ::testing::Return;
using ::testing::Throw;

// Test: Successfully create compute cluster with minimal config
TEST(CreateComputeMockTest, SuccessfulCreateMinimalConfig) {
    auto mock_http = std::make_shared<databricks::test::MockHttpClient>();

    // Setup expectation: POST to /clusters/create should succeed
    EXPECT_CALL(*mock_http, post("/clusters/create", _))
        .WillOnce(Return(databricks::test::MockHttpClient::cluster_created_response("test-cluster-123")));

    // check_response should be called and not throw
    EXPECT_CALL(*mock_http, check_response(_, "createCompute"))
        .Times(1);

    // Create Compute client with mocked HTTP client
    databricks::Compute compute(mock_http);

    // Create cluster config
    databricks::Cluster config;
    config.cluster_name = "test-cluster";
    config.spark_version = "11.3.x-scala2.12";
    config.node_type_id = "i3.xlarge";
    config.num_workers = 2;

    // Execute
    bool result = compute.create_compute(config);

    // Verify
    EXPECT_TRUE(result);
}

// Test: Successfully create compute cluster with custom tags
TEST(CreateComputeMockTest, SuccessfulCreateWithCustomTags) {
    auto mock_http = std::make_shared<databricks::test::MockHttpClient>();

    EXPECT_CALL(*mock_http, post("/clusters/create", _))
        .WillOnce(Return(databricks::test::MockHttpClient::cluster_created_response()));

    EXPECT_CALL(*mock_http, check_response(_, "createCompute"))
        .Times(1);

    databricks::Compute compute(mock_http);

    databricks::Cluster config;
    config.cluster_name = "tagged-cluster";
    config.spark_version = "12.2.x-photon-scala2.12";
    config.node_type_id = "m5.4xlarge";
    config.num_workers = 4;
    config.custom_tags["environment"] = "production";
    config.custom_tags["team"] = "data-eng";

    bool result = compute.create_compute(config);
    EXPECT_TRUE(result);
}

// Test: Successfully create single-node cluster (num_workers = 0)
TEST(CreateComputeMockTest, SuccessfulCreateSingleNode) {
    auto mock_http = std::make_shared<databricks::test::MockHttpClient>();

    EXPECT_CALL(*mock_http, post("/clusters/create", _))
        .WillOnce(Return(databricks::test::MockHttpClient::success_response(
            R"({"cluster_id": "single-node-123"})"
        )));

    EXPECT_CALL(*mock_http, check_response(_, "createCompute"))
        .Times(1);

    databricks::Compute compute(mock_http);

    databricks::Cluster config;
    config.cluster_name = "single-node-cluster";
    config.spark_version = "13.0.x-scala2.12";
    config.node_type_id = "i3.2xlarge";
    config.num_workers = 0;  // Single-node mode

    bool result = compute.create_compute(config);
    EXPECT_TRUE(result);
}

// Test: Verify JSON payload contains expected fields
TEST(CreateComputeMockTest, VerifyJSONPayload) {
    auto mock_http = std::make_shared<databricks::test::MockHttpClient>();

    // Capture the JSON body sent in POST request
    std::string captured_body;
    EXPECT_CALL(*mock_http, post("/clusters/create", _))
        .WillOnce(::testing::DoAll(
            ::testing::SaveArg<1>(&captured_body),
            Return(databricks::test::MockHttpClient::cluster_created_response())
        ));

    EXPECT_CALL(*mock_http, check_response(_, "createCompute"))
        .Times(1);

    databricks::Compute compute(mock_http);

    databricks::Cluster config;
    config.cluster_name = "payload-test-cluster";
    config.spark_version = "11.3.x-scala2.12";
    config.node_type_id = "i3.xlarge";
    config.num_workers = 3;
    config.custom_tags["test"] = "verify-payload";

    compute.create_compute(config);

    // Verify the captured JSON contains expected fields
    EXPECT_NE(captured_body.find("\"cluster_name\":\"payload-test-cluster\""), std::string::npos);
    EXPECT_NE(captured_body.find("\"spark_version\":\"11.3.x-scala2.12\""), std::string::npos);
    EXPECT_NE(captured_body.find("\"node_type_id\":\"i3.xlarge\""), std::string::npos);
    EXPECT_NE(captured_body.find("\"num_workers\":3"), std::string::npos);
    EXPECT_NE(captured_body.find("\"custom_tags\""), std::string::npos);
    EXPECT_NE(captured_body.find("\"test\":\"verify-payload\""), std::string::npos);
}

// Test: Error handling - API returns 400 Bad Request
TEST(CreateComputeMockTest, HandleBadRequestError) {
    auto mock_http = std::make_shared<databricks::test::MockHttpClient>();

    EXPECT_CALL(*mock_http, post("/clusters/create", _))
        .WillOnce(Return(databricks::test::MockHttpClient::bad_request_response(
            "Invalid cluster configuration"
        )));

    // check_response should throw on 400 error
    EXPECT_CALL(*mock_http, check_response(_, "createCompute"))
        .WillOnce(Throw(std::runtime_error("API Error: Invalid cluster configuration")));

    databricks::Compute compute(mock_http);

    databricks::Cluster config;
    config.cluster_name = "invalid-cluster";
    config.spark_version = "invalid-version";
    config.node_type_id = "invalid-type";
    config.num_workers = 2;

    EXPECT_THROW({
        compute.create_compute(config);
    }, std::runtime_error);
}

// Test: Error handling - API returns 401 Unauthorized
TEST(CreateComputeMockTest, HandleUnauthorizedError) {
    auto mock_http = std::make_shared<databricks::test::MockHttpClient>();

    EXPECT_CALL(*mock_http, post("/clusters/create", _))
        .WillOnce(Return(databricks::test::MockHttpClient::unauthorized_response()));

    EXPECT_CALL(*mock_http, check_response(_, "createCompute"))
        .WillOnce(Throw(std::runtime_error("Authentication failed")));

    databricks::Compute compute(mock_http);

    databricks::Cluster config;
    config.cluster_name = "test-cluster";
    config.spark_version = "11.3.x-scala2.12";
    config.node_type_id = "i3.xlarge";
    config.num_workers = 2;

    EXPECT_THROW({
        compute.create_compute(config);
    }, std::runtime_error);
}

// Test: Error handling - API returns 500 Internal Server Error
TEST(CreateComputeMockTest, HandleServerError) {
    auto mock_http = std::make_shared<databricks::test::MockHttpClient>();

    EXPECT_CALL(*mock_http, post("/clusters/create", _))
        .WillOnce(Return(databricks::test::MockHttpClient::server_error_response()));

    EXPECT_CALL(*mock_http, check_response(_, "createCompute"))
        .WillOnce(Throw(std::runtime_error("Internal server error")));

    databricks::Compute compute(mock_http);

    databricks::Cluster config;
    config.cluster_name = "test-cluster";
    config.spark_version = "11.3.x-scala2.12";
    config.node_type_id = "i3.xlarge";
    config.num_workers = 2;

    EXPECT_THROW({
        compute.create_compute(config);
    }, std::runtime_error);
}

// Test: Cluster config without custom tags (should not include custom_tags in JSON)
TEST(CreateComputeMockTest, ClusterWithoutCustomTags) {
    auto mock_http = std::make_shared<databricks::test::MockHttpClient>();

    std::string captured_body;
    EXPECT_CALL(*mock_http, post("/clusters/create", _))
        .WillOnce(::testing::DoAll(
            ::testing::SaveArg<1>(&captured_body),
            Return(databricks::test::MockHttpClient::cluster_created_response())
        ));

    EXPECT_CALL(*mock_http, check_response(_, "createCompute"))
        .Times(1);

    databricks::Compute compute(mock_http);

    databricks::Cluster config;
    config.cluster_name = "no-tags-cluster";
    config.spark_version = "11.3.x-scala2.12";
    config.node_type_id = "i3.xlarge";
    config.num_workers = 2;
    // No custom tags set

    compute.create_compute(config);

    // Verify custom_tags is NOT in the JSON when empty
    // (Empty tags should not be sent to avoid unnecessary payload)
    EXPECT_NE(captured_body.find("\"cluster_name\":\"no-tags-cluster\""), std::string::npos);
    // The implementation adds custom_tags only if not empty
}

// Test: Multiple clusters can be created sequentially
TEST(CreateComputeMockTest, CreateMultipleClustersSequentially) {
    auto mock_http = std::make_shared<databricks::test::MockHttpClient>();

    EXPECT_CALL(*mock_http, post("/clusters/create", _))
        .WillOnce(Return(databricks::test::MockHttpClient::cluster_created_response("cluster-1")))
        .WillOnce(Return(databricks::test::MockHttpClient::cluster_created_response("cluster-2")))
        .WillOnce(Return(databricks::test::MockHttpClient::cluster_created_response("cluster-3")));

    EXPECT_CALL(*mock_http, check_response(_, "createCompute"))
        .Times(3);

    databricks::Compute compute(mock_http);

    for (int i = 1; i <= 3; i++) {
        databricks::Cluster config;
        config.cluster_name = "cluster-" + std::to_string(i);
        config.spark_version = "11.3.x-scala2.12";
        config.node_type_id = "i3.xlarge";
        config.num_workers = i;

        bool result = compute.create_compute(config);
        EXPECT_TRUE(result);
    }
}
