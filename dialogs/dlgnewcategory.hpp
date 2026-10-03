#pragma once

#include "util/helper.hpp"

#include <QDialog>


namespace Ui {
class dlgNewCategory;
}

class dlgNewCategory : public QDialog
{
  Q_OBJECT


public:

  using categoryData = std::pair<QString, QString>;
  explicit dlgNewCategory(SW::OpenMode mode, const std::optional<categoryData>& list = std::nullopt,
						  QWidget *parent = nullptr);

  ~dlgNewCategory();

  QString category() const noexcept;
  QString description() const noexcept;
  QString descriptionToolTip() const noexcept;

private:
  Ui::dlgNewCategory *ui;
  SW::OpenMode mode_;

  void initForm(const std::optional<categoryData>& list);

private slots:

  bool validateData() noexcept;


  // QDialog interface
public slots:
  virtual void accept() override;
};

