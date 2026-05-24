#include "config/ConfigLoader.h"
#include <fstream>
#include <gtest/gtest.h>

using namespace keyflow;

// Helper to get test config directory
std::string getTestConfigPath(const std::string &filename) {
  return "fixtures/test_configs/" + filename;
}

// ===== Load From File Tests =====

TEST(ConfigLoaderTest, LoadSimpleConfig) {
  try {
    JsonConfig config =
        ConfigLoader::loadFromFile(getTestConfigPath("simple.json"));

    EXPECT_FALSE(config.remapping.empty());
  } catch (const std::exception &e) {
    FAIL() << "Failed to load simple config: " << e.what();
  }
}

TEST(ConfigLoaderTest, LoadComplexConfig) {
  try {
    JsonConfig config =
        ConfigLoader::loadFromFile(getTestConfigPath("complex.json"));

    EXPECT_FALSE(config.remapping.empty());
    EXPECT_FALSE(config.layers.empty());
  } catch (const std::exception &e) {
    FAIL() << "Failed to load complex config: " << e.what();
  }
}

TEST(ConfigLoaderTest, LoadNonExistentFile) {
  EXPECT_THROW(
      {
        [[maybe_unused]] auto config =
            ConfigLoader::loadFromFile("nonexistent.json");
      },
      std::runtime_error);
}

TEST(ConfigLoaderTest, LoadInvalidJSON) {
  // Load succeeds but validation should fail
  try {
    JsonConfig config =
        ConfigLoader::loadFromFile(getTestConfigPath("invalid.json"));
    ValidationResult result = ConfigLoader::validate(config);
    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.errors.empty());
  } catch (const std::exception &e) {
    FAIL() << "Unexpected exception: " << e.what();
  }
}

// ===== Load From String Tests =====

TEST(ConfigLoaderTest, LoadFromValidJSONString) {
  std::string jsonStr = R"({
        "version": "2.0",
        "remapping": {
            "A": "B"
        }
    })";

  try {
    JsonConfig config = ConfigLoader::loadFromString(jsonStr);
    EXPECT_FALSE(config.remapping.empty());
    EXPECT_EQ(config.remapping["A"], "B");
  } catch (const std::exception &e) {
    FAIL() << "Failed to load from string: " << e.what();
  }
}

TEST(ConfigLoaderTest, LoadFromInvalidJSONString) {
  std::string jsonStr = "{ invalid json }";
  EXPECT_THROW(
      { [[maybe_unused]] auto config = ConfigLoader::loadFromString(jsonStr); },
      std::runtime_error);
}

TEST(ConfigLoaderTest, LoadFromEmptyString) {
  std::string jsonStr = "";
  EXPECT_THROW(
      { [[maybe_unused]] auto config = ConfigLoader::loadFromString(jsonStr); },
      std::runtime_error);
}

// ===== Validation Tests =====

TEST(ConfigLoaderValidationTest, ValidConfigPassesValidation) {
  JsonConfig config;
  config.remapping["A"] = "B";

  ValidationResult result = ConfigLoader::validate(config);
  EXPECT_TRUE(result.valid);
  EXPECT_TRUE(result.errors.empty());
}

TEST(ConfigLoaderValidationTest, ValidationResultBoolConversion) {
  ValidationResult valid;
  valid.valid = true;
  EXPECT_TRUE(static_cast<bool>(valid));

  ValidationResult invalid;
  invalid.valid = false;
  EXPECT_FALSE(static_cast<bool>(invalid));
}

// Note: More comprehensive validation tests would require understanding
// the specific validation rules implemented in ConfigLoader. Tests above
// cover the basic loading and validation infrastructure.
