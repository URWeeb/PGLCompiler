#pragma once

#include <memory>
#include <string>
#include <variant>

struct IntType {
  bool operator==(const IntType &) const = default;
};

struct BoolType {
  bool operator==(const BoolType &) const = default;
};

struct VoidType {
  bool operator==(const VoidType &) const = default;
};

struct StructType {
  std::string name;
  bool operator==(const StructType &) const = default;
};

struct Type;

struct ArrayType {
  std::unique_ptr<Type> element_type;

  ArrayType() = default;

  explicit ArrayType(std::unique_ptr<Type> element)
      : element_type(std::move(element)) {}

  ArrayType(const ArrayType &other)
      : element_type(other.element_type
                         ? std::make_unique<Type>(*other.element_type)
                         : nullptr) {}

  ArrayType &operator=(const ArrayType &other) {
    if (this != &other) {
      element_type = other.element_type
                         ? std::make_unique<Type>(*other.element_type)
                         : nullptr;
    }

    return *this;
  }

  ArrayType(ArrayType &&) = default;
  ArrayType &operator=(ArrayType &&) = default;

  bool operator==(const ArrayType &other) const;
};

struct Type : std::variant<BoolType, IntType, VoidType, StructType, ArrayType> {
  using variant::variant;
  bool operator==(const Type &) const = default;
};

inline bool ArrayType::operator==(const ArrayType &other) const {
  if (!element_type || !other.element_type) {
    return element_type == other.element_type;
  }

  return *element_type == *other.element_type;
}

// Немного магии из С++17
template <class... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};

inline std::string TypeToString(const Type &type) {
  return std::visit(
      overloaded{[](const IntType &) { return std::string("int"); },
                 [](const BoolType &) { return std::string("bool"); },
                 [](const VoidType &) { return std::string("void"); },
                 [](const StructType &cl) { return cl.name; },
                 [](const ArrayType &arr) {
                   return TypeToString(*arr.element_type) + "[]";
                 }},
      type);
}