#include "dlgnewcategory.hpp"
#include "ui_dlgnewcategory.h"

#include "helperdatabase/helperdb.hpp"

#include <QMessageBox>
#include <QPushButton>


dlgNewCategory::dlgNewCategory(SW::OpenMode mode, const QStringList &list, QWidget *parent) :
  QDialog(parent), ui(new Ui::dlgNewCategory){

  ui->setupUi(this);

  setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);

  auto cancelButton = ui->buttonBox->button(QDialogButtonBox::Cancel);
  cancelButton->setText("Cancelar");
  auto okButton = ui->buttonBox->button(QDialogButtonBox::Ok);


  if(static_cast<bool>(mode)){
      setWindowTitle(SW::Helper_t::appName().append(" - Editar datos de la categoría"));
      ui->txtCategory->setText(list.value(0));
      ui->pteDesc->setPlainText(list.value(1));
      okButton->setText("Actualizar datos");

    }else{

      setWindowTitle(SW::Helper_t::appName().append(" - Nueva categoría"));
      okButton->setText("Crear categoría");
    }


  QObject::connect(ui->buttonBox, &QDialogButtonBox::accepted, this, [this, mode](){

      SW::HelperDataBase_t helperdb_{};

      uint32_t userid {0};

	  (SW::Helper_t::sessionStatus_ == SW::SessionStatus::Session_start) ?
		userid = helperdb_.getUser_id(SW::Helper_t::current_user_, SW::User::U_user) :
		userid = helperdb_.getUser_id(SW::Helper_t::current_user_, SW::User::U_public);

      if(!static_cast<bool>(mode)){
          if(validateData()){

              if(helperdb_.categoryExists(ui->txtCategory->text().toUpper(), userid)){
                  QMessageBox::warning(this, SW::Helper_t::appName(),
                                       QString("<p><cite>La categoría: "
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

    });


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


bool dlgNewCategory::validateData()  noexcept{
  if(ui->txtCategory->text().simplified().isEmpty()){
      QMessageBox::warning(this, SW::Helper_t::appName(), QStringLiteral("Debe ingresar un nombre de categoría!\n"));
      ui->txtCategory->setFocus(Qt::OtherFocusReason);
      return false;
    }
  return true;
}


