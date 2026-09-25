#include "logindialog.hpp"
#include "ui_logindialog.h"

#include "swwidgets/createuserwidget.hpp"

#include <QCloseEvent>
#include <QLineEdit>
#include <QMessageBox>
#include <QPropertyAnimation>
#include <QSettings>
#include <QTimer>


LogInDialog::LogInDialog(QWidget *parent) :
  QDialog(parent), ui(new Ui::LogInDialog), isExpanded_(false){

  ui->setupUi(this);

  createUserWidget_ = new CreateUserWidget(this);
  ui->creatUserLayout->addWidget(createUserWidget_);

  setupAnimation();

  setUp_Form();
  applyIcons();

  readSettings();
  ui->pbLogIn->setDefault(true);  

  setupUiConnections();

}//end constructor

LogInDialog::~LogInDialog()
{
  delete ui;
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
  ui->txtUser->setFocus();

}

void LogInDialog::setStateControls(bool op) noexcept{

  ui->txtPassword->setDisabled(op);
  ui->txtUser->setDisabled(op);
  ui->pbCancel->setDisabled(op);
  ui->pbLogIn->setDisabled(op);

}

void LogInDialog::applyIcons() noexcept {

  const auto iconColor = SW::Helper_t::currentIconColor();

  const QString arrowIcon = isExpanded_ ? ":/img/up.svg" : ":/img/down.svg";
  ui->btnOtherOptions->setIcon(SW::Helper_t::svgIcon(arrowIcon, iconColor));
}

void LogInDialog::setupUiConnections() const{

  QObject::connect(createUserWidget_, &CreateUserWidget::userCreated, this, [this](){
	// Vuelve a la vista de inicio de sesión tras crear el usuario con éxito.
	ui->btnOtherOptions->setChecked(false);
  });

  QObject::connect(ui->pbCancel, &QPushButton::clicked, this, &LogInDialog::reject_form);

  QObject::connect(ui->pbLogIn, &QPushButton::clicked, this, &LogInDialog::on_userLogin);

  ui->btnOtherOptions->setCheckable(true);
  QObject::connect(ui->btnOtherOptions, &QToolButton::toggled, this, &LogInDialog::on_handleToggleAnimation);

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

void LogInDialog::on_handleToggleAnimation(bool checked){

  if(checked){

	ui->btnOtherOptions->setIcon(QIcon(":/img/up.svg"));
	ui->btnOtherOptions->setToolTip("<span>Volver a Inicio de sesión!</span>");

	createUserWidget_->setMaximumHeight(QWIDGETSIZE_MAX);
	auto targetHeight = createUserWidget_->sizeHint().height();

	createUserWidget_->setMaximumHeight(0);

	collapseAnimation_->setStartValue(0);
	collapseAnimation_->setEndValue(targetHeight);

	collapseAnimation_->start();
	setStateControls(true);

	createUserWidget_->focusFirstField();

	isExpanded_ = true;

  }else{

	ui->btnOtherOptions->setIcon(QIcon(":/img/down.svg"));

	ui->btnOtherOptions->setToolTip("<span>"
									"Crear un nuevo usuario y/o<br>"
									"restablecer clave o password!"
									"</span>");

	collapseAnimation_->setStartValue(createUserWidget_->height());
	collapseAnimation_->setEndValue(0);

	collapseAnimation_->start();

	ui->groupBox->setEnabled(true);

	setStateControls(false);

	ui->txtUser->setFocus(Qt::OtherFocusReason);

	ui->pbLogIn->setDefault(true);

	isExpanded_ = false;
  }

  QTimer::singleShot(collapseAnimation_->duration(), this, &LogInDialog::adjustSize);
  applyIcons();

}

void LogInDialog::on_userLogin(){

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

}

void LogInDialog::setupAnimation(){

  collapseAnimation_ = new QPropertyAnimation(createUserWidget_, "maximumHeight", this);
  collapseAnimation_->setDuration(300);
  collapseAnimation_->setEasingCurve(QEasingCurve::InOutQuad);

  createUserWidget_->setMaximumHeight(0);
  createUserWidget_->setVisible(true);

}

void LogInDialog::closeEvent(QCloseEvent *event){
  writeSettings();
  createUserWidget_->saveControlStates();
  event->accept();
}