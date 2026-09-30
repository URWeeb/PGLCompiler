#pragma once

#include <fstream>
#include <iostream>
#include <unordered_map>
#include <variant>
#include <vector>

#include "tree.hpp"

struct ArrayID {
  size_t value;
  bool operator==(const ArrayID &) const = default;
};

struct ObjectID {
  size_t value;
  bool operator==(const ObjectID &) const = default;
};
using PossibleValue = std::variant<int, bool, ArrayID, ObjectID>;

struct ArrayData {
  std::vector<PossibleValue> elements;
};

struct ObjectData {
  std::string struct_name;
  std::unordered_map<std::string, PossibleValue> fields;
};

struct RuntimeData {
  std::vector<ArrayData> arrays;
  std::vector<ObjectData> objects;

  ArrayID alloc_array(ArrayData data) {
    arrays.push_back(std::move(data));
    return ArrayID{arrays.size() - 1};
  }

  ObjectID alloc_object(ObjectData data) {
    objects.push_back(std::move(data));
    return ObjectID{objects.size() - 1};
  }
};

struct ReturnException : public std::exception {
  PossibleValue value;

  explicit ReturnException(const PossibleValue value) : value(value) {}
};