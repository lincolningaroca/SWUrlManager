#pragma once

#include "util/helper.hpp"
#include <QDialog>

namespace Ui { class ConfigDialog; }

class ConfigDialog : public QDialog
{
  Q_OBJECT

public:

  enum class ConfigSection : int {
	Appearance = 0,
	AppStyle = 1,
	DataBase = 2,
	General = 3

  };

  explicit ConfigDialog(Qt::ColorScheme currentScheme, bool isFusionActive, QWidget *parent = nullptr);
  ~ConfigDialog();

  // Retorna el esquema seleccionado por el usuario
  Qt::ColorScheme selectedScheme() const noexcept;

  SW::DbConfig getDbConfig() const noexcept;
  void setDbConfig(const SW::DbConfig& config) noexcept;

  void setCurrentPage(int index);


private:
  Ui::ConfigDialog *ui;

  Qt::ColorScheme selectedScheme_{Qt::ColorScheme::Unknown};
  Qt::ColorScheme originalScheme_{Qt::ColorScheme::Unknown};

  bool selectedStyle_{false};
  bool originalStyle_{false};

  // // Idioma
  QString currentLang_{};
  // QString originalLanguage_{};

  void initDialog() noexcept;
  void setupLanguageCombo() noexcept;
  void setCurrentTheme(Qt::ColorScheme scheme) noexcept;
  void applyThemeSelection() noexcept;

  void applyLanguageSelection();
  void saveLastSelection();
  void restoreLastSelection();

  void applyAllStyles() noexcept;

  void setupUiConnections();

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
  void dbConfigSaved(const SW::DbConfig& config);

  // QWidget interface
protected:
  virtual void closeEvent(QCloseEvent *event) override;
  virtual void changeEvent(QEvent *event) override;
};
