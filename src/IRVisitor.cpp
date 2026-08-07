#include "IRVisitor.hpp"

#include <llvm/IR/Constants.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/TargetParser/Host.h>
#include <stdexcept>

IRVisitor::IRVisitor(const SymbolTable &symbol_table)
    : module_(std::make_unique<llvm::Module>("program", context_)),
      builder_(context_), symbol_table_(symbol_table) {
  module_->setTargetTriple(llvm::sys::getDefaultTargetTriple());
  DeclareExternals();
}

bool IRVisitor::IsBlockTerminated() const {
  return builder_.GetInsertBlock()->getTerminator() != nullptr;
}

void IRVisitor::DeclareExternals() {
  auto *printf_type = llvm::FunctionType::get(i32Ty(), {ptrTy()}, true);
  printf_function_ = module_->getOrInsertFunction("printf", printf_type);

  auto *malloc_type = llvm::FunctionType::get(ptrTy(), {i64Ty()}, false);
  malloc_function_ = module_->getOrInsertFunction("malloc", malloc_type);

  fmt_int_ = builder_.CreateGlobalString("%d\n", ".fmt_int", 0, module_.get());
}

llvm::Type *IRVisitor::LLVMType(const Type &type) {
  return std::visit(
      overloaded{
          [&](const IntType &) -> llvm::Type * { return i32Ty(); },
          [&](const BoolType &) -> llvm::Type * { return i1Ty(); },
          [&](const VoidType &) -> llvm::Type * { return voidTy(); },
          [&](const ClassType &) -> llvm::Type * { return ptrTy(); },
          [&](const ArrayType &) -> llvm::Type * { return ptrTy(); },
      },
      type);
}

void IRVisitor::DeclareStructTypes(const Program &node) {
  for (const auto &clss : node.classes) {
    class_types_[clss->name] = llvm::StructType::create(context_, clss->name);
  }

  for (const auto &clss : node.classes) {
    std::vector<llvm::Type *> body;
    unsigned index = 0;

    for (const auto &field : clss->fields) {
      body.push_back(LLVMType(field.type));
      field_indices_[clss->name][field.name] = index++;
      field_types_[clss->name][field.name] = field.type;
    }

    class_types_[clss->name]->setBody(body);
  }
}

void IRVisitor::DeclareFunctionSignatures(const Program &node) {
  for (const auto &function : node.functions) {
    std::vector<llvm::Type *> parameters;

    for (const auto &parameter : function->parameters) {
      parameters.push_back(LLVMType(parameter.type));
    }

    auto *function_type = llvm::FunctionType::get(
        LLVMType(function->return_type), parameters, false);

    llvm::Function::Create(function_type, llvm::Function::ExternalLinkage,
                           function->name, *module_);
  }

  for (const auto &clss : node.classes) {
    for (const auto &method : clss->methods) {
      std::vector<llvm::Type *> parameters = {ptrTy()};

      for (const auto &parameter : method->parameters) {
        parameters.push_back(LLVMType(parameter.type));
      }

      auto *function_type = llvm::FunctionType::get(
          LLVMType(method->return_type), parameters, false);

      llvm::Function::Create(function_type, llvm::Function::ExternalLinkage,
                             MethodFunctionName(clss->name, method->name),
                             *module_);
    }
  }
}

void IRVisitor::EmitBody(const Statements &statements) {
  for (const auto &statement : statements) {
    statement->Accept(*this);

    if (IsBlockTerminated()) {
      break;
    }
  }
}

void IRVisitor::AddDefaultReturn(const Type &return_type) {
  if (IsBlockTerminated()) {
    return;
  }

  std::visit(overloaded{
                 [&](const IntType &) {
                   builder_.CreateRet(llvm::ConstantInt::get(i32Ty(), 0));
                 },
                 [&](const VoidType &) { builder_.CreateRetVoid(); },
                 [&](const BoolType &) {
                   builder_.CreateRet(llvm::ConstantInt::get(i1Ty(), 0));
                 },
                 [&](const ClassType &) {
                   builder_.CreateRet(llvm::ConstantPointerNull::get(
                       llvm::PointerType::getUnqual(context_)));
                 },
                 [&](const ArrayType &) {
                   builder_.CreateRet(llvm::ConstantPointerNull::get(
                       llvm::PointerType::getUnqual(context_)));
                 },
             },
             return_type);
}

std::string IRVisitor::MethodFunctionName(const std::string &clss,
                                          const std::string &method) {
  return clss + "__" + method;
}

llvm::AllocaInst *IRVisitor::CreateEntryAlloca(const std::string &name,
                                               llvm::Type *ty) const {
  auto &entry = current_function_->getEntryBlock();
  llvm::IRBuilder<> temp(&entry, entry.begin());

  return temp.CreateAlloca(ty, nullptr, name);
}

std::pair<llvm::Value *, Type>
IRVisitor::LookupAddress(const std::string &name) {
  auto literal = local_vars_.find(name);

  if (literal != local_vars_.end()) {
    return {literal->second.alloca, literal->second.type};
  }

  auto field_ptr = fields_.find(name);

  if (field_ptr != fields_.end()) {
    return {field_ptr->second.value, field_ptr->second.type};
  }

  throw std::runtime_error("IRVisitor: variable '" + name + "' not found");
}

llvm::Value *IRVisitor::SizeOf(llvm::Type *ty) {
  auto *null_pointer =
      llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(context_));

  auto *get_element_pointer = builder_.CreateGEP(
      ty, null_pointer, llvm::ConstantInt::get(i32Ty(), 1), "sizeof");

  return builder_.CreatePtrToInt(get_element_pointer, i64Ty(), "size");
}

llvm::Value *IRVisitor::EmitMalloc(llvm::Value *byte_count) {
  return builder_.CreateCall(malloc_function_, {byte_count}, "malloc_ptr");
}

void IRVisitor::Visit(const Program &node) {
  DeclareStructTypes(node);
  DeclareFunctionSignatures(node);

  for (auto &clss : node.classes) {
    clss->Accept(*this);
  }

  for (auto &function : node.functions) {
    function->Accept(*this);
  }

  if (!node.instructions.empty()) {
    auto *function_type = llvm::FunctionType::get(i32Ty(), {}, false);
    auto *main_function = llvm::Function::Create(
        function_type, llvm::Function::ExternalLinkage, "main", *module_);
    auto *basic_block =
        llvm::BasicBlock::Create(context_, "entry", main_function);
    builder_.SetInsertPoint(basic_block);
    current_function_ = main_function;
    EmitBody(node.instructions);
    AddDefaultReturn(IntType{});
  }
}

void IRVisitor::Visit(const IntLiteral &node) {
  last_value_ = llvm::ConstantInt::get(i32Ty(), node.value, true);
}

void IRVisitor::Visit(const BoolLiteral &node) {
  last_value_ = llvm::ConstantInt::get(i1Ty(), node.value ? 1 : 0);
}

void IRVisitor::Visit(const IdentityLiteral &node) {
  auto [ptr, type] = LookupAddress(node.value);
  last_value_ = builder_.CreateLoad(LLVMType(type), ptr, node.value);
}

void IRVisitor::Visit(const UnaryOperation &node) {
  node.operand->Accept(*this);

  if (node.operation == "-") {
    last_value_ = builder_.CreateNeg(last_value_, "neg");
  } else if (node.operation == "!") {
    last_value_ = builder_.CreateNot(last_value_, "not");
  }
}

void IRVisitor::Visit(const BinaryOperation &node) {
  if (node.operation == "&&") {
    node.left_operand->Accept(*this);
    llvm::Value *lhs_val = last_value_;
    auto *lhs_exit_bb = builder_.GetInsertBlock();
    auto *rhs_bb =
        llvm::BasicBlock::Create(context_, "and.rhs", current_function_);
    auto *merge_bb =
        llvm::BasicBlock::Create(context_, "and.end", current_function_);
    builder_.CreateCondBr(lhs_val, rhs_bb, merge_bb);
    builder_.SetInsertPoint(rhs_bb);
    node.right_operand->Accept(*this);
    llvm::Value *rhs_val = last_value_;
    auto *rhs_exit_bb = builder_.GetInsertBlock();
    builder_.CreateBr(merge_bb);
    builder_.SetInsertPoint(merge_bb);
    auto *phi = builder_.CreatePHI(i1Ty(), 2, "and.result");
    phi->addIncoming(llvm::ConstantInt::getFalse(context_), lhs_exit_bb);
    phi->addIncoming(rhs_val, rhs_exit_bb);
    last_value_ = phi;
    return;
  }

  if (node.operation == "||") {
    node.left_operand->Accept(*this);
    llvm::Value *lhs_val = last_value_;
    auto *lhs_exit_bb = builder_.GetInsertBlock();
    auto *rhs_bb =
        llvm::BasicBlock::Create(context_, "or.rhs", current_function_);
    auto *merge_bb =
        llvm::BasicBlock::Create(context_, "or.end", current_function_);
    builder_.CreateCondBr(lhs_val, merge_bb, rhs_bb);
    builder_.SetInsertPoint(rhs_bb);
    node.right_operand->Accept(*this);
    llvm::Value *rhs_val = last_value_;
    auto *rhs_exit_bb = builder_.GetInsertBlock();
    builder_.CreateBr(merge_bb);
    builder_.SetInsertPoint(merge_bb);
    auto *phi = builder_.CreatePHI(i1Ty(), 2, "or.result");
    phi->addIncoming(llvm::ConstantInt::getTrue(context_), lhs_exit_bb);
    phi->addIncoming(rhs_val, rhs_exit_bb);
    last_value_ = phi;
    return;
  }

  node.left_operand->Accept(*this);
  llvm::Value *l_val = last_value_;
  node.right_operand->Accept(*this);
  llvm::Value *r_val = last_value_;

  if (node.operation == "+") {
    last_value_ = builder_.CreateAdd(l_val, r_val, "add");
  } else if (node.operation == "-") {
    last_value_ = builder_.CreateSub(l_val, r_val, "sub");
  } else if (node.operation == "*") {
    last_value_ = builder_.CreateMul(l_val, r_val, "mul");
  } else if (node.operation == "/") {
    last_value_ = builder_.CreateSDiv(l_val, r_val, "div");
  } else if (node.operation == "==") {
    last_value_ = builder_.CreateICmpEQ(l_val, r_val, "eq");
  } else if (node.operation == "!=") {
    last_value_ = builder_.CreateICmpNE(l_val, r_val, "ne");
  } else if (node.operation == "<") {
    last_value_ = builder_.CreateICmpSLT(l_val, r_val, "lt");
  } else if (node.operation == ">") {
    last_value_ = builder_.CreateICmpSGT(l_val, r_val, "gt");
  } else if (node.operation == ">=") {
    last_value_ = builder_.CreateICmpSGE(l_val, r_val, "ge");
  } else if (node.operation == "<=") {
    last_value_ = builder_.CreateICmpSLE(l_val, r_val, "le");
  }
}

void IRVisitor::Visit(const NewObjectExpression &node) {
  auto *struct_type = class_types_.at(node.class_name);
  auto *size = SizeOf(struct_type);
  auto *raw = EmitMalloc(size);

  auto *memset_type =
      llvm::FunctionType::get(ptrTy(), {ptrTy(), i32Ty(), i64Ty()}, false);
  auto memset_function = module_->getOrInsertFunction("memset", memset_type);
  builder_.CreateCall(memset_function,
                      {raw, llvm::ConstantInt::get(i32Ty(), 0), size});

  last_value_ = raw;
}

void IRVisitor::Visit(const NewArrayExpression &node) {
  auto *element_type = LLVMType(node.element_type);
  auto *element_size = SizeOf(element_type);

  node.size->Accept(*this);
  auto *count = last_value_;
  auto *count64 = builder_.CreateSExt(count, i64Ty(), "count64");
  auto *total = builder_.CreateMul(count64, element_size, "arr_bytes");
  last_value_ = EmitMalloc(total);
}

void IRVisitor::Visit(const ArrayIndexExpression &node) {
  auto [array_addr, array_type] = LookupAddress(node.name);
  const Type &typed_element_type =
      *std::get<ArrayType>(array_type).element_type;
  auto *element_type = LLVMType(typed_element_type);
  auto *array_ptr = builder_.CreateLoad(ptrTy(), array_addr, "arr");
  node.index->Accept(*this);
  auto *element_ptr =
      builder_.CreateGEP(element_type, array_ptr, last_value_, "elem.ptr");
  last_value_ = builder_.CreateLoad(element_type, element_ptr, "elem");
}

void IRVisitor::Visit(const FieldAccessExpression &node) {
  auto [object_addr, object_type] = LookupAddress(node.object);
  const auto &class_name = std::get<ClassType>(object_type).name;
  auto *object_ptr = builder_.CreateLoad(ptrTy(), object_addr, "obj");
  auto *struct_type = class_types_.at(class_name);
  unsigned idx = field_indices_.at(class_name).at(node.field);
  const Type &field_type = field_types_.at(class_name).at(node.field);
  auto *gep = builder_.CreateGEP(struct_type, object_ptr,
                                 {llvm::ConstantInt::get(i32Ty(), 0),
                                  llvm::ConstantInt::get(i32Ty(), idx)},
                                 node.field + ".ptr");
  last_value_ = builder_.CreateLoad(LLVMType(field_type), gep, node.field);
}

void IRVisitor::Visit(const MethodCallExpression &node) {
  auto [obj_addr, obj_type] = LookupAddress(node.object);
  const auto &class_name = std::get<ClassType>(obj_type).name;
  auto *obj_ptr = builder_.CreateLoad(ptrTy(), obj_addr, "obj");
  llvm::Function *meth =
      module_->getFunction(MethodFunctionName(class_name, node.method_name));
  std::vector<llvm::Value *> args = {obj_ptr};

  for (const auto &a : node.args) {
    a->Accept(*this);
    args.push_back(last_value_);
  }

  if (meth->getReturnType()->isVoidTy()) {
    builder_.CreateCall(meth, args);
    last_value_ = nullptr;
  } else {
    last_value_ = builder_.CreateCall(meth, args, "mcall");
  }
}

void IRVisitor::Visit(const FunctionCallExpression &node) {
  llvm::Function *func = module_->getFunction(node.function_name);
  std::vector<llvm::Value *> args;

  for (const auto &a : node.args) {
    a->Accept(*this);
    args.push_back(last_value_);
  }

  if (func->getReturnType()->isVoidTy()) {
    builder_.CreateCall(func, args);
    last_value_ = nullptr;
  } else {
    last_value_ = builder_.CreateCall(func, args, node.function_name + ".ret");
  }
}

void IRVisitor::Visit(const MethodCallStatement &node) {
  auto [obj_addr, obj_type] = LookupAddress(node.object);
  const auto &class_name = std::get<ClassType>(obj_type).name;
  auto *obj_ptr = builder_.CreateLoad(ptrTy(), obj_addr, "obj");
  llvm::Function *meth =
      module_->getFunction(MethodFunctionName(class_name, node.method_name));
  std::vector<llvm::Value *> args = {obj_ptr};

  for (const auto &a : node.args) {
    a->Accept(*this);
    args.push_back(last_value_);
  }

  builder_.CreateCall(meth, args);
}

void IRVisitor::Visit(const FunctionCallStatement &node) {
  llvm::Function *func = module_->getFunction(node.function_name);
  std::vector<llvm::Value *> args;

  for (const auto &a : node.args) {
    a->Accept(*this);
    args.push_back(last_value_);
  }

  builder_.CreateCall(func, args);
}

void IRVisitor::Visit(const VariableDeclaration &node) {
  node.value->Accept(*this);
  auto *alloca = CreateEntryAlloca(node.name, LLVMType(node.type));
  builder_.CreateStore(last_value_, alloca);
  local_vars_[node.name] = {alloca, node.type};
}

void IRVisitor::Visit(const AssignStatement &node) {
  node.value->Accept(*this);
  auto [ptr, _type] = LookupAddress(node.name);
  builder_.CreateStore(last_value_, ptr);
}

void IRVisitor::Visit(const ArrayAssignStatement &node) {
  auto [arr_addr, arr_type] = LookupAddress(node.name);
  const Type &elem_type = *std::get<ArrayType>(arr_type).element_type;
  auto *elem_ty = LLVMType(elem_type);
  auto *arr_ptr = builder_.CreateLoad(ptrTy(), arr_addr, "arr");
  node.index->Accept(*this);
  auto *idx = last_value_;
  node.value->Accept(*this);
  auto *val = last_value_;
  auto *elem_ptr = builder_.CreateGEP(elem_ty, arr_ptr, idx, "elem.ptr");
  builder_.CreateStore(val, elem_ptr);
}

void IRVisitor::Visit(const IfStatement &node) {
  node.condition->Accept(*this);
  auto *then_bb =
      llvm::BasicBlock::Create(context_, "if.then", current_function_);
  auto *merge_bb =
      llvm::BasicBlock::Create(context_, "if.end", current_function_);
  builder_.CreateCondBr(last_value_, then_bb, merge_bb);
  builder_.SetInsertPoint(then_bb);
  EmitBody(node.instructions);

  if (!IsBlockTerminated()) {
    builder_.CreateBr(merge_bb);
  }

  builder_.SetInsertPoint(merge_bb);
}

void IRVisitor::Visit(const IfElseStatement &node) {
  node.condition->Accept(*this);
  auto *then_bb =
      llvm::BasicBlock::Create(context_, "if.then", current_function_);
  auto *else_bb =
      llvm::BasicBlock::Create(context_, "if.else", current_function_);
  auto *merge_bb =
      llvm::BasicBlock::Create(context_, "if.end", current_function_);
  builder_.CreateCondBr(last_value_, then_bb, else_bb);
  builder_.SetInsertPoint(then_bb);
  EmitBody(node.then_instructions);

  if (!IsBlockTerminated()) {
    builder_.CreateBr(merge_bb);
  }

  builder_.SetInsertPoint(else_bb);
  EmitBody(node.else_instructions);

  if (!IsBlockTerminated()) {
    builder_.CreateBr(merge_bb);
  }

  builder_.SetInsertPoint(merge_bb);
}

void IRVisitor::Visit(const WhileStatement &node) {
  auto *cond_bb =
      llvm::BasicBlock::Create(context_, "while.cond", current_function_);
  auto *body_bb =
      llvm::BasicBlock::Create(context_, "while.body", current_function_);
  auto *after_bb =
      llvm::BasicBlock::Create(context_, "while.end", current_function_);
  builder_.CreateBr(cond_bb);
  builder_.SetInsertPoint(cond_bb);
  node.condition->Accept(*this);
  builder_.CreateCondBr(last_value_, body_bb, after_bb);
  builder_.SetInsertPoint(body_bb);
  EmitBody(node.body);

  if (!IsBlockTerminated()) {
    builder_.CreateBr(cond_bb);
  }

  builder_.SetInsertPoint(after_bb);
}

void IRVisitor::Visit(const PrintStatement &node) {
  node.expression->Accept(*this);
  auto *val = last_value_;

  if (val->getType()->isIntegerTy(1)) {
    val = builder_.CreateZExt(val, i32Ty(), "bool.int");
  }

  builder_.CreateCall(printf_function_, {fmt_int_, val});
}

void IRVisitor::Visit(const ReturnStatement &node) {
  node.value->Accept(*this);
  builder_.CreateRet(last_value_);
}

void IRVisitor::Visit(const ClassDeclaration &node) {
  std::string previous = current_class_;
  current_class_ = node.name;

  for (const auto &method : node.methods) {
    method->Accept(*this);
  }

  current_class_ = previous;
}

void IRVisitor::Visit(const FunctionDeclaration &node) {
  llvm::Function *function = module_->getFunction(node.name);

  if (!function) {
    return;
  }

  auto *basic_block = llvm::BasicBlock::Create(context_, "entry", function);
  builder_.SetInsertPoint(basic_block);
  current_function_ = function;
  local_vars_.clear();
  fields_.clear();
  self_ptr_ = nullptr;
  auto argument_iter = function->arg_begin();

  for (const auto &parameter : node.parameters) {
    argument_iter->setName(parameter.name);
    auto *alloca =
        CreateEntryAlloca(parameter.name + ".addr", LLVMType(parameter.type));
    builder_.CreateStore(argument_iter, alloca);
    local_vars_[parameter.name] = {alloca, parameter.type};
    ++argument_iter;
  }

  EmitBody(node.body);
  AddDefaultReturn(node.return_type);
  current_function_ = nullptr;
  local_vars_.clear();
}

void IRVisitor::Visit(const MethodDeclaration &node) {
  llvm::Function *function =
      module_->getFunction(MethodFunctionName(current_class_, node.name));

  if (!function) {
    return;
  }

  auto *basic_block = llvm::BasicBlock::Create(context_, "entry", function);
  builder_.SetInsertPoint(basic_block);
  current_function_ = function;
  local_vars_.clear();
  fields_.clear();
  auto argument_iter = function->arg_begin();
  argument_iter->setName("self");
  self_ptr_ = argument_iter++;
  auto *struct_type = class_types_.at(current_class_);

  for (const auto &[field_name, field_index] : field_indices_[current_class_]) {
    auto *gep =
        builder_.CreateGEP(struct_type, self_ptr_,
                           {llvm::ConstantInt::get(i32Ty(), 0),
                            llvm::ConstantInt::get(i32Ty(), field_index)},
                           field_name + ".ptr");
    fields_[field_name] = {gep, field_types_.at(current_class_).at(field_name)};
  }

  for (const auto &parameter : node.parameters) {
    argument_iter->setName(parameter.name);
    auto *alloca =
        CreateEntryAlloca(parameter.name + ".addr", LLVMType(parameter.type));
    builder_.CreateStore(argument_iter++, alloca);
    local_vars_[parameter.name] = {alloca, parameter.type};
  }

  EmitBody(node.body);
  AddDefaultReturn(node.return_type);
  current_function_ = nullptr;
  local_vars_.clear();
  fields_.clear();
  self_ptr_ = nullptr;
}

void IRVisitor::Print(llvm::raw_ostream &os) const {
  module_->print(os, nullptr);
}

bool IRVisitor::SaveToFile(const std::string &path) const {
  std::error_code ec;
  llvm::raw_fd_ostream out(path, ec, llvm::sys::fs::OF_Text);
  if (ec) {
    llvm::errs() << "Cannot open '" << path << "': " << ec.message() << "\n";
    return false;
  }
  module_->print(out, nullptr);
  return true;
}

std::unique_ptr<llvm::Module> IRVisitor::GetModule() {
  return std::move(module_);
}