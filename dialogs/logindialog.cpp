#include "logindialog.hpp"
#include "ui_logindialog.h"

#include "resetpassworddialog.hpp"

#include <QCheckBox>
#include <QCloseEvent>
#include <QLineEdit>
#include <QMessageBox>
#include <QPropertyAnimation>
#include <QSettings>
#include <QTimer>


LogInDialog::LogInDialog(QWidget *parent, OpenMode op) :
  QDialog(parent), ui(new Ui::LogInDialog), isExpanded_(false){

  ui->setupUi(this);

  setupAnimation();


  setUp_Form();
  readSettings();
  ui->pbLogIn->setDefault(true);


  QObject::connect(ui->pbCancel, &QPushButton::clicked, this, &LogInDialog::reject_form);

  QObject::connect(ui->pbLogIn, &QPushButton::clicked, this, [this](){

    if(!helperdb_.logIn(ui->txtUser->text().simplified(), ui->txtPassword->text().simplified())){
      QMessageBox::warning(this, SW::Helper_t::appName(), QStringLiteral("<span>"
                                                                         "<strong>"
                                                                         "Los datos que ingreso son incorrectos\n"
                                                                         "vuelva a intentarlo."
                                                                         "</strong>"
                                                                         "</span>"));
      ui->txtUser->selectAll();
      ui->txtUser->setFocus(Qt::OtherFocusReason);

      return;
    }

    userName_ = ui->txtUser->text();
    accept();
  });

  //coneccion del boton de mas opciones
  ui->btnOtherOptions->setCheckable(true);


  QObject::connect(ui->btnOtherOptions, &QToolButton::toggled, this, &LogInDialog::handleToggleAnimation);

  //coneccion de combo box metodo de recuperacion
  QObject::connect(ui->cboRestoreType, &QComboBox::currentIndexChanged, this, &LogInDialog::setOptionsToComboBox);


  //connect to create user button
  QObject::connect(ui->btnCreateUser, &QAbstractButton::clicked, this, [this, op](){

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

      if(op == NO_STATE)
        ui->btnOtherOptions->toggle();
      else
        accept();
    }


  });

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

  QObject::connect(ui->chkGenPassword, &QCheckBox::clicked, this, [this](bool checked){

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
  });

  //gen passowrd button
  QObject::connect(ui->btnGenPassword, &QPushButton::clicked, this, [this](){

    const auto password{SW::Helper_t::generateSecurePassword()};
    ui->txtNewPassword->setText(password);
    ui->txtRePassword->setText(password);
  });

  applyIcons();

}//end constructor

LogInDialog::~LogInDialog()
{
  delete ui;
}

QString LogInDialog::getTextForAuthType(SW::AuthType type) const {
  for (int i = 0; i < ui->cboRestoreType->count(); ++i) {
	if (ui->cboRestoreType->itemData(i).value<SW::AuthType>() == type) {
	  return ui->cboRestoreType->itemText(i);
	}
  }
  return QString();
}
void LogInDialog::setToggledToButton(bool op){

  setWindowTitle("Crear nuevo usuario.");
  ui->groupBox->setDisabled(op);
  ui->btnOtherOptions->toggle();
  ui->btnOtherOptions->setDisabled(op);
  ui->btnResetPassword->setVisible(!op);

}

void LogInDialog::setUp_Form() noexcept{

  setWindowTitle(SW::Helper_t::appName().append(" - inicio de sesión"));

  ui->txtUser->setPlaceholderText("Usuario");
  ui->txtUser->setClearButtonEnabled(true);

  ui->txtPassword->setEchoMode(QLineEdit::Password);
  ui->txtPassword->setPlaceholderText("Clave o password");
  ui->txtPassword->setClearButtonEnabled(true);

  ui->btnOtherOptions->setIcon(QIcon(QStringLiteral(":/img/down.svg")));
  layout()->setSizeConstraint(QLayout::SetFixedSize);

  ui->btnOtherOptions->setToolTip("<p>"
                                  "<span> Crear un nuevo usuario y/o<br>"
                                  "restablecer clave o password!"
                                  "</span>"
                                  "</p>");
  ui->btnGenPassword->setDisabled(true);


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

void LogInDialog::setStateControls(bool op) noexcept{

  ui->txtPassword->setDisabled(op);
  ui->txtUser->setDisabled(op);
  ui->pbCancel->setDisabled(op);
  ui->pbLogIn->setDisabled(op);

}

void LogInDialog::applyIcons() noexcept {

  const auto iconColor = SW::Helper_t::currentIconColor();

  // Botón expandir/colapsar — depende del estado actual
  const QString arrowIcon = isExpanded_ ? ":/img/up.svg" : ":/img/down.svg";
  ui->btnOtherOptions->setIcon(SW::Helper_t::svgIcon(arrowIcon, iconColor));

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

void LogInDialog::setOptionsToComboBox(int index) noexcept{

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


void LogInDialog::clearControls() noexcept{
  ui->txtNewUser->clear();
  ui->txtNewPassword->clear();
  ui->txtRePassword->clear();
  ui->txtfirstValue->clear();
  ui->txtConfirmValue->clear();
  ui->checkBox->setChecked(true);

}


bool LogInDialog::Validate_hasNoEmpty() const noexcept{
  return ui->txtNewUser->text().isEmpty() || ui->txtNewPassword->text().isEmpty() || ui->txtRePassword->text().isEmpty() ||
         ui->txtfirstValue->text().isEmpty() || ui->txtConfirmValue->text().isEmpty();
}

void LogInDialog::writeSettings() const noexcept{
  QSettings settings(qApp->organizationName(), SW::Helper_t::appName());

  settings.setValue(QStringLiteral("pos_login_form"), saveGeometry());

}

void LogInDialog::readSettings(){
  QSettings settings(qApp->organizationName(), SW::Helper_t::appName());

  restoreGeometry(settings.value(QStringLiteral("pos_login_form"), QByteArray()).toByteArray());

}

void LogInDialog::reject_form() noexcept{
  writeSettings();
  reject();
}

void LogInDialog::setFeatures(QLineEdit *lineEdit, QCheckBox *checkBox, bool checked) noexcept{

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

void LogInDialog::handleToggleAnimation(bool checked){

  if(checked){

	ui->btnOtherOptions->setIcon(QIcon(":/img/up.svg"));
    ui->btnOtherOptions->setToolTip("<span>Volver a Inicio de sesión!</span>");

    ui->widget->setMaximumHeight(QWIDGETSIZE_MAX);
    auto targetHeight = ui->widget->sizeHint().height();

    ui->widget->setMaximumHeight(0);

    collapseAnimation_->setStartValue(0);
    collapseAnimation_->setEndValue(targetHeight);

    collapseAnimation_->start();
    setStateControls(true);

    ui->txtNewUser->setFocus(Qt::OtherFocusReason);
    ui->btnCreateUser->setDefault(true);

    isExpanded_ = true;

  }else{

    // Cambiar icono a "abajo" para indicar que puede expandirse
	ui->btnOtherOptions->setIcon(QIcon(":/img/down.svg"));

    // Actualizar tooltip
    ui->btnOtherOptions->setToolTip("<span>"
                                    "Crear un nuevo usuario y/o<br>"
                                    "restablecer clave o password!"
                                    "</span>");

    collapseAnimation_->setStartValue(ui->widget->height());
    collapseAnimation_->setEndValue(0);

    // Iniciar la animación
    collapseAnimation_->start();

    // Habilitar el groupBox de inicio de sesión
    ui->groupBox->setEnabled(true);

    // Actualizar estado de controles
    setStateControls(false);

    // Devolver el foco al campo de usuario
    ui->txtUser->setFocus(Qt::OtherFocusReason);

    // Cambiar botón default
    ui->pbLogIn->setDefault(true);

    // Actualizar variable de estado
    isExpanded_ = false;
  }

  QTimer::singleShot(collapseAnimation_->duration(), this, &LogInDialog::adjustSize);
  applyIcons();

}

void LogInDialog::setupAnimation(){

  collapseAnimation_ = new QPropertyAnimation(ui->widget, "maximumHeight", this);
  collapseAnimation_->setDuration(300);
  collapseAnimation_->setEasingCurve(QEasingCurve::InOutQuad);

  ui->widget->setMaximumHeight(0);
  ui->widget->setVisible(true);

}

void LogInDialog::closeEvent(QCloseEvent *event){
  writeSettings();
  event->accept();
}
