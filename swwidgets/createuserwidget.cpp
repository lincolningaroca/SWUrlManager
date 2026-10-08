#include "createuserwidget.hpp"
#include "ui_createuserwidget.h"

#include "dialogs/resetpassworddialog.hpp"


#include <QMessageBox>
#include <QSettings>
#include <QCheckBox>
#include <QLineEdit>

CreateUserWidget::CreateUserWidget(QWidget *parent)
  : QWidget(parent)
  , ui(new Ui::CreateUserWidget)
{
  ui->setupUi(this);

  setUp_Form();

  restoreControlStates();
  applyIcons();

  setupUiConnections();


}


CreateUserWidget::~CreateUserWidget()
{
  delete ui;
}

void CreateUserWidget::focusFirstField() noexcept {
  ui->txtNewUser->setFocus(Qt::OtherFocusReason);
}

void CreateUserWidget::saveControlStates() const{

  QSettings settings(qApp->organizationName(), qApp->applicationName());
  settings.beginGroup("createUserWidget");

  settings.setValue("chkGenPassword", ui->chkGenPassword->isChecked());
  settings.setValue("cboRestoreType", ui->cboRestoreType->currentText());

  settings.endGroup();

}

void CreateUserWidget::restoreControlStates(){

  QSettings settings(qApp->organizationName(), qApp->applicationName());
  settings.beginGroup("createUserWidget");

  auto chkState = settings.value("chkGenPassword", true).toBool();
  auto cboRestoreType_value = settings.value("cboRestoreType", QString()).toString();

  ui->chkGenPassword->setChecked(chkState);
  handleGenPasswordToggle(chkState);

  const auto findIndex = ui->cboRestoreType->findText(cboRestoreType_value);
  if(findIndex != -1){
	ui->cboRestoreType->setCurrentIndex(findIndex);
	setOptionsToComboBox(findIndex);

  }
  settings.endGroup();

}

void CreateUserWidget::setFeatures(QLineEdit *lineEdit, QCheckBox *checkBox, bool checked) noexcept{

  if(!lineEdit || !checkBox){
	return;
  }

  const QColor windowColor = qApp->palette().color(QPalette::Window);
  const bool isDark = (windowColor.lightness() < 128);
  const QColor iconColor = isDark ? QColor(220, 220, 220) : QColor(50, 50, 50);

  if(checked){
	lineEdit->setEchoMode(QLineEdit::Normal);
	checkBox->setIcon(SW::Helper_t::svgIcon(":/img/open.svg", iconColor));
	checkBox->setToolTip(tr("Ocultar los caracteres."));
  } else {
	lineEdit->setEchoMode(QLineEdit::Password);
	checkBox->setIcon(SW::Helper_t::svgIcon(":/img/close.svg", iconColor));
	checkBox->setToolTip(tr("Mostrar los caracteres."));
  }

}

void CreateUserWidget::setupUiConnections(){

  //coneccion de combo box metodo de recuperacion
  QObject::connect(ui->cboRestoreType, &QComboBox::currentIndexChanged, this, &CreateUserWidget::setOptionsToComboBox);
  //connect to create user button
  QObject::connect(ui->btnCreateUser, &QAbstractButton::clicked, this, &CreateUserWidget::handleCreateUserClicked);

  QObject::connect(ui->btnResetPassword, &QPushButton::clicked, this, [this](){

	ResetPasswordDialog resetPassword{this};
	resetPassword.setWindowTitle(SW::Helper_t::appName() + tr(" - Restablecer clave o password"));
	resetPassword.exec();

  });

  QObject::connect(ui->checkBox_2, &QCheckBox::clicked, this, [this](bool checked){
	setFeatures(ui->txtNewPassword, ui->checkBox_2, checked);
  });
  QObject::connect(ui->checkBox_3, &QCheckBox::clicked, this, [this](bool checked){
	setFeatures(ui->txtRePassword, ui->checkBox_3, checked);
  });
  QObject::connect(ui->checkBox_4, &QCheckBox::clicked, this, [this](bool checked){
	setFeatures(ui->txtfirstValue, ui->checkBox_4, checked);
  });
  QObject::connect(ui->checkBox_5, &QCheckBox::clicked, this, [this](bool checked){
	setFeatures(ui->txtConfirmValue, ui->checkBox_5, checked);
  });

  QObject::connect(ui->chkGenPassword, &QCheckBox::clicked, this, &CreateUserWidget::handleGenPasswordToggle);

  //gen passowrd button
  QObject::connect(ui->btnGenPassword, &QPushButton::clicked, this, [this](){

	const auto password{SW::Helper_t::generateSecurePassword()};
	ui->txtNewPassword->setText(password);
	ui->txtRePassword->setText(password);
  });

}

void CreateUserWidget::handleGenPasswordToggle(bool checked){

  if(checked){

	ui->txtNewPassword->setEchoMode(QLineEdit::Normal);
	ui->txtRePassword->setEchoMode(QLineEdit::Normal);
	ui->txtNewPassword->clear();
	ui->txtRePassword->clear();
  }else{
	ui->txtNewPassword->setEchoMode(QLineEdit::Password);
	ui->txtRePassword->setEchoMode(QLineEdit::Password);
	ui->txtNewPassword->clear();
	ui->txtRePassword->clear();
  }

  ui->btnGenPassword->setEnabled(checked);
  ui->txtNewPassword->setReadOnly(checked);
  ui->txtRePassword->setReadOnly(checked);
  ui->checkBox_2->setChecked(checked);
  ui->checkBox_3->setChecked(checked);
  ui->checkBox_2->setDisabled(checked);
  ui->checkBox_3->setDisabled(checked);

}

void CreateUserWidget::handleCreateUserClicked(){

  if(Validate_hasNoEmpty()){
	QMessageBox::warning(this, SW::Helper_t::appName(), tr("Todos los campos son requeridos!"));
	ui->txtNewUser->setFocus();
	return;
  }

  if(ui->txtNewPassword->text().size() < 8 || ui->txtRePassword->text().size() < 8){
	QMessageBox::warning(this, SW::Helper_t::appName(), tr("El password o clave, debe tener 8 caracteres como mínimo."));
	ui->txtRePassword->selectAll();
	ui->txtRePassword->setFocus();
	return;
  }

  if(!SW::Helper_t::verify_Values(ui->txtNewPassword->text(), ui->txtRePassword->text())){
	QMessageBox::warning(this, SW::Helper_t::appName(), tr("El password o clave de confirmación no coincide!"));
	ui->txtRePassword->selectAll();
	ui->txtRePassword->setFocus();
	return;
  }

  if(!ui->chkGenPassword->isChecked()){

	if(!SW::Helper_t::isPasswordSecure(ui->txtRePassword->text())){
	  QMessageBox::warning(this, SW::Helper_t::appName(), tr("<p><b>Debe ingresar una contraseña segura.</b>"
															 "Requisitos mínimos:"
															 "<ul>"
															 "<li>Al menos una letra mayúscula</li>"
															 "<li>Al menos una letra minúscula</li>"
															 "<li>Al menos un número</li>"
															 "<li>Al menos un carácter especial (ej. #$%&@)</li>"
															 "</ul>Ejemplo de clave segura: <b>MiClave@123</b></p>"));
	  ui->txtRePassword->selectAll();
	  ui->txtRePassword->setFocus(Qt::OtherFocusReason);
	  return;
	}

  }

  const auto type = static_cast<SW::AuthType>(ui->cboRestoreType->currentData().toInt());
  if(type == SW::AuthType::Numeric_pin){
	if(ui->txtfirstValue->text().size() < 4 || ui->txtConfirmValue->text().size() <4){
	  QMessageBox::warning(this, SW::Helper_t::appName(), tr("El PIN numérico debe contener 4 digitos!"));
	  ui->txtfirstValue->selectAll();
	  ui->txtfirstValue->setFocus();
	  return;
	}
	if(!SW::Helper_t::verify_Values(ui->txtfirstValue->text(), ui->txtConfirmValue->text())){
	  QMessageBox::warning(this, SW::Helper_t::appName(), tr("El número de confirmación no coincide!"));
	  ui->txtConfirmValue->selectAll();
	  ui->txtConfirmValue->setFocus();
	  return;
	}

  }

  if(helperdb_.userExists(ui->txtNewUser->text())){
	QMessageBox::warning(this, SW::Helper_t::appName(), tr("El nombre de usuario <b>%1</b> ya está registrado.<br>"
														   "Por favor, intente con otro nombre.").arg(
															ui->txtNewUser->text().simplified()));
	ui->txtNewUser->selectAll();
	ui->txtNewUser->setFocus(Qt::OtherFocusReason);
	return;
  }

  const auto user = ui->txtNewUser->text();

  const auto password = ui->txtRePassword->text();
  QString first_value =ui->txtfirstValue->text();
  QString confirm_value = ui->txtConfirmValue->text();

  if(helperdb_.createUser(user, password, SW::Helper_t::currentUser_.value(SW::User::U_user),
						   ui->cboRestoreType->currentText(), first_value, confirm_value)){
	QMessageBox::information(this, SW::Helper_t::appName(), tr("El nuevo usuario fue creado con éxito!"));
	clearControls();

	emit userCreated();

  }

}
// void CreateUserWidget::handleCreateUserClicked(){

//   if(Validate_hasNoEmpty()){
// 	QMessageBox::warning(this, SW::Helper_t::appName(), tr("Todos los campos son requeridos!"));
// 	ui->txtNewUser->setFocus();
// 	return;
//   }

//   if(ui->txtNewPassword->text().size() < 8 || ui->txtRePassword->text().size() < 8){
// 	QMessageBox::warning(this, SW::Helper_t::appName(), tr("El password o clave, debe tener 8 caracteres como mínimo."));
// 	ui->txtRePassword->selectAll();
// 	ui->txtRePassword->setFocus();
// 	return;
//   }

//   if(!SW::Helper_t::verify_Values(ui->txtNewPassword->text(), ui->txtRePassword->text())){
// 	QMessageBox::warning(this, SW::Helper_t::appName(), tr("El password o clave de confirmación no coincide!"));
// 	ui->txtRePassword->selectAll();
// 	ui->txtRePassword->setFocus();
// 	return;
//   }

//   if(!ui->chkGenPassword->isChecked()){

// 	if(!SW::Helper_t::isPasswordSecure(ui->txtRePassword->text())){
// 	  QMessageBox::warning(this, SW::Helper_t::appName(), tr("<p><b>Debe ingresar una contraseña segura.</b>"
// 															 "Requisitos mínimos:"
// 															 "<ul>"
// 															 "<li>Al menos una letra mayúscula</li>"
// 															 "<li>Al menos una letra minúscula</li>"
// 															 "<li>Al menos un número</li>"
// 															 "<li>Al menos un carácter especial (ej. #$%&@)</li>"
// 															 "</ul>Ejemplo de clave segura: <b>MiClave@123</b></p>"));
// 	  ui->txtRePassword->selectAll();
// 	  ui->txtRePassword->setFocus(Qt::OtherFocusReason);
// 	  return;
// 	}

//   }
//   auto type = ui->cboRestoreType->currentData().value<SW::AuthType>();
//   if(type == SW::AuthType::Numeric_pin){
// 	if(ui->txtfirstValue->text().size() < 4 || ui->txtConfirmValue->text().size() <4){
// 	  QMessageBox::warning(this, SW::Helper_t::appName(), tr("El PIN numérico debe contener 4 digitos!"));
// 	  ui->txtfirstValue->selectAll();
// 	  ui->txtfirstValue->setFocus();
// 	  return;
// 	}
// 	if(!SW::Helper_t::verify_Values(ui->txtfirstValue->text(), ui->txtConfirmValue->text())){
// 	  QMessageBox::warning(this, SW::Helper_t::appName(), tr("El número de confirmación no coincide!"));
// 	  ui->txtConfirmValue->selectAll();
// 	  ui->txtConfirmValue->setFocus();
// 	  return;
// 	}

//   }


//   if(helperdb_.userExists(ui->txtNewUser->text())){
// 	QMessageBox::warning(this, SW::Helper_t::appName(), tr("El nombre de usuario <b>%1</b> ya está registrado.<br>"
// 														   "Por favor, intente con otro nombre.").arg(
// 															ui->txtNewUser->text().simplified()));
// 	ui->txtNewUser->selectAll();
// 	ui->txtNewUser->setFocus(Qt::OtherFocusReason);
// 	return;
//   }

//   const auto user = ui->txtNewUser->text();

//   const auto password = ui->txtRePassword->text();
//   QString first_value =ui->txtfirstValue->text();
//   QString confirm_value = ui->txtConfirmValue->text();

//   if(helperdb_.createUser(user, password, SW::Helper_t::currentUser_.value(SW::User::U_user),
// 						   ui->cboRestoreType->currentText(), first_value, confirm_value)){
// 	QMessageBox::information(this, SW::Helper_t::appName(), tr("El nuevo usuario fue creado con éxito!"));
// 	clearControls();

// 	emit userCreated();

//   }

// }

bool CreateUserWidget::Validate_hasNoEmpty() const noexcept{
  return ui->txtNewUser->text().isEmpty() || ui->txtNewPassword->text().isEmpty() || ui->txtRePassword->text().isEmpty() ||
		 ui->txtfirstValue->text().isEmpty() || ui->txtConfirmValue->text().isEmpty();
}

void CreateUserWidget::clearControls() noexcept{

  ui->txtNewUser->clear();
  ui->txtNewPassword->clear();
  ui->txtRePassword->clear();
  ui->txtfirstValue->clear();
  ui->txtConfirmValue->clear();
  ui->checkBox->setChecked(true);

}

void CreateUserWidget::setUp_Form() noexcept{

  //new user section
  ui->txtNewPassword->setPlaceholderText(tr("Ingrese una clave (mínimo 8 caracteres)"));
  ui->txtNewPassword->setClearButtonEnabled(true);
  ui->txtNewPassword->setEchoMode(QLineEdit::Password);

  ui->txtRePassword->setPlaceholderText(tr("Vuelva a ingresar su clave"));
  ui->txtRePassword->setEchoMode(QLineEdit::Password);
  ui->txtRePassword->setClearButtonEnabled(true);

  ui->txtNewUser->setPlaceholderText(tr("Ingrese un nombre de usuario"));
  ui->txtNewUser->setClearButtonEnabled(true);


  ui->txtfirstValue->setPlaceholderText(tr("Ingrese una pregunta!"));
  ui->txtfirstValue->setClearButtonEnabled(true);
  ui->txtfirstValue->setEchoMode(QLineEdit::Password);


  ui->txtConfirmValue->setPlaceholderText(tr("Ingrese su respuesta!"));
  ui->txtConfirmValue->setClearButtonEnabled(true);
  ui->txtConfirmValue->setEchoMode(QLineEdit::Password);

  //set the combo box options
  ui->cboRestoreType->addItem(QIcon(":/img/paper_pin.svg"), tr("Pin numérico"),
							  static_cast<int>(SW::AuthType::Numeric_pin));
  ui->cboRestoreType->addItem(QIcon(":/img/paper_pin.svg"), tr("Pregunta secreta"),
							  static_cast<int>(SW::AuthType::Secret_Question));
  ui->checkBox->setChecked(true);
  ui->checkBox->setDisabled(true);


}

void CreateUserWidget::applyIcons() noexcept{

  const auto iconColor = SW::Helper_t::currentIconColor();


  // Mostrar/ocultar password — depende del estado de cada checkbox
  const QString eyeOpen   = ":/img/open.svg";
  const QString eyeClosed = ":/img/close.svg";

  ui->checkBox_2->setIcon(SW::Helper_t::svgIcon(
	ui->checkBox_2->isChecked() ? eyeOpen : eyeClosed, iconColor));
  ui->checkBox_3->setIcon(SW::Helper_t::svgIcon(
	ui->checkBox_3->isChecked() ? eyeOpen : eyeClosed, iconColor));
  ui->checkBox_4->setIcon(SW::Helper_t::svgIcon(
	ui->checkBox_4->isChecked() ? eyeOpen : eyeClosed, iconColor));
  ui->checkBox_5->setIcon(SW::Helper_t::svgIcon(
	ui->checkBox_5->isChecked() ? eyeOpen : eyeClosed, iconColor));

  // Combo box autenticación
  ui->cboRestoreType->setItemIcon(0, SW::Helper_t::svgIcon(":/img/paper_pin.svg", iconColor));
  ui->cboRestoreType->setItemIcon(1, SW::Helper_t::svgIcon(":/img/paper_pin.svg", iconColor));

}

void CreateUserWidget::setOptionsToComboBox(int index) noexcept{

  if(index < 0) return;

  const auto type = static_cast<SW::AuthType>(ui->cboRestoreType->itemData(index).toInt());

  if(type == SW::AuthType::Secret_Question){
	ui->txtfirstValue->clear();
	ui->txtConfirmValue->clear();
	ui->txtfirstValue->setPlaceholderText(tr("Ingrese una pregunta!"));
	ui->txtConfirmValue->setPlaceholderText(tr("Ingrese su respuesta!"));
	ui->txtfirstValue->setValidator(nullptr);
	ui->txtConfirmValue->setValidator(nullptr);
	ui->txtfirstValue->setFocus(Qt::OtherFocusReason);

  }else{
	ui->txtfirstValue->clear();
	ui->txtConfirmValue->clear();
	ui->txtfirstValue->setPlaceholderText(tr("Ingrese PIN numérico de 4 cifras!"));
	ui->txtConfirmValue->setPlaceholderText(tr("Vuelva a ingresar el número"));
	auto* validator = new QRegularExpressionValidator(QRegularExpression(QStringLiteral("^\\d{4}$")), this);
	ui->txtfirstValue->setValidator(validator);
	ui->txtConfirmValue->setValidator(validator);
	ui->txtfirstValue->setFocus(Qt::OtherFocusReason);

  }

}



