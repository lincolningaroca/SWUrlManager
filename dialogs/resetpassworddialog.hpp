#pragma once

#include "helperdatabase/helperdb.hpp"

#include <QDialog>

class QCheckBox;
class QLineEdit;

namespace Ui {
  class ResetPasswordDialog;
}

class ResetPasswordDialog : public QDialog
{
  Q_OBJECT

public:
  explicit ResetPasswordDialog(QWidget *parent = nullptr);
  ~ResetPasswordDialog();

private:
  Ui::ResetPasswordDialog *ui;
  SW::HelperDataBase_t helper;

  uint32_t userId_{0};

  void setFeatures(QLineEdit* w, QCheckBox* b, bool checked) noexcept;

  void applyIcons() noexcept;
  void setInit_Form() noexcept;
};

