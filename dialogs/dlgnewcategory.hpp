#pragma once

#include "util/helper.hpp"

#include <QDialog>


namespace Ui {
class dlgNewCategory;
}

class dlgNewCategory : public QDialog
{
  Q_OBJECT


public:

  explicit dlgNewCategory(SW::OpenMode mode, const QStringList& list = {}, QWidget *parent = nullptr);

  ~dlgNewCategory();

  QString category() const noexcept;
  QString description() const noexcept;
  QString descriptionToolTip() const noexcept;

private:
  Ui::dlgNewCategory *ui;

  bool validateData() noexcept;

};

