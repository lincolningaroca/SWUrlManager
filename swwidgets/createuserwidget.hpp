#pragma once

#include "helperdatabase/helperdb.hpp"
#include "util/helper.hpp"

#include <QWidget>

namespace Ui { class CreateUserWidget; }

class QLineEdit;
class QCheckBox;

class CreateUserWidget : public QWidget
{
  Q_OBJECT

public:
  explicit CreateUserWidget(QWidget *parent = nullptr);
  ~CreateUserWidget();

  void focusFirstField() noexcept;

  void saveControlStates() const;
  void restoreControlStates();

signals:
  void userCreated();

private:
  Ui::CreateUserWidget *ui;

  SW::HelperDataBase_t helperdb_{};

  void setUp_Form() noexcept;
  void applyIcons() noexcept;
  void setOptionsToComboBox(int index) noexcept;
  bool Validate_hasNoEmpty() const noexcept;
  void clearControls() noexcept;
  void setFeatures(QLineEdit *lineEdit, QCheckBox *checkBox, bool checked) noexcept;

private slots:
  void on_chkGenPassword_cliked(bool checked);
  void on_btnCreateUser_cliked(bool checked);

};
