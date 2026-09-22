#include "createnewuserdialog.hpp"
#include "ui_createnewuserdialog.h"

#include "swwidgets/createuserwidget.hpp"

#include <QCloseEvent>
#include <QSettings>

CreateNewUserDialog::CreateNewUserDialog(QWidget *parent)
  : QDialog(parent)
  , ui(new Ui::CreateNewUserDialog)
{
  ui->setupUi(this);
  setWindowTitle(SW::Helper_t::appName().append(" - crear nuevo usuario"));
  setWindowFlags(windowFlags() | Qt::MSWindowsFixedSizeDialogHint);

  createUserWidget_ = new CreateUserWidget(this);
  ui->createUserContainerLayout->addWidget(createUserWidget_);

  QObject::connect(createUserWidget_, &CreateUserWidget::userCreated, this, &CreateNewUserDialog::accept);

  readSettings();
}

CreateNewUserDialog::~CreateNewUserDialog()
{
  delete ui;
}

void CreateNewUserDialog::writeSettings() const noexcept{

  QSettings settings(qApp->organizationName(), SW::Helper_t::appName());

  settings.setValue(QStringLiteral("posNewUserForm"), saveGeometry());

}

void CreateNewUserDialog::readSettings(){

  QSettings settings(qApp->organizationName(), SW::Helper_t::appName());

  restoreGeometry(settings.value(QStringLiteral("posNewUserForm"), QByteArray()).toByteArray());

}

void CreateNewUserDialog::closeEvent(QCloseEvent *event){

  writeSettings();
  createUserWidget_->saveControlStates();
  event->accept();
}