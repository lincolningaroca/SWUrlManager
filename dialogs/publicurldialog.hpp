#pragma once

#include "helperdatabase/helperdb.hpp"
#include "util/helper.hpp"

#include <QDialog>
#include <QSqlDatabase>

namespace Ui {
class PublicUrlDialog;
}

class QAction;
struct SWTableModel;

class PublicUrlDialog : public QDialog{
  Q_OBJECT

public:
  explicit PublicUrlDialog(Qt::ColorScheme colorScheme, QWidget *parent = nullptr);
  ~PublicUrlDialog();

  void setShowGrid(bool checked);

private:
  Ui::PublicUrlDialog *ui;

  Qt::ColorScheme colorScheme_{};

  const QSqlDatabase db_{};
  SW::HelperDataBase_t helperdb_{};
  SWTableModel *model{nullptr};

  QAction *deleteUrl_{nullptr};
  QAction *editUrl_{nullptr};
  QAction *newUrl_{nullptr};
  QAction *openUrl_{nullptr};

  void loadDataComboBox();
  void writeSettings();
  void readSettings();

  void setupContextMenu();

  void applyIcons(Qt::ColorScheme scheme) noexcept;

  uint32_t currentCategoryId() const noexcept;
  void applyTablePalette(Qt::ColorScheme scheme);

private slots:
  void on_openUrl();
  void on_loadDataTableView();

  void on_showContextMenu(const QPoint& pos);

  void on_categorySelectedChanged(int index);
  void on_deleteUrl();
  void on_showMaintenanceDialog(SW::OpenMode mode);


  // QWidget interface
protected:
  virtual void closeEvent(QCloseEvent *event) override;
  virtual void showEvent(QShowEvent *event) override;
};

