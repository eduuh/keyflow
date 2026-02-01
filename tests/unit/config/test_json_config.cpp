#include "config/JsonConfig.h"
#include <gtest/gtest.h>

using namespace keyflow;

// ===== JsonConfig Construction Tests =====

TEST(JsonConfigTest, DefaultConstruction) {
  JsonConfig config;

  EXPECT_EQ(config.version, "1.0");
  EXPECT_TRUE(config.name.empty());
  EXPECT_TRUE(config.remapping.empty());
  EXPECT_TRUE(config.noModCombos.empty());
  EXPECT_TRUE(config.layers.empty());
}

// ===== Field Access Tests =====

TEST(JsonConfigTest, RemappingField) {
  JsonConfig config;

  config.remapping["A"] = "B";
  config.remapping["CapsLock"] = "Escape";

  EXPECT_EQ(config.remapping["A"], "B");
  EXPECT_EQ(config.remapping["CapsLock"], "Escape");
  EXPECT_EQ(config.remapping.size(), 2u);
}

TEST(JsonConfigTest, VersionField) {
  JsonConfig config;
  config.version = "2.0";

  EXPECT_EQ(config.version, "2.0");
}

TEST(JsonConfigTest, NameField) {
  JsonConfig config;
  config.name = "MyConfig";

  EXPECT_EQ(config.name, "MyConfig");
}

// ===== Layer Struct Tests =====

TEST(LayerTest, LayerConstruction) {
  Layer layer;
  layer.name = "vim";
  layer.triggers = {"LeftCtrl"};

  EXPECT_EQ(layer.name, "vim");
  EXPECT_EQ(layer.triggers.size(), 1u);
}

TEST(LayerTest, LayerConstructionWithName) {
  Layer layer("nav");

  EXPECT_EQ(layer.name, "nav");
}

TEST(LayerTest, LayerMappings) {
  Layer layer;
  layer.mappings["H"] = "Left";
  layer.mappings["J"] = "Down";
  layer.mappings["K"] = "Up";
  layer.mappings["L"] = "Right";

  EXPECT_EQ(layer.mappings.size(), 4u);
  EXPECT_EQ(layer.mappings["H"], "Left");
}

TEST(LayerTest, LayerShiftMappings) {
  Layer layer;

  ShiftMapping shift1;
  shift1.key = "1";
  shift1.output = "Exclaim";
  shift1.shift = true;

  layer.shiftMappings.push_back(shift1);

  EXPECT_EQ(layer.shiftMappings.size(), 1u);
  EXPECT_EQ(layer.shiftMappings[0].key, "1");
}

// ===== NoModCombo Struct Tests =====

TEST(NoModComboTest, DefaultConstruction) {
  NoModCombo combo;

  EXPECT_TRUE(combo.key.empty());
  EXPECT_TRUE(combo.output.empty());
  EXPECT_FALSE(combo.shift);
}

TEST(NoModComboTest, ParameterizedConstruction) {
  NoModCombo combo("A", "B", true, "Remap A to B with shift");

  EXPECT_EQ(combo.key, "A");
  EXPECT_EQ(combo.output, "B");
  EXPECT_TRUE(combo.shift);
  EXPECT_EQ(combo.description, "Remap A to B with shift");
}

// ===== ShiftMapping Struct Tests =====

TEST(ShiftMappingTest, DefaultConstruction) {
  ShiftMapping mapping;

  EXPECT_TRUE(mapping.key.empty());
  EXPECT_TRUE(mapping.output.empty());
  EXPECT_TRUE(mapping.shift); // Default is true
}

TEST(ShiftMappingTest, ParameterizedConstruction) {
  ShiftMapping mapping("1", "Exclaim", true, "! symbol");

  EXPECT_EQ(mapping.key, "1");
  EXPECT_EQ(mapping.output, "Exclaim");
  EXPECT_TRUE(mapping.shift);
  EXPECT_EQ(mapping.description, "! symbol");
}

// ===== Multiple Items Tests =====

TEST(JsonConfigTest, MultipleRemappings) {
  JsonConfig config;

  config.remapping["A"] = "B";
  config.remapping["B"] = "A";
  config.remapping["CapsLock"] = "Escape";

  EXPECT_EQ(config.remapping.size(), 3u);
}

TEST(JsonConfigTest, MultipleLayers) {
  JsonConfig config;

  Layer nav("nav");
  nav.triggers = {"Space"};

  Layer numpad("numpad");
  numpad.triggers = {"M"};

  config.layers.push_back(nav);
  config.layers.push_back(numpad);

  EXPECT_EQ(config.layers.size(), 2u);
}

TEST(JsonConfigTest, MultipleNoModCombos) {
  JsonConfig config;

  NoModCombo combo1("A", "B");
  NoModCombo combo2("C", "D");

  config.noModCombos.push_back(combo1);
  config.noModCombos.push_back(combo2);

  EXPECT_EQ(config.noModCombos.size(), 2u);
}

// ===== Complex Config Tests =====

TEST(JsonConfigTest, CompleteConfigStructure) {
  JsonConfig config;

  // Add remappings
  config.remapping["CapsLock"] = "Escape";

  // Add layer
  Layer layer("nav");
  layer.triggers = {"LeftCtrl"};
  layer.mappings["H"] = "Left";
  config.layers.push_back(layer);

  // Add no-mod combo
  NoModCombo combo("A", "B", false, "Test");
  config.noModCombos.push_back(combo);

  EXPECT_EQ(config.remapping.size(), 1u);
  EXPECT_EQ(config.layers.size(), 1u);
  EXPECT_EQ(config.noModCombos.size(), 1u);
}
