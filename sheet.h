#pragma once

#include "cell.h"
#include "common.h"

#include <functional>

enum class DataType {
    VALUES,
    TEXT
};

class Sheet : public SheetInterface {
public:
    void SetCell(Position pos, const std::string& text) override;

    [[nodiscard]] const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;
    void ClearCell(Position pos) override;
    [[nodiscard]] Size GetPrintableSize() const override;
    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

private:
    void CorrectSheetSizeToNewPos(Position pos);

    [[nodiscard]] bool IsPositionInvalid(const Position& pos) const;
    void UpdatePrintableSize();
    void PrintData(std::ostream& output, DataType data_type) const;
    void ClearSheetCache(Sheet& sheet, Cell* cell_ptr);
    void CheckCircularDependency(Sheet& sheet, Cell* init_ptr, Cell* cell_ptr, std::unordered_set<Cell*>& closure);

private:
    Size size_ = { 0, 0 };
    Size printable_size_ = { 0, 0 };
    std::vector<std::vector<std::unique_ptr<Cell>>> sheet_;
};
