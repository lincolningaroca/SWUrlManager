#include "dlgnewcategory.hpp"
#include "ui_dlgnewcategory.h"

#include "helperdatabase/helperdb.hpp"

#include <QDialogButtonBox>
#include <QMessageBox>
#include <QPushButton>

dlgNewCategory::dlgNewCategory(SW::OpenMode mode, const std::optional<categoryData> &list,
  QWidget *parent) :
  QDialog(parent),
  ui(new Ui::dlgNewCategory),
  mode_(mode){

  ui->setupUi(this);
  setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
  initForm(list);

}


dlgNewCategory::~dlgNewCategory()
{
  delete ui;
}

QString dlgNewCategory::category() const noexcept{
  return ui->txtCategory->text().toUpper().simplified();

}

QString dlgNewCategory::description() const noexcept{
  return ui->pteDesc->toPlainText().toUpper().simplified();

}

QString dlgNewCategory::descriptionToolTip() const noexcept{
  return ui->pteDesc->toPlainText();
}

void dlgNewCategory::initForm(const std::optional<categoryData> &list){

  auto *buttonBox = new QDialogButtonBox(this);
  auto *okButton = buttonBox->addButton(tr("Crear categoría"), QDialogButtonBox::AcceptRole);
  auto *cancelButton = buttonBox->addButton(tr("Cancelar"), QDialogButtonBox::RejectRole);
  cancelButton->setDefault(true);
  this->layout()->addWidget(buttonBox);

  if(mode_ == SW::OpenMode::Edit){
	setWindowTitle(SW::Helper_t::appName() + tr(" - Editar datos de la categoría"));
	okButton->setText(tr("Actualizar datos"));

	if(list){
	  const auto& [name, desc] = list.value();
	  ui->txtCategory->setText(name);
	  ui->pteDesc->setPlainText(desc);

	}

  }else{

	setWindowTitle(SW::Helper_t::appName() + tr(" - Nueva categoría"));

  }
  QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &dlgNewCategory::accept);
  QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &dlgNewCategory::reject);

}

bool dlgNewCategory::validateData()  noexcept{
  if(ui->txtCategory->text().simplified().isEmpty()){
	  QMessageBox::warning(this, SW::Helper_t::appName(), tr("Debe ingresar un nombre de categoría!\n"));
      ui->txtCategory->setFocus(Qt::OtherFocusReason);
      return false;
    }
  return true;
}

void dlgNewCategory::accept(){

  SW::HelperDataBase_t helperdb_{};

  uint32_t userid {0};

  (SW::Helper_t::sessionStatus_ == SW::SessionStatus::Session_start) ?
	userid = helperdb_.getUser_id(SW::Helper_t::current_user_, SW::User::U_user).value() :
	userid = helperdb_.getUser_id(SW::Helper_t::current_user_, SW::User::U_public).value();

  if(!userid) return;

  if(mode_ == SW::OpenMode::New){
	if(validateData()){

	  if(helperdb_.categoryExists(ui->txtCategory->text().toUpper(), userid)){
		QMessageBox::warning(this, SW::Helper_t::appName(),
							 tr("<p>La categoría:<br>"
								"<b>\"%1\"</b>"
								", ya esta registrada en la base de datos.<br>"
								"Pruebe con otro nombre por favor!"
								"</p>").arg(ui->txtCategory->text().toUpper()));
		ui->txtCategory->selectAll();
		ui->txtCategory->setFocus(Qt::OtherFocusReason);
		return;
	  }
	  QDialog::accept();

	}
  }else{
	if(validateData())
	  QDialog::accept();
  }
}