#pragma once

#include "util/helper.hpp"
#include <QDialog>

namespace Ui { class ConfigDialog; }

class ConfigDialog : public QDialog
{
  Q_OBJECT

public:
  explicit ConfigDialog(Qt::ColorScheme currentScheme, bool isFusionActive, QWidget *parent = nullptr);
  ~ConfigDialog();

  // Retorna el esquema seleccionado por el usuario
  Qt::ColorScheme selectedScheme() const noexcept;

  DbConfig getDbConfig() const noexcept;
  void setDbConfig(const DbConfig& config) noexcept;

  void setCurrentPage(int index);


private:
  Ui::ConfigDialog *ui;

  Qt::ColorScheme selectedScheme_{Qt::ColorScheme::Unknown};
  Qt::ColorScheme originalScheme_{Qt::ColorScheme::Unknown};

  bool selectedStyle_{false};
  bool originalStyle_{false};

  void initDialog() noexcept;
  void setCurrentTheme(Qt::ColorScheme scheme) noexcept;
  void applyThemeSelection() noexcept;

  void saveLastSelection();
  void restoreLastSelection();

private slots:
  void on_btnSystem_clicked();
  void on_btnLight_clicked();
  void on_btnDark_clicked();
  void on_btnOk_clicked();
  void on_btnApply_clicked();
  void on_btnCancel_clicked();
  void on_listMenu_currentRowChanged(int row);

  void on_btnTestDB_clicked();

signals:
  void themeChanged(Qt::ColorScheme scheme);
  void styleChanged(bool style);
  void dbConfigSaved(const DbConfig& config);

  // QWidget interface
protected:
  virtual void closeEvent(QCloseEvent *event) override;
};
