#include "changepwddialog.hpp"
#include "ui_changepwddialog.h"

#include "helperdatabase/helperdb.hpp"
#include "util/helper.hpp"

#include <QMessageBox>


ChangePwdDialog::ChangePwdDialog(const QString &user, QWidget *parent)
  : QDialog(parent), ui(new Ui::ChangePwdDialog),
  user_(user)
{
  ui->setupUi(this);

  initDialog();

  QObject::connect(ui->bBox, &QDialogButtonBox::accepted, this, on_acceptRole);
  QObject::connect(ui->bBox, &QDialogButtonBox::rejected, this, &ChangePwdDialog::reject);
  QObject::connect(ui->btnGenPassword, &QPushButton::clicked, this, &ChangePwdDialog::on_setPassword);

}

ChangePwdDialog::~ChangePwdDialog()
{
  delete ui;
}

void ChangePwdDialog::initDialog(){

  setWindowFlags(windowFlags() | Qt::MSWindowsFixedSizeDialogHint);
  setWindowTitle("Actualizar cantraseña");

  ui->lblUser->setText(QString("<strong>Estas a punto de cambiar o actualizar la clave de acceso para el usuario: <cite>\"%1\"</cite></strong>").arg(user_));
  ui->lblMessage->setText(QStringLiteral("<p><strong>La clave o contraseña, debe tener al menos una Mayuscula, un número y un caracter especial"
										 "<br>y una longitud mínina de 8 caracteres</strong></p>"));

  ui->txtNewPassword->setEchoMode(QLineEdit::Password);
  ui->txtRePassword->setEchoMode(QLineEdit::Password);

  ui->txtNewPassword->setClearButtonEnabled(true);
  ui->txtRePassword->setClearButtonEnabled(true);

  ui->btnGenPassword->setDisabled(true);

  ui->bBox->button(QDialogButtonBox::Ok)->setText("Cambiar clave");
  ui->bBox->button(QDialogButtonBox::Cancel)->setText("Cancelar");

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

void ChangePwdDialog::on_acceptRole(){

  if(ui->txtNewPassword->text().isEmpty() || ui->txtRePassword->text().isEmpty()){

	QMessageBox::warning(this, qApp->applicationName(), "Todos los campos son requeridos.");
	setFocusToWidget();
	return;

  }

  if(ui->txtNewPassword->text().length() < 8 || ui->txtRePassword->text().length() < 8){

	QMessageBox::warning(this, qApp->applicationName(), "La clave o contraseña debe tener como mínimo 8 caracteres.");
	setFocusToWidget();
	return;

  }

  if(ui->txtNewPassword->text().compare(ui->txtRePassword->text()) != 0){

	QMessageBox::warning(this, qApp->applicationName(), "Las contraseñas no coinciden.");
	setFocusToWidget();
	return;
  }

  if(!SW::Helper_t::isPasswordSecure(ui->txtNewPassword->text())){

	QMessageBox::warning(this, qApp->applicationName(), tr("<span>"
														   "<em>"
														   "Debe ingresar un password o clave segura!<br>"
														   "Nota:<br>"
														   "Para que un password o clave se considere seguro(a), debe cumplir con lo siguiente:"
														   "<ul>"
														   "<li>Debe contener al menos un caracter en mayuscula.</li>"
														   "<li>Debe contener al menos un caracter en minuscula.</li>"
														   "<li>Debe contener al menos un número.</li>"
														   "<li>Debe contener al menos un caracter especial por ejemplo: \"#$%&@\" etc...</li>"
														   "</ul>"
														   "Ejemplo de calve segura: <strong>\"MiClave@123\"</strong>"
														   "</em>"
														   "</span>"));
	setFocusToWidget();
	return;

  }

  SW::HelperDataBase_t helperDb{};

  // const auto user = user_.simplified();
  auto userId = helperDb.getUser_id(user_.simplified(), SW::User::U_user);

  if(helperDb.resetPassword(ui->txtNewPassword->text().simplified(), userId)){

	QMessageBox::information(this, qApp->applicationName(),
							 QString("<strong>Se cambio la clave o contraseña para el usuario: <cite>\"%1\"</cite>"
									 "<br>la próxima vez que inicie sesión, lo hará con su nueva clave.</strong>").arg(user_));
	accept();

  }

}

void ChangePwdDialog::on_setPassword(){

  const auto password{SW::Helper_t::generateSecurePassword()};
  ui->txtNewPassword->setText(password);
  ui->txtRePassword->setText(password);

}
