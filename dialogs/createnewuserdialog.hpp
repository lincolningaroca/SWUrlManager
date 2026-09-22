#pragma once

#include <QDialog>

namespace Ui { class CreateNewUserDialog; }

class CreateUserWidget;

class CreateNewUserDialog : public QDialog
{
  Q_OBJECT

public:
  explicit CreateNewUserDialog(QWidget *parent = nullptr);
  ~CreateNewUserDialog();

private:
  Ui::CreateNewUserDialog *ui;
  CreateUserWidget *createUserWidget_{nullptr};


  void writeSettings() const noexcept;
  void readSettings();

  // QWidget interface
protected:
  virtual void closeEvent(QCloseEvent *event) override;
};
