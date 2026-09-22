#pragma once

#include "helperdatabase/helperdb.hpp"
#include "util/helper.hpp"

#include <QDialog>

namespace Ui {
class LogInDialog;
}

class QCloseEvent;
class QPropertyAnimation;
class CreateUserWidget;

class LogInDialog : public QDialog
{
  Q_OBJECT

public:
  explicit LogInDialog(QWidget *parent = nullptr);
  ~LogInDialog();

  const QString& userName() const noexcept{ return userName_; }

private:
  Ui::LogInDialog *ui;

  QString userName_{};
  SW::HelperDataBase_t helperdb_{};

  CreateUserWidget* createUserWidget_{nullptr};

  QPropertyAnimation* collapseAnimation_{nullptr};
  bool isExpanded_{};

  void setUp_Form() noexcept;
  void setStateControls(bool op) noexcept;

  void writeSettings() const noexcept;
  void readSettings();
  void reject_form() noexcept;

  void handleToggleAnimation(bool checked);
  void setupAnimation();

  void applyIcons() noexcept;

protected:
  void closeEvent(QCloseEvent *event) override;
};