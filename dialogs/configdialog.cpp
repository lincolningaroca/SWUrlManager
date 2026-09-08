#include "configdialog.hpp"
#include "ui_configdialog.h"

#include <QCloseEvent>
#include <QMessageBox>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlError>

ConfigDialog::ConfigDialog(Qt::ColorScheme currentScheme, bool isFusionActive, QWidget *parent)
  : QDialog(parent), ui(new Ui::ConfigDialog),
  selectedScheme_(currentScheme),
  originalScheme_(currentScheme),
  selectedStyle_(isFusionActive),
  originalStyle_(isFusionActive)
{

  ui->setupUi(this);

  ui->txtPassword->setEchoMode(QLineEdit::Password);

  setWindowTitle(QStringLiteral("Configuración"));

  setWindowFlags(windowFlags() | Qt::MSWindowsFixedSizeDialogHint);

  initDialog();
  setCurrentTheme(selectedScheme_);

  ui->chkFusionStyle->setChecked(selectedStyle_);

  setDbConfig(SW::Helper_t::loadDbConfig());

  restoreLastSelection();

  // Navegación lateral
  QObject::connect(ui->listMenu, &QListWidget::currentRowChanged, this, &ConfigDialog::on_listMenu_currentRowChanged);

  // Botones de tema
  QObject::connect(ui->btnSystem, &QPushButton::clicked, this, &ConfigDialog::on_btnSystem_clicked);
  QObject::connect(ui->btnLight,  &QPushButton::clicked, this, &ConfigDialog::on_btnLight_clicked);
  QObject::connect(ui->btnDark,   &QPushButton::clicked, this, &ConfigDialog::on_btnDark_clicked);

  QObject::connect(ui->chkFusionStyle, &QCheckBox::toggled, this, [this](bool checked){
	selectedStyle_ = checked;
	// emit styleChanged(checked); // Si deseas previsualización en tiempo real
  });

  // Cuando MainForm aplica el tema (via Apply), refrescamos los botones del diálogo
  QObject::connect(this, &ConfigDialog::themeChanged, this, [this](Qt::ColorScheme scheme){
	setCurrentTheme(scheme);
  });

  // Botones de diálogo
  QObject::connect(ui->btnOk,     &QPushButton::clicked, this, &ConfigDialog::on_btnOk_clicked);
  QObject::connect(ui->btnApply,  &QPushButton::clicked, this, &ConfigDialog::on_btnApply_clicked);
  QObject::connect(ui->btnCancel, &QPushButton::clicked, this, &ConfigDialog::on_btnCancel_clicked);
}

ConfigDialog::~ConfigDialog()
{
  delete ui;
}

DbConfig ConfigDialog::getDbConfig() const noexcept {

  DbConfig cfg;
  cfg.host     = ui->txtHost->text().trimmed();
  cfg.port     = ui->txtPort->text().toInt();
  cfg.dbName   = ui->txtDbName->text().trimmed();
  cfg.userName = ui->txtUser->text().trimmed();
  cfg.password = ui->txtPassword->text();
  return cfg;
}

void ConfigDialog::setDbConfig(const DbConfig& config) noexcept {

  ui->txtHost->setText(config.host);
  ui->txtPort->setText(QString::number(config.port));
  ui->txtDbName->setText(config.dbName);
  ui->txtUser->setText(config.userName);
  ui->txtPassword->setText(config.password);
}

void ConfigDialog::setCurrentPage(int index) {

  if (index >= 0 && index < ui->listMenu->count()) {
	ui->listMenu->setCurrentRow(index);
  }
}

void ConfigDialog::on_btnTestDB_clicked() {

  const DbConfig cfg = getDbConfig();
  const QString tempConnName = QStringLiteral("TestDbConnection");

  // Si por alguna razón la conexión previa quedó en memoria, la removemos
  if (QSqlDatabase::contains(tempConnName)) {
	QSqlDatabase::removeDatabase(tempConnName);
  }

  bool success = false;
  QString errorMsg;

  // Bloque para aislar la instancia de QSqlDatabase y asegurar su destrucción antes del QMessageBox
  {
	QSqlDatabase testDb = QSqlDatabase::addDatabase(QStringLiteral("QPSQL"), tempConnName);
	testDb.setHostName(cfg.host);
	testDb.setPort(cfg.port);
	testDb.setDatabaseName(cfg.dbName);
	testDb.setUserName(cfg.userName);
	testDb.setPassword(cfg.password);

	success = testDb.open();
	if (!success) {
	  errorMsg = testDb.lastError().text();
	} else {
	  testDb.close();
	}
  } // 'testDb' se destruye completamente aquí

  // Limpiar el recurso de la lista de conexiones de Qt
  QSqlDatabase::removeDatabase(tempConnName);

  // Mostrar la notificación exactamente una vez
  if (success) {
	QMessageBox::information(this, windowTitle(), QStringLiteral("¡Conexión a la base de datos exitosa!"));
  } else {
	QMessageBox::critical(this, windowTitle(), QStringLiteral("Error al conectar a la base de datos:\n") + errorMsg);
  }
}

Qt::ColorScheme ConfigDialog::selectedScheme() const noexcept{

  return selectedScheme_;
}

void ConfigDialog::initDialog() noexcept{

  ui->listMenu->setIconSize(QSize(24, 24));
  ui->listMenu->setSpacing(2);

  auto *itemApariencia = new QListWidgetItem(QIcon(":/img/palette.png"), "  Apariencia");
  itemApariencia->setSizeHint(QSize(130, 40));
  ui->listMenu->addItem(itemApariencia);

  // Iconos de los botones de tema
  ui->btnSystem->setIcon(QIcon(":/img/system.png"));
  ui->btnSystem->setIconSize(QSize(32, 32));

  ui->btnLight->setIcon(QIcon(":/img/light.png"));
  ui->btnLight->setIconSize(QSize(32, 32));

  ui->btnDark->setIcon(QIcon(":/img/dark.png"));
  ui->btnDark->setIconSize(QSize(32, 32));

  auto *itemStyleApp = new QListWidgetItem(QIcon(":/img/style-fusion.png"), "Estilo de la aplicación");
  itemStyleApp->setSizeHint(QSize(130, 40));
  ui->listMenu->addItem(itemStyleApp);

  ui->lblImagen->setPixmap(QPixmap(":/img/style-fusion.png").scaled(
	256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation));
  ui->lblImagen->setAlignment(Qt::AlignCenter);

  auto *itemDbConexion = new QListWidgetItem(QIcon(":/img/dbConfig.png"), "Base de datos");
  itemDbConexion->setSizeHint(QSize(130, 40));
  ui->listMenu->addItem(itemDbConexion);
}


void ConfigDialog::setCurrentTheme(Qt::ColorScheme scheme) noexcept{

  const QString selectedStyle = R"(
	QPushButton {
	  border: 2px solid #4A90D9;
	  border-radius: 6px;
	  background-color: #2D5A8E;
	  color: white;
	  font-weight: bold;
	}
  )";

  const QString textColor = qApp->palette().color(QPalette::ButtonText).name();
  const QString hoverColor = qApp->palette().color(QPalette::Highlight).name();

  const QString normalStyle = QString(R"(
	QPushButton {
	  border: 1px solid #555;
	  border-radius: 6px;
	  background-color: transparent;
	  color: %1;
	}
	QPushButton:hover {
	  border: 1px solid %2;
	  background-color: %2;
	  color: white;
	}
  )").arg(textColor, hoverColor);

  ui->btnSystem->setStyleSheet(normalStyle);
  ui->btnLight->setStyleSheet(normalStyle);
  ui->btnDark->setStyleSheet(normalStyle);

  switch(scheme){
	case Qt::ColorScheme::Unknown:
	  ui->btnSystem->setStyleSheet(selectedStyle);
	  break;
	case Qt::ColorScheme::Light:
	  ui->btnLight->setStyleSheet(selectedStyle);
	  break;
	case Qt::ColorScheme::Dark:
	  ui->btnDark->setStyleSheet(selectedStyle);
	  break;
  }
}

void ConfigDialog::applyThemeSelection() noexcept{

  emit themeChanged(selectedScheme_);

}


void ConfigDialog::saveLastSelection(){

  QSettings settings(qApp->organizationName(), qApp->applicationName());

  auto *currentItem = ui->listMenu->currentItem();

  if(currentItem)
	settings.setValue("configDialogLastSelection", currentItem->text());

}

void ConfigDialog::restoreLastSelection(){

  QSettings settings(qApp->organizationName(), qApp->applicationName());
  auto lastText = settings.value("configDialogLastSelection", "").toString();
  if(!lastText.isEmpty()){

	QList<QListWidgetItem *> items = ui->listMenu->findItems(lastText, Qt::MatchExactly);

	if(!items.isEmpty()){

	  ui->listMenu->setCurrentItem(items.first());
	  ui->listMenu->scrollToItem(items.first());
	}
  }

}

// ── Slots de selección de tema ───────────────────────────────────────────────

void ConfigDialog::on_btnSystem_clicked(){

  selectedScheme_ = Qt::ColorScheme::Unknown;
  setCurrentTheme(selectedScheme_);

}

void ConfigDialog::on_btnLight_clicked(){

  selectedScheme_ = Qt::ColorScheme::Light;
  setCurrentTheme(selectedScheme_);

}

void ConfigDialog::on_btnDark_clicked(){

  selectedScheme_ = Qt::ColorScheme::Dark;
  setCurrentTheme(selectedScheme_);

}

// ── Slots de botones de diálogo ──────────────────────────────────────────────
void ConfigDialog::on_btnOk_clicked(){

  applyThemeSelection();
  emit styleChanged(selectedStyle_);
  SW::Helper_t::saveDbConfig(getDbConfig());
  saveLastSelection();
  accept();

}

void ConfigDialog::on_btnApply_clicked(){

  applyThemeSelection();
  emit styleChanged(selectedStyle_);
  SW::Helper_t::saveDbConfig(getDbConfig());
  // No cierra el diálogo

}

void ConfigDialog::on_btnCancel_clicked(){

  emit themeChanged(originalScheme_);
  emit styleChanged(originalStyle_);
  saveLastSelection();
  reject();

}


// ── Slots de navegación ──────────────────────────────────────────────────────
void ConfigDialog::on_listMenu_currentRowChanged(int row){

  if(row != -1)
	ui->stackedWidget->setCurrentIndex(row);

}

void ConfigDialog::closeEvent(QCloseEvent *event){

  saveLastSelection();
  emit themeChanged(originalScheme_);
  emit styleChanged(originalStyle_);
  event->accept();

}