#include "config/ConfigLoader.h"
#include <gtest/gtest.h>

using namespace keyflow;

namespace {

constexpr const char *kSimpleConfig = R"({
  "version": "2.0",
  "name": "Simple",
  "remapping": { "CapsLock": "Escape" }
})";

constexpr const char *kComplexConfig = R"({
  "version": "2.0",
  "remapping": { "CapsLock": "Escape", "A": "B" },
  "layers": [{
    "name": "nav",
    "triggers": ["RightAlt"],
    "mappings": { "H": "Left", "J": "Down" }
  }],
  "noModCombos": [{ "key": "1", "output": "Exclaim", "shift": true }]
})";

constexpr const char *kInvalidKeyConfig = R"({
  "version": "2.0",
  "remapping": { "INVALID_KEY_NAME_123": "A" }
})";

} // namespace

TEST(ConfigLoaderTest, LoadsSimpleConfigFromString) {
  JsonConfig config = ConfigLoader::loadFromString(kSimpleConfig);
  EXPECT_EQ(config.name, "Simple");
  EXPECT_EQ(config.remapping["CapsLock"], "Escape");
}

TEST(ConfigLoaderTest, LoadsComplexConfigFromString) {
  JsonConfig config = ConfigLoader::loadFromString(kComplexConfig);
  EXPECT_EQ(config.remapping.size(), 2u);
  EXPECT_EQ(config.layers.size(), 1u);
  EXPECT_EQ(config.noModCombos.size(), 1u);
}

TEST(ConfigLoaderTest, NonExistentFileThrows) {
  EXPECT_THROW(
      {
        [[maybe_unused]] auto c =
            ConfigLoader::loadFromFile("nonexistent.json");
      },
      std::runtime_error);
}

TEST(ConfigLoaderTest, MalformedJSONThrows) {
  EXPECT_THROW(
      {
        [[maybe_unused]] auto c =
            ConfigLoader::loadFromString("{ invalid json }");
      },
      std::runtime_error);
}

TEST(ConfigLoaderTest, EmptyStringThrows) {
  EXPECT_THROW(
      { [[maybe_unused]] auto c = ConfigLoader::loadFromString(""); },
      std::runtime_error);
}

TEST(ConfigLoaderValidationTest, ValidationCatchesUnknownKey) {
  JsonConfig config = ConfigLoader::loadFromString(kInvalidKeyConfig);
  ValidationResult result = ConfigLoader::validate(config);
  EXPECT_FALSE(result.valid);
  EXPECT_FALSE(result.errors.empty());
}

TEST(ConfigLoaderValidationTest, ValidConfigPasses) {
  JsonConfig config;
  config.remapping["A"] = "B";
  EXPECT_TRUE(ConfigLoader::validate(config).valid);
}
