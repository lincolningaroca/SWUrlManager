#pragma once

#include <QDialog>

namespace Ui {
  class CategoryDialog;
}

class CategoryDialog : public QDialog
{
  Q_OBJECT

public:
  explicit CategoryDialog(const QList<QPair<uint32_t, QString>>& categoryList, QWidget *parent = nullptr);
  ~CategoryDialog();

  uint32_t getCategoryId() const noexcept;

private:
  Ui::CategoryDialog *ui;
  void loadCategoryComboBox(const QList<QPair<uint32_t, QString>> &categoryList) noexcept;


};

