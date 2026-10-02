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
  setWindowFlags(windowFlags() | Qt::MSWindowsFixedSizeDialogHint);

  ui->txtPassword->setEchoMode(QLineEdit::Password);
  ui->chkFusionStyle->setChecked(selectedStyle_);

  initDialog();
  setupLanguageCombo();
  setCurrentTheme(selectedScheme_);
  setDbConfig(SW::Helper_t::loadDbConfig());
  restoreLastSelection();

  setupUiConnections();
}

ConfigDialog::~ConfigDialog()
{
  delete ui;
}

SW::DbConfig ConfigDialog::getDbConfig() const noexcept {

  SW::DbConfig cfg;
  cfg.host     = ui->txtHost->text().trimmed();
  cfg.port     = ui->txtPort->text().toInt();
  cfg.dbName   = ui->txtDbName->text().trimmed();
  cfg.userName = ui->txtUser->text().trimmed();
  cfg.password = ui->txtPassword->text();
  return cfg;
}

void ConfigDialog::setDbConfig(const SW::DbConfig &config) noexcept {

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

  const SW::DbConfig cfg = getDbConfig();
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
	QMessageBox::information(this, windowTitle(), tr("¡Conexión a la base de datos exitosa!"));
  } else {
	QMessageBox::critical(this, windowTitle(), tr("Error al conectar a la base de datos:\n") + errorMsg);
  }
}

Qt::ColorScheme ConfigDialog::selectedScheme() const noexcept{

  return selectedScheme_;
}

void ConfigDialog::initDialog() noexcept{

  ui->listMenu->setIconSize(QSize(24, 24));
  ui->listMenu->setSpacing(2);

  applyAllStyles();

  auto *itemApariencia = new QListWidgetItem(QIcon(":/img/palette.png"), tr("Apariencia"));
  itemApariencia->setSizeHint(QSize(130, 40));
  itemApariencia->setData(Qt::UserRole, static_cast<int>(ConfigSection::Appearance));
  ui->listMenu->addItem(itemApariencia);

  // Iconos de los botones de tema
  ui->btnSystem->setIcon(QIcon(":/img/system.png"));
  ui->btnSystem->setIconSize(QSize(32, 32));

  ui->btnLight->setIcon(QIcon(":/img/light.png"));
  ui->btnLight->setIconSize(QSize(32, 32));

  ui->btnDark->setIcon(QIcon(":/img/dark.png"));
  ui->btnDark->setIconSize(QSize(32, 32));

  auto *itemStyleApp = new QListWidgetItem(QIcon(":/img/style-fusion.png"), tr("Estilo de la aplicación"));
  itemStyleApp->setSizeHint(QSize(130, 40));
  itemStyleApp->setData(Qt::UserRole, static_cast<int>(ConfigSection::AppStyle));
  ui->listMenu->addItem(itemStyleApp);

  ui->lblImagen->setPixmap(QPixmap(":/img/style-fusion.png").scaled(
	256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation));
  ui->lblImagen->setAlignment(Qt::AlignCenter);

  auto *itemDbConexion = new QListWidgetItem(QIcon(":/img/dbConfig.png"), tr("Base de datos"));
  itemDbConexion->setSizeHint(QSize(130, 40));
  itemDbConexion->setData(Qt::UserRole, static_cast<int>(ConfigSection::DataBase));
  ui->listMenu->addItem(itemDbConexion);

  auto *itemGenerales = new QListWidgetItem(QIcon(":/img/generalOption.png"), tr("Generales"));
  itemGenerales->setSizeHint(QSize(130, 40));
  itemGenerales->setData(Qt::UserRole, static_cast<int>(ConfigSection::General));
  ui->listMenu->addItem(itemGenerales);

  //llenar el combobox con las opciones de idioma
  ui->languageLabel->setText(tr("Idioma"));

}

void ConfigDialog::setupLanguageCombo() noexcept {
  ui->languageComboBox->clear();
  ui->languageComboBox->addItem(tr("Español"), QStringLiteral("es"));
  ui->languageComboBox->addItem(tr("English"), QStringLiteral("en"));

  ui->languageLabel->setText(tr("Idioma"));

  // Leer idioma de la configuración guardada
  // QSettings settings(qApp->organizationName(), SW::Helper_t::appName());
  // currentLang_ = settings.value(QStringLiteral("language"), QStringLiteral("es")).toString();
  currentLang_ = SW::Helper_t::currentLanguage();

  // Seleccionar el ítem correspondiente en el ComboBox
  int index = ui->languageComboBox->findData(currentLang_);
  if (index != -1) {
	ui->languageComboBox->setCurrentIndex(index);
  } else {
	ui->languageComboBox->setCurrentIndex(0); // Por defecto Español
  }
}

void ConfigDialog::applyAllStyles() noexcept {
  // Colores dinámicos de la paleta del sistema
  const QColor accentColor   = qApp->palette().color(QPalette::Accent);    // hover — acento genérico, no selección
  const QColor selectedColor = qApp->palette().color(QPalette::Highlight); // ítem/botón realmente seleccionado
  const QColor textColor     = qApp->palette().color(QPalette::ButtonText);
  const QColor borderColor   = qApp->palette().color(QPalette::Mid);

  // Color tenue para hover (Alpha = 100 de 255)
  QColor hoverColor = accentColor;
  hoverColor.setAlpha(90);

  // Estilo para botones en estado NORMAL y HOVER
  const QString btnNormalStyle = QString(R"(
		QPushButton {
			border: 1px solid %1;
			border-radius: 6px;
			background-color: transparent;
			color: %2;
			padding: 8px;
		}
		QPushButton:hover {
			border: 1px solid %3;
			background-color: %3;
			color: white;
		}
	)").arg(borderColor.name(QColor::HexArgb),
										textColor.name(QColor::HexArgb),
										hoverColor.name(QColor::HexArgb));

  // Estilo para botón SELECCIONADO (usa color sólido con alpha completo)
  const QString btnSelectedStyle = QString(R"(
		QPushButton {
			border: 2px solid %1;
			border-radius: 6px;
			background-color: %1;
			color: white;
			font-weight: bold;
			padding: 8px;
		}
	)").arg(selectedColor.name(QColor::HexArgb));

  // Estilo para el menú lateral (QListWidget)
  const QString menuStyle = QString(R"(
		QListWidget {
			border: none;
			background-color: transparent;
			outline: none;
		}
		QListWidget::item {
			border: none;
			padding: 10px 8px;
			border-radius: 6px;
			margin: 2px 4px;
			color: %1;
		}
		QListWidget::item:hover {
			background-color: %2;
		}
		QListWidget::item:selected {
			background-color: %3;
			color: white;
			font-weight: bold;
		}
	)").arg(textColor.name(QColor::HexArgb),
								   hoverColor.name(QColor::HexArgb),
								   selectedColor.name(QColor::HexArgb));

  // Aplicar estilos
  ui->btnSystem->setStyleSheet(btnNormalStyle);
  ui->btnLight->setStyleSheet(btnNormalStyle);
  ui->btnDark->setStyleSheet(btnNormalStyle);
  ui->listMenu->setStyleSheet(menuStyle);

  // Aplicar estilo seleccionado al botón activo
  switch(selectedScheme_){
	case Qt::ColorScheme::Unknown:
	  ui->btnSystem->setStyleSheet(btnSelectedStyle);
	  break;
	case Qt::ColorScheme::Light:
	  ui->btnLight->setStyleSheet(btnSelectedStyle);
	  break;
	case Qt::ColorScheme::Dark:
	  ui->btnDark->setStyleSheet(btnSelectedStyle);
	  break;
  }
}

void ConfigDialog::setupUiConnections(){

  // Navegación lateral
  QObject::connect(ui->listMenu, &QListWidget::currentRowChanged, this, &ConfigDialog::on_listMenu_currentRowChanged);

  // Botones de tema
  QObject::connect(ui->btnSystem, &QPushButton::clicked, this, &ConfigDialog::on_btnSystem_clicked);
  QObject::connect(ui->btnLight,  &QPushButton::clicked, this, &ConfigDialog::on_btnLight_clicked);
  QObject::connect(ui->btnDark,   &QPushButton::clicked, this, &ConfigDialog::on_btnDark_clicked);

  QObject::connect(ui->chkFusionStyle, &QCheckBox::toggled, this, [this](bool checked){
	selectedStyle_ = checked;

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


void ConfigDialog::setCurrentTheme(Qt::ColorScheme scheme) noexcept {

  selectedScheme_ = scheme;
  applyAllStyles();

}

void ConfigDialog::applyThemeSelection() noexcept{

  emit themeChanged(selectedScheme_);

}

void ConfigDialog::applyLanguageSelection() {

  // QSettings settings(qApp->organizationName(), SW::Helper_t::appName());
  const QString selectedLanguage = ui->languageComboBox->currentData().toString();
  // settings.setValue(QStringLiteral("language"), selectedLanguage);
  SW::Helper_t::setLanguage(selectedLanguage);
}


void ConfigDialog::saveLastSelection(){

  QSettings settings(qApp->organizationName(), qApp->applicationName());

  auto *currentItem = ui->listMenu->currentItem();

  if(currentItem){
	int sectionId = currentItem->data(Qt::UserRole).toInt();
	settings.setValue("configDialogLastSelection", sectionId);
  }
}

void ConfigDialog::restoreLastSelection(){

  QSettings settings(qApp->organizationName(), qApp->applicationName());

  // Si no existe valor guardado, seleccionamos General por defecto (0)
  const auto defaultSection = static_cast<int>(ConfigSection::General);
  const auto savedSection = settings.value(QStringLiteral("configDialogLastSelection"), defaultSection).toInt();

  // Buscar en la lista el ítem que coincida con el valor del enum en Qt::UserRole
  for (int i = 0; i < ui->listMenu->count(); ++i) {
	QListWidgetItem *item = ui->listMenu->item(i);
	if (item && item->data(Qt::UserRole).toInt() == savedSection) {
	  ui->listMenu->setCurrentItem(item);
	  ui->listMenu->scrollToItem(item);
	  break;
	}
  }

}

// ── Slots de selección de tema ───────────────────────────────────────────────

void ConfigDialog::on_btnSystem_clicked(){

  setCurrentTheme(Qt::ColorScheme::Unknown);

}

void ConfigDialog::on_btnLight_clicked(){

  setCurrentTheme(Qt::ColorScheme::Light);

}

void ConfigDialog::on_btnDark_clicked(){

  setCurrentTheme(Qt::ColorScheme::Dark);

}

// ── Slots de botones de diálogo ──────────────────────────────────────────────
void ConfigDialog::on_btnOk_clicked(){

  applyThemeSelection();
  emit styleChanged(selectedStyle_);

  applyLanguageSelection();
  SW::Helper_t::saveDbConfig(getDbConfig());
  saveLastSelection();
  accept();

}

void ConfigDialog::on_btnApply_clicked(){

  applyThemeSelection();
  emit styleChanged(selectedStyle_);

  applyLanguageSelection();
  SW::Helper_t::saveDbConfig(getDbConfig());
  // No cierra el diálogo

}

void ConfigDialog::on_btnCancel_clicked(){

  emit themeChanged(originalScheme_);
  emit styleChanged(originalStyle_);

  // Restaurar el combo box al idioma que estaba guardado en QSettings
  int index = ui->languageComboBox->findData(currentLang_);
  if (index != -1) {
	ui->languageComboBox->setCurrentIndex(index);
  }

  SW::Helper_t::setLanguage(currentLang_);

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

void ConfigDialog::changeEvent(QEvent *event){
  // ===== NUEVO: Re-traducir la interfaz cuando cambia el idioma =====
  if (event->type() == QEvent::LanguageChange) {
	ui->retranslateUi(this);
  }

  if (event->type() == QEvent::PaletteChange ||
	  event->type() == QEvent::ApplicationPaletteChange) {
	// Actualizar todos los estilos cuando cambie el color de énfasis del sistema
	applyAllStyles();
  }
  QDialog::changeEvent(event);
}