#pragma once

#include <QDialog>

namespace Ui { class ChangePwdDialog; }

class ChangePwdDialog : public QDialog
{
  Q_OBJECT

public:
  explicit ChangePwdDialog(const QString& user, QWidget *parent = nullptr);
  ~ChangePwdDialog();

private:
  Ui::ChangePwdDialog *ui;

  QString user_{};

  void initDialog();
  void setFocusToWidget();

private slots:

  void on_acceptRole();
  void on_setPassword();
};
