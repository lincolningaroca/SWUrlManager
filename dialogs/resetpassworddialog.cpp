#include "resetpassworddialog.hpp"
#include "ui_resetpassworddialog.h"

#include "logindialog.hpp"
#include "mainform.hpp"
#include "util/helper.hpp"

#include <QAction>
#include <QMessageBox>
#include <QRegularExpressionValidator>

ResetPasswordDialog::ResetPasswordDialog(QWidget *parent)
  :QDialog(parent), ui(new Ui::ResetPasswordDialog){

  ui->setupUi(this);

  setInit_Form();

  QObject::connect(ui->stackedWidget, &QStackedWidget::currentChanged, this, [this](int index){

    ui->btnAtras->setDisabled(index == 0);

  });

  QObject::connect(ui->btnValidarUsuario, &QPushButton::clicked, this, [this](){

    if(ui->txtUser->text() == SW::Helper_t::defaultUser || ui->txtUser->text().isEmpty()){
      ui->txtUser->clear();
      ui->txtUser->setFocus(Qt::OtherFocusReason);
      return;
    }

    if(!helper.userExists(ui->txtUser->text().simplified())){
      QMessageBox::warning(this, SW::Helper_t::appName(),
                           QStringLiteral("<p><cite>Nombre de usuario incorrecto.</cite></p>"));
      ui->txtUser->selectAll();
      ui->txtUser->setFocus(Qt::OtherFocusReason);
      return;
    }
	const auto user = ui->txtUser->text().simplified();
    userId_ = helper.getUser_id(user, SW::User::U_user);
	const auto rescue_type = helper.validateRescueType(userId_);

	LogInDialog login(this);

	if(rescue_type == login.getTextForAuthType(SW::AuthType::Secret_Question)){
      ui->stackedWidget->setCurrentIndex(1);
      ui->txtPregunta->setPlainText(helper.getQuestion(userId_));
      ui->btnRespuesta->setDefault(true);
      ui->txtRespuesta->setFocus(Qt::OtherFocusReason);

    }
    else{
      ui->stackedWidget->setCurrentIndex(2);
      ui->btnClaveNumerica->setDefault(true);
      ui->txtPIN->setFocus(Qt::OtherFocusReason);
    }


  });

  //btn pregunta secreta
  QObject::connect(ui->btnRespuesta, &QAbstractButton::clicked, this, [this](){
    if(!helper.validateAnswer(ui->txtRespuesta->text(), userId_)){
      QMessageBox::warning(this, SW::Helper_t::appName(),
                           QStringLiteral("<p><cite>Su respuesta es incorrecta.</cite></p>"));
      ui->txtRespuesta->selectAll();
      ui->txtRespuesta->setFocus(Qt::OtherFocusReason);
      return;
    }

    ui->stackedWidget->setCurrentIndex(3);
    ui->txtNewPassword->setFocus(Qt::OtherFocusReason);
    ui->btnReset->setDefault(true);
  });

  //btn pin numerico
  QObject::connect(ui->btnClaveNumerica, &QPushButton::clicked, this, [this](){

    if(ui->txtPIN->text().size() < 4){
      QMessageBox::warning(this, SW::Helper_t::appName(), QStringLiteral("<span><em>El PIN numérico debe contener 4 digitos!</em></span>"));
      ui->txtPIN->selectAll();
      ui->txtPIN->setFocus(Qt::OtherFocusReason);
      return;
    }
    if(!helper.validateAnswer(ui->txtPIN->text(), userId_)){
      QMessageBox::warning(this, SW::Helper_t::appName(),
                           QStringLiteral("<p><em>El número que ingreso es incorrecto.</em></p>"));
      ui->txtPIN->selectAll();
      ui->txtPIN->setFocus(Qt::OtherFocusReason);
      return;
    }

    ui->stackedWidget->setCurrentIndex(3);
    ui->txtNewPassword->setFocus(Qt::OtherFocusReason);
    ui->btnReset->setDefault(true);
  });

  //btn volver atras

  QObject::connect(ui->btnAtras, &QPushButton::clicked, this, [this](){

    ui->txtUser->clear();
    ui->txtNewPassword->clear();
    ui->txtPIN->clear();
    ui->txtPregunta->clear();
    ui->txtRePassword->clear();
    ui->txtRespuesta->clear();

    ui->stackedWidget->setCurrentIndex(0);
    ui->btnValidarUsuario->setDefault(true);
    ui->txtUser->setFocus(Qt::OtherFocusReason);
  });

  //btn reset password
  QObject::connect(ui->btnReset, &QPushButton::clicked, this, [this](){
    if(ui->txtNewPassword->text().isEmpty()){
      QMessageBox::warning(this, SW::Helper_t::appName(), QStringLiteral("<span><em>Este campo es requerido.</em></span>"));
      ui->txtNewPassword->setFocus(Qt::OtherFocusReason);
      return;
    }
    if(ui->txtRePassword->text().isEmpty()){
      QMessageBox::warning(this, SW::Helper_t::appName(), QStringLiteral("<span><em>Este campo es requerido.</em></span>"));
      ui->txtRePassword->setFocus(Qt::OtherFocusReason);
      return;
    }
    if(ui->txtRePassword->text().simplified() != ui->txtNewPassword->text().simplified()){
      QMessageBox::warning(this, SW::Helper_t::appName(), QStringLiteral("<span><strong><em>La clave o password de confirmación no coincide.</em></strong></span>"));
      ui->txtRePassword->selectAll();
      ui->txtRePassword->setFocus(Qt::OtherFocusReason);
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
    if(!SW::Helper_t::isPasswordSecure(ui->txtRePassword->text().simplified())){
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
      ui->txtRePassword->setFocus();
      return;
    }
    if(helper.resetPassword(ui->txtRePassword->text(), userId_)){
      QMessageBox::information(this, SW::Helper_t::appName(), QStringLiteral("<span><em>Su clave o password de acceso fue cambiado con éxito!</em></strong></span>"));

      accept();
    }
  });
}// fin del constructor

ResetPasswordDialog::~ResetPasswordDialog(){
  delete ui;
}

void ResetPasswordDialog::setFeatures(QLineEdit *w, QCheckBox *b, bool checked) noexcept {

  const auto iconColor = SW::Helper_t::currentIconColor();

  if(checked){
	w->setEchoMode(QLineEdit::Normal);
	b->setIcon(SW::Helper_t::svgIcon(":/img/open.svg", iconColor));
	b->setToolTip("Ocultar los caracteres.");
  } else {
	w->setEchoMode(QLineEdit::Password);
	b->setIcon(SW::Helper_t::svgIcon(":/img/close.svg", iconColor));
	b->setToolTip("Mostrar los caracteres.");
  }
}

void ResetPasswordDialog::setInit_Form() noexcept{

  setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
  ui->stackedWidget->setCurrentIndex(0);
  ui->txtUser->setClearButtonEnabled(true);

  if(ui->stackedWidget->currentIndex() == 0)   ui->btnAtras->setDisabled(true);

  const auto re = QRegularExpression(R"(^\d{4}$)");

  auto* validator = new QRegularExpressionValidator(re, this);
  ui->txtPIN->setValidator(validator);

  ui->txtPIN->setEchoMode(QLineEdit::Password);
  ui->txtNewPassword->setEchoMode(QLineEdit::Password);
  ui->txtRePassword->setEchoMode(QLineEdit::Password);
  ui->txtRespuesta->setEchoMode(QLineEdit::Password);


  auto msg = QString("<p>Al restaurar su password o clave, se le solicitará, los datos que ingresó,<br>"
					"al momento de registrar su usuario, segun sea el caso, que haya elegido<br>"
					"un <strong><i>PIN numérico</i></strong> o una <strong><i>pregunta secreta</i></strong>, para la restauración de su clave.</p>");
  ui->lblDesc->setText(msg);

  auto icon = QPixmap(":/img/contrasena.png").scaled(
	QSize(48, 48), Qt::KeepAspectRatio, Qt::SmoothTransformation);
  ui->lblIcon->setPixmap(icon);


  //iconos y acciones para QLineEdit para mostrar y/o ocultar los caracteres de los controles.
  QObject::connect(ui->checkBox, &QCheckBox::clicked, this, [this](bool checked){
    setFeatures(ui->txtRespuesta, ui->checkBox, checked);


  });
  QObject::connect(ui->checkBox_2, &QCheckBox::clicked, this, [this](bool checked){
    setFeatures(ui->txtPIN, ui->checkBox_2, checked);
  });
  QObject::connect(ui->checkBox_3, &QCheckBox::clicked, this, [this](bool checked){
    setFeatures(ui->txtNewPassword, ui->checkBox_3, checked);
  });
  QObject::connect(ui->checkBox_4, &QCheckBox::clicked, this, [this](bool checked){
    setFeatures(ui->txtRePassword, ui->checkBox_4, checked);
  });

  applyIcons();

}

void ResetPasswordDialog::applyIcons() noexcept {

  const auto iconColor = SW::Helper_t::currentIconColor();

  const QString eyeOpen   = ":/img/open.svg";
  const QString eyeClosed = ":/img/close.svg";

  ui->checkBox->setIcon(SW::Helper_t::svgIcon(
	ui->checkBox->isChecked() ? eyeOpen : eyeClosed, iconColor));
  ui->checkBox_2->setIcon(SW::Helper_t::svgIcon(
	ui->checkBox_2->isChecked() ? eyeOpen : eyeClosed, iconColor));
  ui->checkBox_3->setIcon(SW::Helper_t::svgIcon(
	ui->checkBox_3->isChecked() ? eyeOpen : eyeClosed, iconColor));
  ui->checkBox_4->setIcon(SW::Helper_t::svgIcon(
	ui->checkBox_4->isChecked() ? eyeOpen : eyeClosed, iconColor));
}
