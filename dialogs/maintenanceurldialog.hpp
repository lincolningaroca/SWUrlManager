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
  MidleWidget *midleWidget_{nullptr};
  SW::HelperDataBase_t helperdb_{};
  uint32_t id_{};
  SW::OpenMode mode_;
  const QList<QVariant> &dataUrl_{};

  void writeSettings() const;
  void readSettings();
  void initForm();


  // QWidget interface
protected:
  virtual void closeEvent(QCloseEvent *event) override;

  // QDialog interface
public slots:
  virtual void accept() override;
};
