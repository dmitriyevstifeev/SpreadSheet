#include "formula.h"
#include "FormulaAST.h"

#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream &operator<<(std::ostream &output, FormulaError fe) {
  return output << "#DIV/0!"s;
}

namespace
  {
    class Formula
        : public FormulaInterface {
     public:
      explicit Formula(std::string expression)
          : ast_(ParseFormulaAST(expression)) {
      }

      Value Evaluate(const SheetInterface &sheet) const override {
        CellLookup cell_lookup = [&sheet](Position pos) {
          double result = 0.0;
          if (!pos.IsValid()) {
            throw FormulaError(FormulaError::Category::Ref);
          }
          if (sheet.GetCell(pos) == nullptr) {
            return 0.0;
          }
          CellInterface::Value val = sheet.GetCell(pos)->GetValue();
          if (std::holds_alternative<double>(val)) {
            result = std::get<double>(val);
          }
          if (std::holds_alternative<FormulaError>(val)) {
            throw std::get<FormulaError>(val);
          }
          if (std::holds_alternative<std::string>(val)) {
            try {
              result = std::stod(std::get<std::string>(val));
            } catch (std::invalid_argument &err) {
              throw FormulaError(FormulaError::Category::Value);
            }
          }
          return result;
        };
        Value val;
        try {
          val = ast_.Execute(cell_lookup);
        } catch (FormulaError &err) {
          val = err;
        }
        return val;
      };

      std::string GetExpression() const override {
        std::stringstream ss;
        ast_.PrintFormula(ss);
        return ss.str();
      };

      std::vector<Position> GetReferencedCells() const {
        std::vector<Position> referenced_cells;
        for (const auto &cell: ast_.GetCells()) {
          referenced_cells.push_back(cell);
        }
        return referenced_cells;
      }

     private:
      FormulaAST ast_;
    };
  } // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
  return std::make_unique<Formula>(std::move(expression));
}