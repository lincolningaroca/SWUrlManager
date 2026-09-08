#pragma once

#include "helperdatabase/helperdb.hpp"
#include "util/helper.hpp"

#include <QDialog>

namespace Ui {
class LogInDialog;
}

class QCheckBox;
class QCloseEvent;
class QLineEdit;
class QPropertyAnimation;

class LogInDialog : public QDialog
{
  Q_OBJECT

public:

  enum OpenMode{ NO_STATE, FIRST_TIME};

  explicit LogInDialog(QWidget *parent = nullptr, OpenMode op = NO_STATE);
  ~LogInDialog();

  const QString& userName() const  noexcept{ return userName_;}
  QString getTextForAuthType(SW::AuthType type) const;

  void setToggledToButton(bool op);

private:
  Ui::LogInDialog *ui;

  QString userName_{};
  SW::HelperDataBase_t helperdb_{};

  QPropertyAnimation* collapseAnimation_{nullptr};
  bool isExpanded_{};


  void setUp_Form() noexcept;
  void setStateControls(bool op) noexcept;
  void setOptionsToComboBox(int index) noexcept;


  void clearControls() noexcept;
  bool Validate_hasNoEmpty() const noexcept;

  void writeSettings() const noexcept;
  void readSettings();
  void reject_form() noexcept;

  void setFeatures(QLineEdit *lineEdit, QCheckBox *checkBox, bool checked) noexcept;

  void handleToggleAnimation(bool checked);
  void setupAnimation();

  void applyIcons() noexcept;


  // QWidget interface
protected:
  void closeEvent(QCloseEvent *event) override;
};

