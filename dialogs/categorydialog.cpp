#include "categorydialog.hpp"
#include "ui_categorydialog.h"

#include <QDialogButtonBox>
#include <QPushButton>

CategoryDialog::CategoryDialog(const QList<QPair<uint32_t, QString>> &categoryList, QWidget *parent) :
  QDialog(parent), ui(new Ui::CategoryDialog){
  ui->setupUi(this);

  setWindowFlags(windowFlags() | Qt::MSWindowsFixedSizeDialogHint);

  loadCategoryComboBox(categoryList);

  QDialogButtonBox *buttonBox = new QDialogButtonBox(this);
  buttonBox->addButton(tr("Cambiar de categoría"), QDialogButtonBox::AcceptRole);
  auto *cancelButton = buttonBox->addButton(tr("Cancelar"), QDialogButtonBox::RejectRole);
  cancelButton->setDefault(true);

  this->layout()->addWidget(buttonBox);  

  QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &CategoryDialog::accept);
  QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &CategoryDialog::reject);
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
