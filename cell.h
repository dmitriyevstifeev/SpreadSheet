#pragma once

#include "common.h"
#include "formula.h"

#include <functional>
#include <unordered_set>

class Sheet;
class Impl;
class EmptyImpl;

class Cell
    : public CellInterface {
 public:
  explicit Cell(SheetInterface &sheet);

  [[nodiscard]] std::string GetText() const override;
  [[nodiscard]] Value GetValue() const override;
  [[nodiscard]] std::vector<Position> GetReferencedCells() const override;

  void ClearCache();
  void Set(std::string text);
  void Clear();
  bool IsEmpty();
  [[nodiscard]] std::vector<Position> GetDependentCells() const;
  void AddDependentCell(Position pos);

 private:
  SheetInterface &sheet_;
  std::unique_ptr<Impl> impl_ = nullptr;
  std::vector<Position> dependent_cells_;
};

class Impl {
 public:
  virtual ~Impl() = default;
  virtual CellInterface::Value GetValue() = 0;
  [[nodiscard]] virtual std::string GetText() const = 0;
  [[nodiscard]] virtual std::vector<Position> GetReferencedCells() const = 0;
  virtual void ClearCache() = 0;
};

class EmptyImpl
    : public Impl {
 public:
  EmptyImpl() = default;

  [[nodiscard]] std::string GetText() const override;
  CellInterface::Value GetValue() override;
  [[nodiscard]] std::vector<Position> GetReferencedCells() const override;
  void ClearCache() override {}
};

class TextImpl
    : public Impl {
 public:
  TextImpl(std::string text);

  CellInterface::Value GetValue() override;
  [[nodiscard]] std::string GetText() const override;
  [[nodiscard]] std::vector<Position> GetReferencedCells() const override;
  void ClearCache() override {}

 private:
  std::string text_;
};

class FormulaImpl
    : public Impl {
 public:
  FormulaImpl(const SheetInterface &sheet, std::string text);

  CellInterface::Value GetValue() override;
  [[nodiscard]] std::string GetText() const override;
  [[nodiscard]] std::vector<Position> GetReferencedCells() const override;
  void ClearCache() override;
  [[nodiscard]] CellInterface::Value CalculateFormula() const;

 private:
  const SheetInterface &sheet_;
  std::unique_ptr<FormulaInterface> parsed_obj_ptr_;
  std::optional<CellInterface::Value> cached_value_;
};

std::ostream &operator<<(std::ostream &output, const CellInterface::Value &val);
