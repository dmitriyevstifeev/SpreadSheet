#include "cell.h"
#include "common.h"
#include "sheet.h"

#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

void Sheet::CorrectSheetSizeToNewPos(Position pos) {
  if (pos.row + 1 > size_.rows) {
    sheet_.resize(pos.row + 1);
    for (auto &i: sheet_) {
      i.resize(size_.cols + 1);
    }
    size_.rows = pos.row + 1;
  }
  if (pos.col + 1 > size_.cols) {
    for (auto &i: sheet_) {
      i.resize(pos.col + 1);
    }

    size_.cols = pos.col + 1;
  }
}

void Sheet::SetCell(Position pos, const std::string &text) {
  if (!pos.IsValid()) {
    throw InvalidPositionException("Wrong position"s);
  }
  CorrectSheetSizeToNewPos(pos);
  Cell *cell_ptr = reinterpret_cast<Cell *>(this->GetCell(pos));
  if (cell_ptr) {
    if (cell_ptr->GetText() == text) {
      return;
    }
    cell_ptr->Set(text);
  } else {
    auto new_cell = std::make_unique<Cell>(*this);
    new_cell->Set(text);
    sheet_[pos.row][pos.col] = std::move(new_cell);
    cell_ptr = reinterpret_cast<Cell *>(this->GetCell(pos));
  }

  for (const auto &ref_cell_pos: cell_ptr->GetReferencedCells()) {
    if (!ref_cell_pos.IsValid()) {
      return;
    }
    if (this->GetCell(ref_cell_pos) == nullptr) {
      auto new_cell = std::make_unique<Cell>(*this);
      CorrectSheetSizeToNewPos(ref_cell_pos);
      sheet_[ref_cell_pos.row][ref_cell_pos.col] = std::move(new_cell);
      printable_size_ = size_;

      return;
    }
  }

  std::unordered_set<Cell *> closure;
  CheckCircularDependency(*this, cell_ptr, cell_ptr, closure);
  for (const auto &cell_pos: cell_ptr->GetReferencedCells()) {
    auto in_cell_ptr = reinterpret_cast<Cell *>(this->GetCell(cell_pos));
    if (in_cell_ptr) {
      in_cell_ptr->AddDependentCell(pos);
    }
  }
  ClearSheetCache(*this, cell_ptr);
  printable_size_ = size_;
}

void Sheet::CheckCircularDependency(Sheet &sheet, Cell *init_ptr, Cell *cell_ptr, std::unordered_set<Cell *> &closure) {
  if (!cell_ptr) {
    return;
  }
  auto cells = cell_ptr->GetReferencedCells();
  for (const auto &cell_pos: cells) {
    if (!cell_pos.IsValid()) {
      continue;
    }
    auto *in_cell_ptr = reinterpret_cast<Cell *>(sheet.GetCell(cell_pos));
    if (closure.find(in_cell_ptr) != closure.end()) {
      continue;
    }
    if (init_ptr == in_cell_ptr) {
      throw CircularDependencyException("Circular dependency"s);
    }
    closure.insert(in_cell_ptr);
    CheckCircularDependency(sheet, init_ptr, in_cell_ptr, closure);
  }
}

void Sheet::ClearSheetCache(Sheet &sheet, Cell *cell_ptr) {
  for (const auto &cell_pos: cell_ptr->GetDependentCells()) {
    auto *in_cell_ptr = reinterpret_cast<Cell *>(sheet.GetCell(cell_pos));
    in_cell_ptr->ClearCache();
    ClearSheetCache(sheet, in_cell_ptr);
  }
}

CellInterface *Sheet::GetCell(Position pos) {
  if (!pos.IsValid()) {
    throw InvalidPositionException("Invalid position"s);
  }
  if (IsPositionInvalid(pos)) {
    return nullptr;
  }
  auto &cell_link = sheet_[pos.row][pos.col];
  if (!cell_link || cell_link->IsEmpty()) {
    return nullptr;
  }
  return cell_link.get();
}

const CellInterface *Sheet::GetCell(Position pos) const {
  if (!pos.IsValid()) {
    throw InvalidPositionException("Invalid position"s);
  }
  if (IsPositionInvalid(pos)) {
    return nullptr;
  }
  auto &cell_link = sheet_[pos.row][pos.col];
  if (!cell_link || cell_link->IsEmpty()) {
    return nullptr;
  }
  return cell_link.get();
}

void Sheet::ClearCell(Position pos) {
  auto *cell_ptr = reinterpret_cast<Cell *>(GetCell(pos));
  if (!cell_ptr) {
    return;
  }
  cell_ptr->Clear();
  UpdatePrintableSize();
}

void Sheet::UpdatePrintableSize() {
  int last_non_empty_row = Position::NONE.row;
  int last_non_empty_col = Position::NONE.col;

  for (int row = 0; row < printable_size_.rows; ++row) {
    for (int col = 0; col < printable_size_.cols; ++col) {
      auto *cell_ptr = reinterpret_cast<Cell *>(GetCell({row, col}));
      if (cell_ptr) {
        if (!cell_ptr->IsEmpty()) {
          if (row > last_non_empty_row) {
            last_non_empty_row = row;
          }
         if (col > last_non_empty_col) {
            last_non_empty_col = col;
          }
        }
      }
    }
  }

  printable_size_ = {last_non_empty_row + 1, last_non_empty_col + 1};
}

Size Sheet::GetPrintableSize() const {
  return printable_size_;
}

void Sheet::PrintData(std::ostream &output, DataType data_type) const {
  auto printable_size = GetPrintableSize();

  for (int row = 0; row < printable_size.rows; ++row) {
    for (int col = 0; col < printable_size.cols; ++col) {
      const auto *cell_ptr = reinterpret_cast<const Cell *>(GetCell({row, col}));
      if (cell_ptr) {
        if (data_type == DataType::VALUES) {
          output << cell_ptr->GetValue();
        }
        if (data_type == DataType::TEXT) {
          output << cell_ptr->GetText();
        }
      }
      if (col + 1 != printable_size.cols) {
        output << "\t";
      }
    }

    output << "\n";
  }
}

void Sheet::PrintValues(std::ostream &output) const {
  PrintData(output, DataType::VALUES);
}

void Sheet::PrintTexts(std::ostream &output) const {
  PrintData(output, DataType::TEXT);
}

bool Sheet::IsPositionInvalid(const Position &pos) const {
  return pos.col + 1 > size_.cols || pos.row + 1 > size_.rows;
}

std::unique_ptr<SheetInterface> CreateSheet() {
  return std::make_unique<Sheet>();
}
