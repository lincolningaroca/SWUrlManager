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

  //coneccion de combo box metodo de recuperacion
  QObject::connect(ui->cboRestoreType, &QComboBox::currentIndexChanged, this, &CreateUserWidget::setOptionsToComboBox);
  //connect to create user button
  QObject::connect(ui->btnCreateUser, &QAbstractButton::clicked, this, &CreateUserWidget::on_btnCreateUser_clicked);

  QObject::connect(ui->btnResetPassword, &QPushButton::clicked, this, [this](){

	ResetPasswordDialog resetPassword{this};
	resetPassword.setWindowTitle(SW::Helper_t::appName().append(" - Restablecer clave o password"));
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

  QObject::connect(ui->chkGenPassword, &QCheckBox::clicked, this, &CreateUserWidget::on_chkGenPassword_clicked);

  //gen passowrd button
  QObject::connect(ui->btnGenPassword, &QPushButton::clicked, this, [this](){

	const auto password{SW::Helper_t::generateSecurePassword()};
	ui->txtNewPassword->setText(password);
	ui->txtRePassword->setText(password);
  });


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
  on_chkGenPassword_clicked(chkState);

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
	checkBox->setToolTip("Ocultar los caracteres.");
  } else {
	lineEdit->setEchoMode(QLineEdit::Password);
	checkBox->setIcon(SW::Helper_t::svgIcon(":/img/close.svg", iconColor));
	checkBox->setToolTip("Mostrar los caracteres.");
  }

}

void CreateUserWidget::on_chkGenPassword_clicked(bool checked){

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

void CreateUserWidget::on_btnCreateUser_clicked(bool checked){

  if(Validate_hasNoEmpty()){
	QMessageBox::warning(this, SW::Helper_t::appName(), QStringLiteral("<span><em>Todos los campos son requeridos!</em></span>"));
	ui->txtNewUser->setFocus();
	return;
  }

  if(ui->txtNewPassword->text().size() < 8 || ui->txtRePassword->text().size() < 8){
	QMessageBox::warning(this, SW::Helper_t::appName(),
						 QStringLiteral("<span>"
										"<em>"
										"El password o clave, debe tener 8 caracteres como mínimo."
										"</em>"
										"</span>"));
	ui->txtRePassword->selectAll();
	ui->txtRePassword->setFocus();
	return;
  }

  if(!SW::Helper_t::verify_Values(ui->txtNewPassword->text(), ui->txtRePassword->text())){
	QMessageBox::warning(this, SW::Helper_t::appName(),
						 QStringLiteral("<span>"
										"<strong>"
										"<em>"
										"El password o clave de confirmación no coincide!"
										"</em>"
										"</strong>"
										"</span>"));
	ui->txtRePassword->selectAll();
	ui->txtRePassword->setFocus();
	return;
  }

  if(!ui->chkGenPassword->isChecked()){

	if(!SW::Helper_t::isPasswordSecure(ui->txtRePassword->text())){
	  QMessageBox::warning(this, SW::Helper_t::appName(),
						   QStringLiteral("<span>"
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
	  ui->txtRePassword->selectAll();
	  ui->txtRePassword->setFocus(Qt::OtherFocusReason);
	  return;
	}

  }
  auto type = ui->cboRestoreType->currentData().value<SW::AuthType>();
  if(type == SW::AuthType::Numeric_pin){
	if(ui->txtfirstValue->text().size() < 4 || ui->txtConfirmValue->text().size() <4){
	  QMessageBox::warning(this, SW::Helper_t::appName(), QStringLiteral("<span><em>El PIN numérico debe contener 4 digitos!</em></span>"));
	  ui->txtfirstValue->selectAll();
	  ui->txtfirstValue->setFocus();
	  return;
	}
	if(!SW::Helper_t::verify_Values(ui->txtfirstValue->text(), ui->txtConfirmValue->text())){
	  QMessageBox::warning(this, SW::Helper_t::appName(), QStringLiteral("<span><strong><em>El número de confirmación no coincide!</em></strong></span>"));
	  ui->txtConfirmValue->selectAll();
	  ui->txtConfirmValue->setFocus();
	  return;
	}

  }


  if(helperdb_.userExists(ui->txtNewUser->text())){
	QMessageBox::warning(this, SW::Helper_t::appName(), tr("<span><em>El nombre de usuario: <strong>%1</strong> ya esta registrado.<br>"
														   "Vuelva a intentarlo con otro nombre porfavor!"
														   "</em></span>").arg(ui->txtNewUser->text()));
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
	QMessageBox::information(this, SW::Helper_t::appName(), QStringLiteral("<span><em>El nuevo usuario fue creado con éxito!</em></span>"));
	clearControls();

	emit userCreated();

  }

}

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
  ui->txtNewPassword->setPlaceholderText("Ingrese clave o password (mínimo 8 caracteres)");
  ui->txtNewPassword->setClearButtonEnabled(true);
  ui->txtNewPassword->setEchoMode(QLineEdit::Password);

  ui->txtRePassword->setPlaceholderText("Vuelva a ingresar su clave o password (mínimo 8 caracteres)");
  ui->txtRePassword->setEchoMode(QLineEdit::Password);
  ui->txtRePassword->setClearButtonEnabled(true);

  ui->txtNewUser->setPlaceholderText("Ingrese un nombre de usuario");
  ui->txtNewUser->setClearButtonEnabled(true);


  ui->txtfirstValue->setPlaceholderText("Ingrese una pregunta!");
  ui->txtfirstValue->setClearButtonEnabled(true);
  ui->txtfirstValue->setEchoMode(QLineEdit::Password);


  ui->txtConfirmValue->setPlaceholderText("Ingrese su respuesta!");
  ui->txtConfirmValue->setClearButtonEnabled(true);
  ui->txtConfirmValue->setEchoMode(QLineEdit::Password);

  //set the combo box options
  ui->cboRestoreType->addItem(QIcon(":/img/paper_pin.svg"), "Pin numérico", QVariant::fromValue(SW::AuthType::Numeric_pin));
  ui->cboRestoreType->addItem(QIcon(":/img/paper_pin.svg"), "Pregunta secreta", QVariant::fromValue(SW::AuthType::Secret_Question));
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

  auto type = ui->cboRestoreType->itemData(index).value<SW::AuthType>();

  if(type == SW::AuthType::Secret_Question){
	ui->txtfirstValue->clear();
	ui->txtConfirmValue->clear();
	ui->txtfirstValue->setPlaceholderText("Ingrese una pregunta!");
	ui->txtConfirmValue->setPlaceholderText("Ingrese su respuesta!");
	ui->txtfirstValue->setValidator(nullptr);
	ui->txtConfirmValue->setValidator(nullptr);
	ui->txtfirstValue->setFocus(Qt::OtherFocusReason);

  }else{
	ui->txtfirstValue->clear();
	ui->txtConfirmValue->clear();
	ui->txtfirstValue->setPlaceholderText("Ingrese PIN numérico de 4 cifras!");
	ui->txtConfirmValue->setPlaceholderText("Vuelva a ingresar el número");
	auto* validator = new QRegularExpressionValidator(QRegularExpression(QStringLiteral("^\\d{4}$")), this);
	ui->txtfirstValue->setValidator(validator);
	ui->txtConfirmValue->setValidator(validator);
	ui->txtfirstValue->setFocus(Qt::OtherFocusReason);

  }

}



