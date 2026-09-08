#include "categorydialog.hpp"
#include "ui_categorydialog.h"

CategoryDialog::CategoryDialog(const QList<QPair<uint32_t, QString>> &categoryList, QWidget *parent) :
  QDialog(parent), ui(new Ui::CategoryDialog){
  ui->setupUi(this);

  setWindowFlags(windowFlags() | Qt::MSWindowsFixedSizeDialogHint);

  loadCategoryComboBox(categoryList);

  ui->cancelPushButton->setDefault(true);

  QObject::connect(ui->cancelPushButton, &QPushButton::clicked, this, [this](){ reject();});
  QObject::connect(ui->aceptPushButton, &QPushButton::clicked, this, [this](){ accept();});
}

CategoryDialog::~CategoryDialog(){
  delete ui;
}

uint32_t CategoryDialog::getCategoryId() const noexcept{
  return ui->categoryComboBox->currentData().isValid() ? ui->categoryComboBox->currentData().toUInt() : 1;

}

void CategoryDialog::loadCategoryComboBox(const QList<QPair<uint32_t, QString>>& categoryList) noexcept {

  QSignalBlocker blocker(ui->categoryComboBox);

  ui->categoryComboBox->clear();

  // Recorremos la lista manteniendo el orden exacto
  for (const auto& [id, name] : categoryList) {
	ui->categoryComboBox->addItem(name, id);
  }
}
