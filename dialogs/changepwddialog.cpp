#include "changepwddialog.hpp"
#include "ui_changepwddialog.h"

#include "helperdatabase/helperdb.hpp"
#include "util/helper.hpp"

#include <QDialogButtonBox>
#include <QMessageBox>
#include <QPushButton>

ChangePwdDialog::ChangePwdDialog(const QString &user, QWidget *parent)
  : QDialog(parent), ui(new Ui::ChangePwdDialog),
  user_(user)
{
  ui->setupUi(this);

  initDialog();

  QObject::connect(ui->btnGenPassword, &QPushButton::clicked, this, &ChangePwdDialog::on_setPassword);

}

ChangePwdDialog::~ChangePwdDialog()
{
  delete ui;
}

void ChangePwdDialog::initDialog(){

  setWindowFlags(windowFlags() | Qt::MSWindowsFixedSizeDialogHint);
  setWindowTitle(tr("Actualizar cantraseña"));

  ui->lblUser->setText(tr("<p>Estas a punto de cambiar o actualizar la clave de acceso para el usuario: <b>\"%1\"</b></p>").arg(user_));
  ui->lblMessage->setText(tr("<p><b>La clave o contraseña, debe tener al menos una Mayuscula, un número y un caracter especial"
							 "<br>y una longitud mínina de 8 caracteres</b></p>"));

  ui->txtNewPassword->setEchoMode(QLineEdit::Password);
  ui->txtRePassword->setEchoMode(QLineEdit::Password);

  ui->txtNewPassword->setClearButtonEnabled(true);
  ui->txtRePassword->setClearButtonEnabled(true);

  ui->btnGenPassword->setDisabled(true);

  auto *buttonBox = new QDialogButtonBox(this);
  buttonBox->addButton(tr("Cambiar clave"), QDialogButtonBox::AcceptRole);
  auto *cancelButton = buttonBox->addButton(tr("Cancelar"), QDialogButtonBox::RejectRole);
  cancelButton->setDefault(true);

  this->layout()->addWidget(buttonBox);

  QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &ChangePwdDialog::accept);
  QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &ChangePwdDialog::reject);


  QObject::connect(ui->chkGenPassword, &QCheckBox::toggled, this, [this](bool state){

	ui->btnGenPassword->setEnabled(state);
	if(!state){
	  ui->txtNewPassword->setFocus(Qt::OtherFocusReason);
	  if(!ui->txtNewPassword->text().isEmpty()){
		ui->txtNewPassword->selectAll();
	  }

	}
  });

  QObject::connect(ui->chkShowPwd, &QCheckBox::toggled, this, [this](bool state){

	if(state){
	  ui->txtNewPassword->setEchoMode(QLineEdit::Normal);
	  ui->txtRePassword->setEchoMode(QLineEdit::Normal);
	}else{
	  ui->txtNewPassword->setEchoMode(QLineEdit::Password);
	  ui->txtRePassword->setEchoMode(QLineEdit::Password);
	}

  });

}

void ChangePwdDialog::setFocusToWidget(){

  ui->txtNewPassword->setFocus(Qt::OtherFocusReason);
  if(!ui->txtNewPassword->text().isEmpty())
	ui->txtNewPassword->selectAll();


}

void ChangePwdDialog::on_setPassword(){

  const auto password{SW::Helper_t::generateSecurePassword()};
  ui->txtNewPassword->setText(password);
  ui->txtRePassword->setText(password);

}


void ChangePwdDialog::accept(){

  if(ui->txtNewPassword->text().isEmpty() || ui->txtRePassword->text().isEmpty()){

	QMessageBox::warning(this, qApp->applicationName(), tr("Todos los campos son requeridos."));
	setFocusToWidget();
	return;

  }

  if(ui->txtNewPassword->text().length() < 8 || ui->txtRePassword->text().length() < 8){

	QMessageBox::warning(this, qApp->applicationName(), tr("La clave o contraseña debe tener como mínimo 8 caracteres."));
	setFocusToWidget();
	return;

  }

  if(ui->txtNewPassword->text().compare(ui->txtRePassword->text()) != 0){

	QMessageBox::warning(this, qApp->applicationName(), tr("Las contraseñas no coinciden."));
	setFocusToWidget();
	return;
  }

  if(!SW::Helper_t::isPasswordSecure(ui->txtNewPassword->text())){

	QMessageBox::warning(this, qApp->applicationName(), tr("<p>Debe ingresar un password o clave segura!<br>"
														   "Nota:<br>"
														   "Para que un password o clave se considere seguro(a), debe cumplir con lo siguiente:"
														   "<ul>"
														   "<li>Debe contener al menos un caracter en mayuscula.</li>"
														   "<li>Debe contener al menos un caracter en minuscula.</li>"
														   "<li>Debe contener al menos un número.</li>"
														   "<li>Debe contener al menos un caracter especial por ejemplo: \"#$%&@\" etc...</li>"
														   "</ul>"
														   "Ejemplo de calve segura: <b>\"MiClave@123\"</b></p>"));
	setFocusToWidget();
	return;

  }

  SW::HelperDataBase_t helperDb{};

  const auto userId = helperDb.getUser_id(user_.simplified(), SW::User::U_user);
  if(!userId) return;

  if(helperDb.resetPassword(ui->txtNewPassword->text().simplified(), userId.value())){

	QMessageBox::information(this, qApp->applicationName(),
							 tr("<p>Se cambio la clave o contraseña para el usuario: <b>\"%1\"</b>"
								"<br>la próxima vez que inicie sesión, lo hará con su nueva clave.</p>").arg(user_));
	QDialog::accept();

  }
}