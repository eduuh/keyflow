#include "config/ConfigBuilder.h"
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

class ShippedConfigTest : public testing::TestWithParam<const char *> {};

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

TEST(ConfigLoaderTest, LayerMetadataIsNotAKeyMapping) {
  const auto config = ConfigLoader::loadFromString(R"({
    "version": "1.0",
    "layers": [{
      "name": "nav",
      "trigger": "RALT",
      "mappings": {
        "_comment": "Navigation keys",
        "_note": "Ignored metadata",
        "H": "Left"
      }
    }]
  })");

  ASSERT_EQ(config.layers.size(), 1u);
  EXPECT_EQ(config.layers.front().mappings.size(), 1u);
  EXPECT_EQ(config.layers.front().mappings.at("H"), "Left");
  EXPECT_TRUE(ConfigLoader::validate(config).valid);
  Pipeline pipeline;
  EXPECT_TRUE(ConfigBuilder::buildPipeline(config, pipeline, false));
}

TEST(ConfigLoaderValidationTest, DeclaredCustomLayerTriggerPasses) {
  const auto config = ConfigLoader::loadFromString(R"({
    "version": "1.0",
    "customModifiers": [{
      "key": "LeftWin",
      "modifierName": "WIN_MOD"
    }],
    "layers": [{
      "name": "Win",
      "trigger": "WIN_MOD",
      "mappings": { "J": "KP_7" }
    }]
  })");

  const auto result = ConfigLoader::validate(config);
  EXPECT_TRUE(result.valid) << testing::PrintToString(result.errors);
  Pipeline pipeline;
  EXPECT_TRUE(ConfigBuilder::buildPipeline(config, pipeline, false));
}

TEST(ConfigLoaderValidationTest, StandardModifierAliasesMatchBuilder) {
  const auto config = ConfigLoader::loadFromString(R"({
    "version": "1.0",
    "layers": [{
      "name": "nav",
      "triggers": ["RALT", "MOD11", "LALT", "MOD12", "MOD13", "PRINT"],
      "mappings": { "H": "Left" }
    }]
  })");

  const auto result = ConfigLoader::validate(config);
  EXPECT_TRUE(result.valid) << testing::PrintToString(result.errors);
  Pipeline pipeline;
  EXPECT_TRUE(ConfigBuilder::buildPipeline(config, pipeline, false));
}

TEST(ConfigLoaderValidationTest, UndeclaredLayerTriggerFails) {
  const auto config = ConfigLoader::loadFromString(R"({
    "version": "1.0",
    "layers": [{
      "name": "nav",
      "trigger": "UNKNOWN_MOD",
      "mappings": { "H": "Left" }
    }]
  })");

  const auto result = ConfigLoader::validate(config);
  EXPECT_FALSE(result.valid);
  ASSERT_EQ(result.errors.size(), 1u);
  EXPECT_EQ(result.errors.front().first, "layers[0].trigger");
}

TEST(ConfigLoaderValidationTest, NonModifierKeyCannotTriggerLayer) {
  const auto config = ConfigLoader::loadFromString(R"({
    "version": "1.0",
    "layers": [{
      "name": "nav",
      "trigger": "A",
      "mappings": { "H": "Left" }
    }]
  })");

  const auto result = ConfigLoader::validate(config);
  EXPECT_FALSE(result.valid);
  ASSERT_EQ(result.errors.size(), 1u);
  EXPECT_EQ(result.errors.front().first, "layers[0].trigger");
}

TEST_P(ShippedConfigTest, ValidatesAndBuildsUnchanged) {
  const auto config = ConfigLoader::loadFromFile(GetParam());

  const auto result = ConfigLoader::validate(config);
  EXPECT_TRUE(result.valid) << testing::PrintToString(result.errors);
  Pipeline pipeline;
  EXPECT_TRUE(ConfigBuilder::buildPipeline(config, pipeline, false));
}

INSTANTIATE_TEST_SUITE_P(WindowsConfigurations, ShippedConfigTest,
                         testing::Values(KEYFLOW_CONFIG_PATH,
                                         KEYFLOW_DESIGNER_CONFIG_PATH));
