#include "dialogs/configdialog.hpp"
#include "helperdatabase/helperdb.hpp"
#include "mainform.hpp"
#include "util/cryptomanager.hpp"
#include "util/helper.hpp"

#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFontDatabase>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlError>
#include <QUrl>
#include <QVBoxLayout>

constexpr int MIN_POSTGRESQL_VERSION = 17;
const QString PG_DOWNLOAD_URL = QStringLiteral("https://www.postgresql.org/download/");

bool unlockOrSetupEncryption(QSqlDatabase& db, QByteArray& outDek) {

  if (auto cached = SW::CryptoManager::loadCachedLocalDEK()) {
	outDek = *cached;
	return true;
  }

  const bool alreadyConfigured = SW::CryptoManager::hasSecuritySettings(db);

  if (alreadyConfigured) {
	// Solo desbloqueo — una sola vez, sin confirmación (como cualquier login).
	bool ok = false;
	const QString masterPwd = QInputDialog::getText(
	  nullptr, qApp->applicationName(),
	  QStringLiteral("Ingrese la contraseña maestra para habilitar esta instalación."),
	  QLineEdit::Password, QString(), &ok);

	if (!ok || masterPwd.isEmpty()) return false;

	auto dek = SW::CryptoManager::loadDEK(masterPwd, db);
	if (!dek) {
	  QMessageBox::critical(nullptr, qApp->applicationName(), "Contraseña incorrecta.");
	  return false;
	}
	outDek = *dek;

  } else {
	// Primera vez: se define la contraseña maestra — se pide dos veces para
	// evitar que un typo del administrador quede grabado sin forma de detectarlo.
	QString masterPwd;
	QString confirmPwd;
	bool ok = false;

	while (true) {
	  masterPwd = QInputDialog::getText(
		nullptr, qApp->applicationName(),
		QStringLiteral("Defina una contraseña maestra para proteger los datos.\n"
					   "Guárdela en un lugar seguro: sin ella no hay forma de recuperar la información."),
		QLineEdit::Password, QString(), &ok);

	  if (!ok || masterPwd.isEmpty()) return false;

	  if (!SW::Helper_t::isPasswordSecure(masterPwd)) {
		QMessageBox::warning(nullptr, qApp->applicationName(),
							 QStringLiteral("La contraseña debe tener al menos 8 caracteres, una mayúscula, "
											"un número y un carácter especial."));
		continue;
	  }

	  confirmPwd = QInputDialog::getText(
		nullptr, qApp->applicationName(),
		QStringLiteral("Confirme la contraseña maestra."),
		QLineEdit::Password, QString(), &ok);

	  if (!ok) return false;

	  if (masterPwd != confirmPwd) {
		QMessageBox::warning(nullptr, qApp->applicationName(),
							 QStringLiteral("Las contraseñas no coinciden. Intente nuevamente."));
		continue;
	  }

	  break;
	}

	auto dek = SW::CryptoManager::generateDEK();
	if (!dek || !SW::CryptoManager::storeDEK(*dek, masterPwd, db)) {
	  QMessageBox::critical(nullptr, qApp->applicationName(), "No se pudo inicializar el cifrado.");
	  return false;
	}
	outDek = *dek;
  }

  SW::CryptoManager::cacheLocalDEK(outDek);
  return true;
}

/**
 * @brief Comprueba la existencia y versión mínima del motor PostgreSQL en el cliente.
 * @return true si la verificación es exitosa; false si debe cerrarse el programa.
 */
bool verifyPostgreSQLRequirement() {

  auto pgStatus = SW::Helper_t::checkPostgresqlInstallation();

  if (!pgStatus.isInstalled) {
	QMessageBox msgBox;
	msgBox.setIcon(QMessageBox::Critical);
	msgBox.setWindowTitle(qApp->applicationName());
	msgBox.setText(QStringLiteral("<b>PostgreSQL no está instalado en el sistema.</b>"));
	msgBox.setInformativeText(
	  QStringLiteral("Esta aplicación requiere el motor de base de datos PostgreSQL (versión %1 o superior) para funcionar.\n\n"
					 "¿Desea abrir el sitio oficial de PostgreSQL para descargarlo?")
		.arg(MIN_POSTGRESQL_VERSION));
	msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
	msgBox.setDefaultButton(QMessageBox::Yes);

	if (msgBox.exec() == QMessageBox::Yes) {
	  if(!SW::Helper_t::open_Url(QUrl(PG_DOWNLOAD_URL))){
		qWarning() << "No se pudo abrir el navegador web predeterminado.";
	  }
	}
	return false;
  }

  if (pgStatus.majorVersion > 0 && pgStatus.majorVersion < MIN_POSTGRESQL_VERSION) {
	QMessageBox msgBox;
	msgBox.setIcon(QMessageBox::Warning);
	msgBox.setWindowTitle(qApp->applicationName());
	msgBox.setText(QStringLiteral("<b>Versión de PostgreSQL incompatible.</b>"));
	msgBox.setInformativeText(
	  QStringLiteral("Se detectó PostgreSQL versión %1 en su equipo.\n"
					 "Esta aplicación requiere como mínimo la versión %2.\n\n"
					 "Por favor, actualice su instalación de PostgreSQL desde el sitio oficial.")
		.arg(pgStatus.majorVersion)
		.arg(MIN_POSTGRESQL_VERSION));
	msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
	msgBox.setDefaultButton(QMessageBox::Yes);

	if (msgBox.exec() == QMessageBox::Yes) {
	  if(!SW::Helper_t::open_Url(QUrl(PG_DOWNLOAD_URL))){
		qWarning() << "No se pudo abrir el navegador web predeterminado.";
	  }
	}
	return false;
  }

  return true;
}

/**
 * @brief The SingleIntsanceManager class
 * Esta estructura determina si hay una instancia de la apliacion abierta;
 * si es asi ya no se vuelve a lanzar otra app.
 */
struct SingleIntsanceManager{

  static bool isRunning(const QString& serverName){

	QLocalSocket socket{};
	socket.connectToServer(serverName);

	if(socket.waitForConnected(500)){
	  socket.disconnectFromServer();
	  return true;
	}
	return false;
  }

  static bool initServer(const QString& serverName, QObject* parent = nullptr){

	auto* server = new QLocalServer(parent);
	QLocalServer::removeServer(serverName);

	if(!server->listen(serverName)){
	  delete server;
	  return false;
	}
	return true;
  }
};

/**
 * @brief connectToDatabase
 * Establece la conexión principal de la app usando la configuración guardada
 */
bool connectToDatabase(const SW::DbConfig& config) {
  const QString connectionName = QStringLiteral("xxxConection");

  if (QSqlDatabase::contains(connectionName)) {
	QSqlDatabase::removeDatabase(connectionName);
  }

  QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QPSQL"), connectionName);
  db.setHostName(config.host);
  db.setPort(config.port);
  db.setDatabaseName(config.dbName);
  db.setUserName(config.userName);
  db.setPassword(config.password);

  if (!db.open()) {
	qCritical() << "Error abriendo conexión principal:" << db.lastError().text();
	return false;
  }

  return true;
}

/**
 * @brief publicUserExists
 * Verifica si el usuario 'public' existe en la tabla users
 * @return true si el usuario existe
 */
bool publicUserExists(){
  QSqlDatabase db = QSqlDatabase::database(QStringLiteral("xxxConection"));
  if(!db.isOpen()) return false;

  QSqlQuery qry(db);
  qry.prepare(R"(SELECT fn_user_exists(?))");
  qry.addBindValue(QStringLiteral("public"));

  if(qry.exec() && qry.next()){
	return qry.value(0).toBool();
  }

  return false;
}

/**
 * @brief initializeDefaultData
 * Carga y ejecuta el script SQL de inicialización desde los recursos de la aplicación
 * @return true si la ejecución fue exitosa o si los datos ya existían
 */
bool initializeDefaultData(){
  // Verificar si el usuario 'public' ya existe
  if(publicUserExists()){
	qInfo() << "El usuario 'public' ya existe. No se requiere inicialización.";
	return true;
  }

  // Cargar el script SQL desde los recursos
  QFile file(":/database/seed.sql");
  if(!file.open(QIODevice::ReadOnly | QIODevice::Text)){
	QMessageBox::critical(nullptr, qApp->applicationName(),
						  "Error al cargar el script de inicialización:\n" + file.errorString());
	return false;
  }

  QTextStream in(&file);
  QString sqlScript = in.readAll();
  file.close();

  // Ejecutar el script
  QSqlDatabase db = QSqlDatabase::database(QStringLiteral("xxxConection"));
  QSqlQuery qry(db);

  if(!qry.exec(sqlScript)){
	QMessageBox::critical(nullptr, qApp->applicationName(),
						  "Error al ejecutar el script de inicialización:\n" + qry.lastError().text());
	return false;
  }

  qInfo() << "Usuario 'public' inicializado correctamente!";
  return true;
}

void cacheBootstrapPassword(const QString& password) {
  const QByteArray protectedBlob = SW::Helper_t::protectLocal(password.toUtf8());
  if (protectedBlob.isEmpty()) return;
  QSettings settings(qApp->organizationName(), qApp->applicationName());
  settings.setValue(QStringLiteral("crypto/adminBootstrapPwd"), protectedBlob.toBase64());
}

QString loadCachedBootstrapPassword() {
  QSettings settings(qApp->organizationName(), qApp->applicationName());
  const auto b64 = settings.value(QStringLiteral("crypto/adminBootstrapPwd")).toByteArray();
  if (b64.isEmpty()) return QString();
  return QString::fromUtf8(SW::Helper_t::unprotectLocal(QByteArray::fromBase64(b64)));
}

void clearCachedBootstrapPassword() {
  QSettings settings(qApp->organizationName(), qApp->applicationName());
  settings.remove(QStringLiteral("crypto/adminBootstrapPwd"));
}

void showBootstrapPasswordDialog(const QString& password) {

  QDialog pwdDialog;
  pwdDialog.setWindowTitle(qApp->applicationName());
  pwdDialog.setWindowFlags(pwdDialog.windowFlags() | Qt::MSWindowsFixedSizeDialogHint);
  pwdDialog.setMinimumWidth(420);

  auto* layout = new QVBoxLayout(&pwdDialog);

  auto* infoLabel = new QLabel(
	QStringLiteral("<b>La cuenta de administrador todavía usa la contraseña temporal.</b><br><br>"
				   "Usuario: <code>admin</code><br><br>"
				   "Contraseña temporal (cópiela e inicie sesión para cambiarla):"),
	&pwdDialog);
  infoLabel->setTextFormat(Qt::RichText);
  infoLabel->setWordWrap(true);

  auto* pwdEdit = new QLineEdit(password, &pwdDialog);
  pwdEdit->setReadOnly(true);
  pwdEdit->selectAll();

  auto* copyButton = new QPushButton(QStringLiteral("Copiar contraseña"), &pwdDialog);
  copyButton->setDefault(true);

  auto* warningLabel = new QLabel(
	QStringLiteral("<i>Este mensaje seguirá apareciendo al abrir la app hasta que inicie sesión "
				   "como admin y establezca una contraseña definitiva.</i>"), &pwdDialog);
  warningLabel->setWordWrap(true);

  auto* closeButton = new QPushButton(QStringLiteral("Entendido, cerrar"), &pwdDialog);

  layout->addWidget(infoLabel);
  layout->addWidget(pwdEdit);
  layout->addWidget(copyButton);
  layout->addWidget(warningLabel);
  layout->addWidget(closeButton);

  QObject::connect(copyButton, &QPushButton::clicked, &pwdDialog, [pwdEdit, copyButton](){
	QApplication::clipboard()->setText(pwdEdit->text());
	copyButton->setText(QStringLiteral("¡Copiada!"));
  });

  QObject::connect(closeButton, &QPushButton::clicked, &pwdDialog, &QDialog::accept);

  pwdEdit->setFocus(Qt::OtherFocusReason);
  pwdDialog.exec();
}

bool bootstrapAdminIfNeeded(){

  QSqlDatabase db = QSqlDatabase::database(QStringLiteral("xxxConection"));

  QSqlQuery permQry(db);
  permQry.prepare(R"(SELECT must_change_password FROM fn_get_user_permissions('admin'))");

  const bool adminExists = permQry.exec() && permQry.next();

  if (!adminExists) {
	// Primera vez: crear el admin con contraseña generada
	const auto generatedPassword = SW::Helper_t::generateSecurePassword(12);
	if (generatedPassword.isEmpty()) return false;

	QSqlQuery bootstrapQry(db);
	bootstrapQry.prepare(R"(SELECT fn_bootstrap_admin(?))");
	bootstrapQry.addBindValue(generatedPassword);

	if (!bootstrapQry.exec() || !bootstrapQry.next() || !bootstrapQry.value(0).toBool()) {
	  qCritical() << "No se pudo crear el usuario administrador inicial.";
	  return false;
	}

	cacheBootstrapPassword(generatedPassword);
	showBootstrapPasswordDialog(generatedPassword);
	return true;
  }

  const bool mustChange = permQry.value(0).toBool();

  if (!mustChange) {
	// Ya se cambió — asegurarse de que no quede nada cacheado de antes.
	clearCachedBootstrapPassword();
	return true;
  }

  // Todavía no se cambió — mostrar de nuevo si tenemos la contraseña cacheada localmente.
  const auto cachedPassword = loadCachedBootstrapPassword();
  if (!cachedPassword.isEmpty()) {
	showBootstrapPasswordDialog(cachedPassword);
  }

  return true;
}


int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
  a.setApplicationName(QStringLiteral("SWUrlManager"));
  a.setApplicationVersion(QStringLiteral("1.0"));
  a.setOrganizationName(QStringLiteral("SWSystem's"));


  // Instalar el interceptor de Logs de Qt
  qInstallMessageHandler(SW::customLogHandler);

  qInfo() << "============================================";
  qInfo() << "Iniciando aplicación:" << a.applicationName() << a.applicationVersion();

  const QString serverName{a.applicationName()};
  if(SingleIntsanceManager::isRunning(serverName)){
	return -1;
  }

  if(!SingleIntsanceManager::initServer(serverName, &a)){
	QMessageBox::critical(nullptr, qApp->applicationName(), "No se pudo iniciar el control de instancia única.");
	return -1;
  }


  // 1. Conectar a la base de datos PostgreSQL
  if(!verifyPostgreSQLRequirement()){
	return -1;
  }

  auto config = SW::Helper_t::loadDbConfig();

  qDebug() << "config.host:"     << config.host;
  qDebug() << "config.port:"     << config.port;
  qDebug() << "config.dbName:"   << config.dbName;
  qDebug() << "config.userName:" << config.userName;
  qDebug() << "config.password está vacío:" << config.password.isEmpty();
  qDebug() << "hasDbConfig:"     << SW::Helper_t::hasDbConfig();

  if (!SW::HelperDataBase_t::ensureDatabaseAndSchemaReady(config)) {
	QMessageBox::critical(nullptr, SW::Helper_t::appName(),
						  QStringLiteral("No se pudo preparar la base de datos para la aplicación. El programa se cerrará."));
	return -1;
  }

  // 2. Conectar a la base de datos PostgreSQL
  qInfo() << "Conectando a PostgreSQL...";
  if(!connectToDatabase(config)){
	return -1;
  }

  // 3. Verificar e inicializar datos por defecto (usuario 'public')
  qInfo() << "Verificando usuario 'public'...";
  if(!initializeDefaultData()){
	return -1;
  }

  if(!bootstrapAdminIfNeeded()){
	QMessageBox::critical(nullptr, qApp->applicationName(),
						  QStringLiteral("No se pudo crear la cuenta de administrador inicial."));
	return -1;
  }

  QByteArray dek;
  QSqlDatabase mainDb = QSqlDatabase::database(QStringLiteral("xxxConection"));
  if (!unlockOrSetupEncryption(mainDb, dek)) {
	return -1;
  }

  SW::Helper_t::sessionEncryptionKey_ = dek;

  //Creacion de la carpeta de la aplicación
  QDir dir(SW::Helper_t::AppLocalDataLocation());
  if(!dir.exists()){
	if(SW::Helper_t::createDataBase_dir())
	  qInfo() << "Carpeta del sistema creado!";
  }


  MainForm w;
  w.setWindowTitle(a.applicationName());

  w.show();

  int result = a.exec();
  qInfo() << "Event loop terminado con código:" << result;

  return result;
}
