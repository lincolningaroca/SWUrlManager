#pragma once

#include <QDialog>

namespace Ui { class AcercaDeDialog; }

class AcercaDeDialog : public QDialog
{
  Q_OBJECT

public:
  explicit AcercaDeDialog(Qt::ColorScheme colorMode, QWidget *parent = nullptr);
  ~AcercaDeDialog();

private:
  Ui::AcercaDeDialog *ui;
  const Qt::ColorScheme colorMode_;
  QFont customFont_;

  void writeSettings() const;
  void readSettings();
  void loadInfo_app() const noexcept;
  void setTextToAbout() const;
  void setImage(Qt::ColorScheme colorMode);

  void setupCustomFont();
  void setupUI();
  void setupConnections();
  void showLicense();




  // QWidget interface
protected:
  virtual void closeEvent(QCloseEvent *event) override;
};
