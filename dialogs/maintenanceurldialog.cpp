#include "maintenanceurldialog.hpp"
#include "ui_maintenanceurldialog.h"

#include "swwidgets/midlewidget.hpp"

#include <QCloseEvent>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>

MaintenanceUrlDialog::MaintenanceUrlDialog(Qt::ColorScheme colorScheme,
  SW::OpenMode mode, const QList<QVariant> &dataUrl,
  uint32_t categoryId, QWidget *parent)
  : QDialog(parent),
  ui(new Ui::MaintenanceUrlDialog),
  currentCategoryId_(categoryId),
  mode_(mode),
  dataUrl_(dataUrl)
{
  ui->setupUi(this);

  setWindowFlags(windowFlags() | Qt::MSWindowsFixedSizeDialogHint);
  initForm();

  const auto iconColor = SW::Helper_t::currentIconColor(colorScheme);
  midleWidget->applyIcons(iconColor);
  midleWidget->setPlacesHolders();

  readSettings();

}

MaintenanceUrlDialog::~MaintenanceUrlDialog()
{
  delete ui;
}


void MaintenanceUrlDialog::closeEvent(QCloseEvent *event){

  writeSettings();
  event->accept();
}

void MaintenanceUrlDialog::writeSettings() const
{
  QSettings settings(qApp->organizationName(), qApp->applicationName());

  settings.beginGroup(QStringLiteral("Editor_p"));

  settings.setValue(QStringLiteral("fontFamily"), midleWidget->currentFont());
  settings.setValue(QStringLiteral("fontSize"), midleWidget->currentFontSize());
  settings.setValue(QStringLiteral("textColor"), midleWidget->textColor());
  settings.endGroup();
}

void MaintenanceUrlDialog::readSettings(){

  QSettings settings(qApp->organizationName(), qApp->applicationName());

  settings.beginGroup(QStringLiteral("Editor_p"));

  const auto fontFamily = settings.value(QStringLiteral("fontFamily"), "Arial").toString();
  const auto fontSize = settings.value(QStringLiteral("fontSize"), 10).toInt();
  const auto colorStr = settings.value(QStringLiteral("textColor"), "").toString();

  QColor textColor{};

  if (!colorStr.isEmpty() && QColor(colorStr).isValid()) {
	// Ya existe un valor guardado, usarlo
	textColor = QColor(colorStr);
  } else {
	// Primera vez: tomar el color de texto de la paleta activa
	// igual que hace ConfigDialog con QPalette::ButtonText
	textColor = qApp->palette().color(QPalette::Text);
  }
  settings.endGroup();

  midleWidget->restoreFont(fontFamily, fontSize, textColor);

}

void MaintenanceUrlDialog::initForm(){

  midleWidget = new MidleWidget(this);
  ui->insertLayout->addWidget(midleWidget);

  auto *buttonBox = new QDialogButtonBox(this);
  auto *okButton = buttonBox->addButton(tr("Guardar datos"), QDialogButtonBox::AcceptRole);
  auto *cancelButton = buttonBox->addButton(tr("&Cancelar"), QDialogButtonBox::RejectRole);
  cancelButton->setDefault(true);

  this->layout()->addWidget(buttonBox);

  if(mode_ == SW::OpenMode::New){

	setWindowTitle(tr("Agregar nueva url"));

  }else{

	setWindowTitle(tr("Editar datos url"));
	id = dataUrl_.value(0).toInt();

	midleWidget->setUrl(dataUrl_.value(1).toString());
	midleWidget->setDescription(dataUrl_.value(2).toString());

	okButton->setText(tr("Guardar cambios"));
  }

  connect(buttonBox, &QDialogButtonBox::accepted, this, &MaintenanceUrlDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &MaintenanceUrlDialog::reject);

}

void MaintenanceUrlDialog::accept(){

  if(mode_ == SW::OpenMode::New){
	if(!SW::Helper_t::urlValidate(midleWidget->url())){
	  QMessageBox::warning(this, SW::Helper_t::appName(), midleWidget->errorMessage());

	  midleWidget->selectAndFocus();
	  return;
	}

	if(helperdb_.urlExists(midleWidget->url(), currentCategoryId_)){

	  auto warningMsg = tr("<p>La url: <b>%1</b>, ya esta registrada!!</p>").arg(midleWidget->url());
	  QMessageBox::warning(this, SW::Helper_t::appName(), warningMsg);

	  midleWidget->selectAndFocus();
	  return;
	}

	if(helperdb_.saveData_url(midleWidget->url(), midleWidget->description(), currentCategoryId_)){

	  writeSettings();
	  QDialog::accept();

	}
  }else{

	if(!SW::Helper_t::urlValidate(midleWidget->url())){
	  QMessageBox::warning(this, SW::Helper_t::appName(), midleWidget->errorMessage());

	  midleWidget->selectAndFocus();
	  return;
	}

	if(!helperdb_.updateData_url(midleWidget->url(), midleWidget->description(), id, currentCategoryId_)){
	  QMessageBox::critical(this, SW::Helper_t::appName(), tr("Fallo la ejecución de la sentencia!"));
	  return;

	}
	writeSettings();
	QDialog::accept();

  }
}