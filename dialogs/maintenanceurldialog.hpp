#pragma once

#include "util/helper.hpp"

#include "helperdatabase/helperdb.hpp"

#include <QDialog>

class MidleWidget;

namespace Ui { class MaintenanceUrlDialog; }

class MaintenanceUrlDialog : public QDialog
{
  Q_OBJECT

public:
  explicit MaintenanceUrlDialog(Qt::ColorScheme colorScheme, SW::OpenMode mode,
								const QList<QVariant>& dataUrl,
								uint32_t categoryId,
								QWidget *parent = nullptr);
  ~MaintenanceUrlDialog();

private:
  Ui::MaintenanceUrlDialog *ui;

  const uint32_t currentCategoryId_{};
  MidleWidget *midleWidget{nullptr};
  SW::HelperDataBase_t helperdb_{};

  int id{};


  void writeSettings() const;
  void readSettings();

public slots:
  void on_acceptPushButton();


  // QWidget interface
protected:
  virtual void closeEvent(QCloseEvent *event) override;
};
