#include "dlgnewcategory.hpp"
#include "ui_dlgnewcategory.h"

#include "helperdatabase/helperdb.hpp"

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

  QObject::connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &dlgNewCategory::onAcceptOption);
  QObject::connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);


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

  auto cancelButton = ui->buttonBox->button(QDialogButtonBox::Cancel);
  cancelButton->setText("Cancelar");
  auto okButton = ui->buttonBox->button(QDialogButtonBox::Ok);

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
	okButton->setText(tr("Crear categoría"));
  }

}

void dlgNewCategory::onAcceptOption(){

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
							 tr("<p><cite>La categoría: "
									 "<strong style='color:#ff0800;'>\"%1\""
									 "</strong>, ya esta registrada en la base de datos.<br>"
									 "pruebe con otro nombre por favor!"
									 "</cite>"
									 "</p>").arg(ui->txtCategory->text().toUpper()));
		ui->txtCategory->selectAll();
		ui->txtCategory->setFocus(Qt::OtherFocusReason);
		return;
	  }
	  accept();

	}
  }else{
	if(validateData())
	  accept();
  }

}


bool dlgNewCategory::validateData()  noexcept{
  if(ui->txtCategory->text().simplified().isEmpty()){
	  QMessageBox::warning(this, SW::Helper_t::appName(), tr("Debe ingresar un nombre de categoría!\n"));
      ui->txtCategory->setFocus(Qt::OtherFocusReason);
      return false;
    }
  return true;
}


