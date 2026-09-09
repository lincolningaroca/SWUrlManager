#include "helperdb.hpp"

#include "util/helper.hpp"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QInputDialog>
#include <QLineEdit>
#include <QLoggingCategory>
#include <QProcess>
#include <QProcessEnvironment>
#include <QSettings>
#include <QSqlError>
#include <QStandardPaths>
#include <QStringView>
#include <QVersionNumber>
#include <algorithm>


namespace SW {

Q_LOGGING_CATEGORY(lcPgToolFinder, "app.database.pgtoolfinder")
static QString s_cachedPgBinDir;

HelperDataBase_t::HelperDataBase_t()
  :db_{QSqlDatabase::database(QStringLiteral("xxxConection"))},
  qry_(db_),
  encryptionKey_{QString::fromLatin1(SW::Helper_t::sessionEncryptionKey_.toHex())}
{
}

// Constructor para conexiones dedicadas a un hilo (ej. UrlImportWorker).
// Qt SQL no permite compartir una QSqlDatabase entre hilos distintos.
HelperDataBase_t::HelperDataBase_t(QSqlDatabase db) noexcept
  : db_{std::move(db)},
  qry_(db_)
{
}

bool HelperDataBase_t::importUrlsBatch(uint32_t categoryId,
									   const QList<UrlImportData>& items,
									   DuplicateAction action,
									   int* insertedCount,
									   int* updatedCount,
									   const std::function<void(int done, int total)>& onProgress) noexcept {
  if (items.isEmpty()) return true;

  // Ahora db_.transaction() funcionará correctamente
  if (!db_.transaction()) {
	errorMessage_ = db_.lastError().text();
	return false;
  }

  QList<UrlImportData> toInsert;
  QList<UrlImportData> toUpdate;
  toInsert.reserve(items.size());
  toUpdate.reserve(items.size());

  QSet<QString> processedInFile;

  for (const auto& item : items) {
	QString cleanUrl = item.url.trimmed();
	if (cleanUrl.isEmpty() || processedInFile.contains(cleanUrl)) {
	  continue;
	}
	processedInFile.insert(cleanUrl);

	if (urlExists(cleanUrl, categoryId)) {
	  if (action == DuplicateAction::Replace) {
		toUpdate.emplaceBack(item);
	  }
	} else {
	  toInsert.emplaceBack(item);
	}
  }

  constexpr int kChunkSize = 200; // filas por execBatch — balance entre round-trips y progreso visible
  const int totalToProcess = static_cast<int>(toInsert.size() + toUpdate.size());
  int processedSoFar = 0;
  // 1. Inserción masiva mediante fn_save_url, en lotes
  for (int start = 0; start < toInsert.size(); start += kChunkSize) {
	const int end = std::min(start + kChunkSize, static_cast<int>(toInsert.size()));
	QVariantList urls, descs, catIds, keys;
	for (int i = start; i < end; ++i) {
	  const auto& item = toInsert[i];
	  urls << item.url.trimmed();
	  descs << item.description.trimmed();
	  catIds << categoryId;
	  keys << encryptionKey_;
	}
	qry_.prepare(QStringLiteral("SELECT public.fn_save_url(?, ?, ?, ?)"));
	qry_.addBindValue(urls);
	qry_.addBindValue(descs);
	qry_.addBindValue(catIds);
	qry_.addBindValue(keys);
	if (!qry_.execBatch()) {
	  errorMessage_ = qry_.lastError().text();
	  db_.rollback();
	  return false;
	}
	processedSoFar += (end - start);
	if (onProgress) onProgress(processedSoFar, totalToProcess);
  }
  // 2. Actualización masiva mediante fn_update_url_by_text, en lotes
  for (int start = 0; start < toUpdate.size(); start += kChunkSize) {
	const int end = std::min(start + kChunkSize, static_cast<int>(toUpdate.size()));
	QVariantList urls, descs, catIds, keys;
	for (int i = start; i < end; ++i) {
	  const auto& item = toUpdate[i];
	  urls << item.url.trimmed();
	  descs << item.description.trimmed();
	  catIds << categoryId;
	  keys << encryptionKey_;
	}
	qry_.prepare(QStringLiteral("SELECT public.fn_update_url_by_text(?, ?, ?, ?)"));
	qry_.addBindValue(urls);
	qry_.addBindValue(descs);
	qry_.addBindValue(catIds);
	qry_.addBindValue(keys);
	if (!qry_.execBatch()) {
	  errorMessage_ = qry_.lastError().text();
	  db_.rollback();
	  return false;
	}
	processedSoFar += (end - start);
	if (onProgress) onProgress(processedSoFar, totalToProcess);
  }

  db_.commit();

  if (insertedCount) *insertedCount = static_cast<int>(toInsert.size());
  if (updatedCount) *updatedCount = static_cast<int>(toUpdate.size());

  return true;
}

QString HelperDataBase_t::getPostgresToolPath(const QString &toolName, bool *found){
  if (found)
	*found = false;

  QString exeName = toolName;
#ifdef Q_OS_WIN
  if (!exeName.endsWith(".exe", Qt::CaseInsensitive))
	exeName += ".exe";
#endif

  // -------------------------------------------------------------------------
  // 0. CACHÉ: si ya localizamos el bin/ de PostgreSQL antes, probarlo primero
  // -------------------------------------------------------------------------
  if (!s_cachedPgBinDir.isEmpty()) {
	QString cachedPath = QDir(s_cachedPgBinDir).filePath(exeName);
	if (QFile::exists(cachedPath)) {
	  qCDebug(lcPgToolFinder) << "Encontrado via cache:" << cachedPath;
	  if (found)
		*found = true;
	  return cachedPath;
	}
  }

  // -------------------------------------------------------------------------
  // 1. VARIABLES DE ENTORNO (prioridad máxima, permiten override del usuario)
  // -------------------------------------------------------------------------
  QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

  const QStringList envVars = {
	"PGBIN", "PGHOME", "POSTGRES_HOME", "POSTGRESQL_HOME",
	"PGROOT", "PG_DIR", "POSTGRES_DIR"
  };
  for (const QString &var : envVars) {
	QString val = env.value(var);
	if (val.isEmpty())
	  continue;

	QString fullPath = QDir(val).filePath(exeName);
	if (QFile::exists(fullPath)) {
	  qCDebug(lcPgToolFinder) << "Encontrado via variable de entorno" << var << ":" << fullPath;
	  s_cachedPgBinDir = QFileInfo(fullPath).absolutePath();
	  if (found) *found = true;
	  return QDir::cleanPath(fullPath);
	}
	// Algunos usan la variable como base, no como bin/
	fullPath = QDir(val).filePath("bin/" + exeName);
	if (QFile::exists(fullPath)) {
	  qCDebug(lcPgToolFinder) << "Encontrado via variable de entorno" << var << "(bin/):" << fullPath;
	  s_cachedPgBinDir = QFileInfo(fullPath).absolutePath();
	  if (found) *found = true;
	  return QDir::cleanPath(fullPath);
	}
  }

  // -------------------------------------------------------------------------
  // 2. PATH DEL SISTEMA
  // -------------------------------------------------------------------------
  QString systemPath = QStandardPaths::findExecutable(exeName);
  if (!systemPath.isEmpty()) {
	qCDebug(lcPgToolFinder) << "Encontrado via PATH del sistema:" << systemPath;
	s_cachedPgBinDir = QFileInfo(systemPath).absolutePath();
	if (found) *found = true;
	return systemPath;
  }

#ifdef Q_OS_WIN
  // -------------------------------------------------------------------------
  // 3. REGISTRO DE WINDOWS - Múltiples hives y claves
  // -------------------------------------------------------------------------
  const QStringList registryPaths = {
	// Instaladores oficiales de EDB (64-bit)
	"HKEY_LOCAL_MACHINE\\SOFTWARE\\PostgreSQL\\Installations",
	// Instaladores oficiales de EDB (32-bit en Windows 64-bit)
	"HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\PostgreSQL\\Installations",
	// BigSQL / PostgreSQL installer antiguos
	"HKEY_LOCAL_MACHINE\\SOFTWARE\\PostgreSQL",
	"HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\PostgreSQL",
	// EnterpriseDB antiguo
	"HKEY_LOCAL_MACHINE\\SOFTWARE\\EnterpriseDB",
	"HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\EnterpriseDB",
  };

  for (const QString &regPath : registryPaths) {
	QSettings reg(regPath, QSettings::NativeFormat);
	// Caso A: claves con "Base Directory" directo
	for (const QString &key : reg.childGroups()) {
	  reg.beginGroup(key);
	  QString baseDir = reg.value("Base Directory").toString();
	  if (baseDir.isEmpty())
		baseDir = reg.value("Installation Directory").toString();
	  if (baseDir.isEmpty())
		baseDir = reg.value("InstallLocation").toString();
	  reg.endGroup();

	  if (!baseDir.isEmpty()) {
		QString fullPath = QDir(baseDir).filePath("bin/" + exeName);
		if (QFile::exists(fullPath)) {
		  qCDebug(lcPgToolFinder) << "Encontrado via registro" << regPath << "/" << key << ":" << fullPath;
		  s_cachedPgBinDir = QFileInfo(fullPath).absolutePath();
		  if (found) *found = true;
		  return QDir::cleanPath(fullPath);
		}
	  }
	}
	// Caso B: valor directo en la raíz de la clave
	QString baseDir = reg.value("Base Directory").toString();
	if (baseDir.isEmpty())
	  baseDir = reg.value("Installation Directory").toString();
	if (!baseDir.isEmpty()) {
	  QString fullPath = QDir(baseDir).filePath("bin/" + exeName);
	  if (QFile::exists(fullPath)) {
		qCDebug(lcPgToolFinder) << "Encontrado via registro (raiz)" << regPath << ":" << fullPath;
		s_cachedPgBinDir = QFileInfo(fullPath).absolutePath();
		if (found) *found = true;
		return QDir::cleanPath(fullPath);
	  }
	}
  }

  // 3b. Registro de desinstalación (donde Windows guarda TODAS las apps instaladas)
  const QStringList uninstallPaths = {
	"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
	"HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
	"HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
  };
  for (const QString &regPath : uninstallPaths) {
	QSettings reg(regPath, QSettings::NativeFormat);
	for (const QString &key : reg.childGroups()) {
	  reg.beginGroup(key);
	  QString displayName = reg.value("DisplayName").toString();
	  bool isPostgresEntry = displayName.contains("PostgreSQL", Qt::CaseInsensitive) ||
							 displayName.contains("pgAdmin", Qt::CaseInsensitive) ||
							 key.contains("PostgreSQL", Qt::CaseInsensitive);

	  QString installLoc;
	  if (isPostgresEntry) {
		installLoc = reg.value("InstallLocation").toString();
		// FIX: solo recurrimos a UninstallString (y solo entonces recortamos
		// el nombre de archivo con QFileInfo::path()) cuando InstallLocation
		// vino vacío. Antes esto se ejecutaba SIEMPRE, y le quitaba el último
		// componente a un InstallLocation ya válido (ej. ".../PostgreSQL/15"
		// se convertía incorrectamente en ".../PostgreSQL").
		if (installLoc.isEmpty()) {
		  QString uninstallString = reg.value("UninstallString").toString();
		  // El UninstallString suele ser algo como
		  // "C:\Program Files\PostgreSQL\15\unins000.exe"
		  if (!uninstallString.isEmpty()) {
			QFileInfo fi(uninstallString);
			installLoc = fi.path(); // directorio padre del desinstalador
		  }
		}
	  }
	  reg.endGroup();

	  if (isPostgresEntry && !installLoc.isEmpty()) {
		QString fullPath = QDir(installLoc).filePath("bin/" + exeName);
		if (QFile::exists(fullPath)) {
		  qCDebug(lcPgToolFinder) << "Encontrado via registro de desinstalacion:" << fullPath;
		  s_cachedPgBinDir = QFileInfo(fullPath).absolutePath();
		  if (found) *found = true;
		  return QDir::cleanPath(fullPath);
		}
		// A veces el uninstall está en la raíz, no en bin/
		fullPath = QDir(installLoc).filePath(exeName);
		if (QFile::exists(fullPath)) {
		  qCDebug(lcPgToolFinder) << "Encontrado via registro de desinstalacion (raiz):" << fullPath;
		  s_cachedPgBinDir = QFileInfo(fullPath).absolutePath();
		  if (found) *found = true;
		  return QDir::cleanPath(fullPath);
		}
	  }
	}
  }

  // -------------------------------------------------------------------------
  // 4. BÚSQUEDA HEURÍSTICA EN DISCO (Windows)
  // -------------------------------------------------------------------------
  const QStringList searchRoots = {
	qEnvironmentVariable("ProgramW6432"),      /*C:\Program Files*/
	qEnvironmentVariable("ProgramFiles(x86)"), /*C:\Program Files (x86)*/
	qEnvironmentVariable("ProgramFiles"),      /*Fallback*/
	  qEnvironmentVariable("SystemDrive") + "/", /*C:\*/
	qEnvironmentVariable("LOCALAPPDATA"),      /*%LOCALAPPDATA%*/
	qEnvironmentVariable("APPDATA"),           /*%APPDATA%*/
  };

  auto sortVersionsDescending = [](QStringList &versions) {
	std::sort(versions.begin(), versions.end(), [](const QString &a, const QString &b) {
	  return QVersionNumber::fromString(a) > QVersionNumber::fromString(b);
	});
  };

  for (const QString &root : searchRoots) {
	if (root.isEmpty()) continue;

	// A) PostgreSQL oficial: <root>\PostgreSQL\<version>\bin
	QDir pgDir(root + "/PostgreSQL");
	if (pgDir.exists()) {
	  QStringList versions = pgDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
	  sortVersionsDescending(versions);
	  for (const QString &v : std::as_const(versions)) {
		QString fullPath = pgDir.filePath(v + "/bin/" + exeName);
		if (QFile::exists(fullPath)) {
		  qCDebug(lcPgToolFinder) << "Encontrado via heuristica de disco (PostgreSQL):" << fullPath;
		  s_cachedPgBinDir = QFileInfo(fullPath).absolutePath();
		  if (found) *found = true;
		  return QDir::cleanPath(fullPath);
		}
	  }
	}

	// B) Búsqueda en carpetas con nombre *postgres*/*pgsql*/*edb* (1 nivel)
	QDir rootDir(root);
	QStringList postgresLike = rootDir.entryList(QStringList() << "*postgres*" << "*pgsql*" << "*edb*",
												 QDir::Dirs | QDir::NoDotAndDotDot);
	for (const QString &folder : std::as_const(postgresLike)) {
	  QDir candidate(rootDir.filePath(folder));
	  // Buscar bin/ directo o <version>/bin
	  QString fullPath = candidate.filePath("bin/" + exeName);
	  if (QFile::exists(fullPath)) {
		qCDebug(lcPgToolFinder) << "Encontrado via heuristica de disco:" << fullPath;
		s_cachedPgBinDir = QFileInfo(fullPath).absolutePath();
		if (found) *found = true;
		return QDir::cleanPath(fullPath);
	  }

	  QStringList subDirs = candidate.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
	  sortVersionsDescending(subDirs);
	  for (const QString &sub : std::as_const(subDirs)) {
		fullPath = candidate.filePath(sub + "/bin/" + exeName);
		if (QFile::exists(fullPath)) {
		  qCDebug(lcPgToolFinder) << "Encontrado via heuristica de disco (subdir):" << fullPath;
		  s_cachedPgBinDir = QFileInfo(fullPath).absolutePath();
		  if (found) *found = true;
		  return QDir::cleanPath(fullPath);
		}
	  }
	}
  }

  // C) psqlODBC
  const QStringList appDirs = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
  for (const QString &baseDir : appDirs) {
	QString odbcPath = baseDir + "/PostgreSQL/psqlODBC/bin/" + exeName;
	if (QFile::exists(odbcPath)) {
	  qCDebug(lcPgToolFinder) << "Encontrado via psqlODBC:" << odbcPath;
	  s_cachedPgBinDir = QFileInfo(odbcPath).absolutePath();
	  if (found) *found = true;
	  return QDir::cleanPath(odbcPath);
	}
  }

#else // Linux / macOS
  // -------------------------------------------------------------------------
  // 5. RUTAS ESTÁNDAR EN LINUX / macOS
  // -------------------------------------------------------------------------
  const QStringList unixPaths = {
	"/usr/lib/postgresql",           // Debian/Ubuntu: /usr/lib/postgresql/16/bin
	"/usr/pgsql",                    // RHEL/CentOS/Fedora: /usr/pgsql-16/bin
	"/opt/PostgreSQL",               // Instalador EDB oficial
	"/opt/postgres",                 // Instalaciones manuales
	"/usr/local/pgsql",              // Compilación desde fuentes
	"/usr/local/postgres",
	"/var/lib/pgsql",                // Algunas distros
	"/Applications/Postgres.app/Contents/Versions", // macOS Postgres.app
	"/opt/homebrew/opt/postgresql",  // macOS Homebrew (Apple Silicon)
	"/usr/local/opt/postgresql",     // macOS Homebrew (Intel)
  };

  for (const QString &base : unixPaths) {
	QDir d(base);
	if (!d.exists()) continue;

	// Si la ruta ya termina en bin/, probar directo
	QString direct = d.filePath(exeName);
	if (QFile::exists(direct)) {
	  qCDebug(lcPgToolFinder) << "Encontrado via ruta estandar unix:" << direct;
	  s_cachedPgBinDir = QFileInfo(direct).absolutePath();
	  if (found) *found = true;
	  return QDir::cleanPath(direct);
	}

	// Buscar subdirectorios de versión, priorizando la más reciente
	QStringList entries = d.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
	std::sort(entries.begin(), entries.end(), [](const QString &a, const QString &b) {
	  static const QRegularExpression numRe("(\\d+(\\.\\d+)?)");
	  auto extract = [](const QString &s) -> double {
		auto m = numRe.match(s);
		return m.hasMatch() ? m.captured(1).toDouble() : -1.0;
	  };
	  return extract(a) > extract(b);
	});
	for (const QString &entry : entries) {
	  // Filtrar solo carpetas que parecen versiones (números o versiones)
	  static const QRegularExpression re("^\\d+(\\.\\d+)?$");
	  if (re.match(entry).hasMatch() || entry.contains("sql", Qt::CaseInsensitive)) {
		QString fullPath = d.filePath(entry + "/bin/" + exeName);
		if (QFile::exists(fullPath)) {
		  qCDebug(lcPgToolFinder) << "Encontrado via ruta estandar unix (version):" << fullPath;
		  s_cachedPgBinDir = QFileInfo(fullPath).absolutePath();
		  if (found) *found = true;
		  return QDir::cleanPath(fullPath);
		}
	  }
	}
  }

  // macOS/Linux: buscar con `which` como último recurso
  QProcess which;
  which.start("which", QStringList() << toolName);
  if (which.waitForFinished(2000) && which.exitCode() == 0) {
	QString path = QString::fromUtf8(which.readAllStandardOutput()).trimmed();
	if (!path.isEmpty() && QFile::exists(path)) {
	  qCDebug(lcPgToolFinder) << "Encontrado via which:" << path;
	  s_cachedPgBinDir = QFileInfo(path).absolutePath();
	  if (found) *found = true;
	  return path;
	}
  }
#endif

  // -------------------------------------------------------------------------
  // 6. ÚLTIMO RECURSO: devolver el nombre tal cual y que el SO resuelva
  //    (found queda en false: el llamador puede distinguir "no encontrado"
  //    de "encontrado", en vez de asumir éxito por tener un QString no vacío)
  // -------------------------------------------------------------------------
  qCWarning(lcPgToolFinder) << "No se pudo localizar" << exeName << "; se devuelve el nombre desnudo como ultimo recurso";
  return exeName;
}

bool HelperDataBase_t::ensureDatabaseAndSchemaReady(DbConfig &config, QWidget* parent) {
  const QString adminConnName = QStringLiteral("AdminConnection");
  bool adminConnected = false;

  // -------------------------------------------------------------------------
  // PASO 1: Validar conexión a 'postgres' y crear BD si no existe
  // -------------------------------------------------------------------------
  while (!adminConnected) {
	{ // Scope delimitado para dbAdmin y queries asociadas
	  QSqlDatabase dbAdmin = QSqlDatabase::addDatabase(QStringLiteral("QPSQL"), adminConnName);
	  dbAdmin.setHostName(config.host);
	  dbAdmin.setPort(config.port);
	  dbAdmin.setUserName(config.userName);
	  dbAdmin.setPassword(config.password);
	  dbAdmin.setDatabaseName(QStringLiteral("postgres"));

	  if (dbAdmin.open()) {
		adminConnected = true;

		QSqlQuery checkQuery(dbAdmin);
		checkQuery.prepare(QStringLiteral("SELECT 1 FROM pg_database WHERE datname = ?"));
		checkQuery.addBindValue(config.dbName);

		if (checkQuery.exec() && !checkQuery.next()) {
		  qDebug() << "Creando base de datos:" << config.dbName;
		  QSqlQuery createDbQuery(dbAdmin);
		  QString createSql = QStringLiteral("CREATE DATABASE \"%1\" ENCODING 'UTF8';").arg(config.dbName);

		  if (!createDbQuery.exec(createSql)) {
			qCritical() << "Error creando la base de datos:" << createDbQuery.lastError().text();
			dbAdmin.close();
			// IMPORTANTE: No llamar a removeDatabase aquí; dejar que finalice el scope
			return false;
		  }
		}
		dbAdmin.close();
	  }
	} // dbAdmin, checkQuery y createDbQuery se destruyen al salir de este bloque

	QSqlDatabase::removeDatabase(adminConnName); // Ahora es seguro remover la conexión

	if (!adminConnected) {


	  const QString dialogTitle = SW::Helper_t::appName();
	  const QString dialogLabel = QStringLiteral(
		"<b>Se requiere la contraseña de PostgreSQL</b><br><br>"
		"Por favor, ingrese la contraseña que asignó durante el <b>proceso de instalación</b> del servidor.<br>"
		"<i>(Si no conoce esta contraseña o el servidor fue configurado por otra persona, "
		"contacte al Administrador de Sistemas o de Base de Datos).</i>"
		);

	  bool ok = false;
	  QString pwd = QInputDialog::getText(
		parent, dialogTitle, dialogLabel, QLineEdit::Password, config.password, &ok
		);

	  if (ok && !pwd.isEmpty()) {
		config.password = pwd;
		SW::Helper_t::saveDbConfig(config);
	  } else {
		return false;
	  }
	}
  }

  // -------------------------------------------------------------------------
  // PASO 2: Conectar a la BD de la app y ejecutar script de tablas/funciones
  // -------------------------------------------------------------------------
  const QString appConnName = QStringLiteral("InitConnection");
  {
	QSqlDatabase dbApp = QSqlDatabase::addDatabase(QStringLiteral("QPSQL"), appConnName);
	dbApp.setHostName(config.host);
	dbApp.setPort(config.port);
	dbApp.setUserName(config.userName);
	dbApp.setPassword(config.password);
	dbApp.setDatabaseName(config.dbName);

	if (!dbApp.open()) {
	  qCritical() << "Error al conectar con la BD de la app:" << dbApp.lastError().text();
	  return false;
	}

	QFile scriptFile(QStringLiteral(":/database/tableAndFunctions.sql"));
	if (!scriptFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
	  qCritical() << "No se pudo abrir :/database/tableAndFunctions.sql";
	  dbApp.close();
	  return false;
	}

	QString scriptContent = QString::fromUtf8(scriptFile.readAll());
	scriptFile.close();

	QSqlQuery query(dbApp);
	if (!query.exec(scriptContent)) {
	  qCritical() << "Error al ejecutar el script de inicialización:" << query.lastError().text();
	  dbApp.close();
	  return false;
	}

	dbApp.close();
  }
  QSqlDatabase::removeDatabase(appConnName);

  qDebug() << "Base de datos y esquema verificados correctamente.";
  return true;
}

QJsonArray HelperDataBase_t::exportUserCategories(uint32_t userId) noexcept {

  QJsonArray result;

  qry_.prepare(R"(SELECT * FROM fn_export_user_data(?, ?))");
  qry_.addBindValue(userId);
  qry_.addBindValue(encryptionKey_);

  if (!qry_.exec()) {
	errorMessage_ = qry_.lastError().text();
	return result;
  }

  QHash<QString, QJsonObject> categoriesByName;
  QStringList orderedNames;

  while (qry_.next()) {
	const QString catName = qry_.value(0).toString();
	const QString catDesc = qry_.value(1).toString();
	const QString urlText = qry_.value(2).toString();
	const QString urlDesc = qry_.value(3).toString();

	if (!categoriesByName.contains(catName)) {
	  QJsonObject cat;
	  cat["name"] = catName;
	  cat["desc"] = catDesc;
	  cat["urls"] = QJsonArray();
	  categoriesByName.insert(catName, cat);
	  orderedNames.append(catName);
	}

	QJsonObject cat = categoriesByName[catName];
	QJsonArray urls = cat["urls"].toArray();
	QJsonObject urlObj;
	urlObj["url"] = urlText;
	urlObj["desc"] = urlDesc;
	urls.append(urlObj);
	cat["urls"] = urls;
	categoriesByName[catName] = cat;
  }

  for (const auto& name : std::as_const(orderedNames)) {
	result.append(categoriesByName[name]);
  }

  return result;
}

bool HelperDataBase_t::restoreUserCategories(uint32_t targetUserId, const QJsonArray& categories,
											 DuplicateAction action, QString* errorOut) noexcept {

  for (const auto& catValue : categories) {
	const QJsonObject cat = catValue.toObject();
	const QString catName = cat["name"].toString();
	const QString catDesc = cat["desc"].toString();

	if (!categoryExists(catName, targetUserId)) {
	  if (!saveCategoryData(catName, catDesc, targetUserId)) {
		if (errorOut) *errorOut = errorMessage_;
		return false;
	  }
	}

	uint32_t categoryId = 0;
	for (const auto& pair : loadList_Category(targetUserId)) {
	  if (pair.second == catName) { categoryId = pair.first; break; }
	}
	if (categoryId == 0) continue;

	QList<UrlImportData> items;
	const QJsonArray urls = cat["urls"].toArray();
	items.reserve(urls.size());
	for (const auto& urlValue : urls) {
	  const QJsonObject urlObj = urlValue.toObject();
	  items.append({urlObj["url"].toString(), urlObj["desc"].toString()});
	}

	if (!importUrlsBatch(categoryId, items, action)) {
	  if (errorOut) *errorOut = errorMessage_;
	  return false;
	}
  }

  return true;
}


bool HelperDataBase_t::userExists(QStringView user) noexcept {

  qry_.prepare(R"(SELECT fn_user_exists(?))");
  qry_.addBindValue(user.toString());  // texto plano — fn_user_exists hashea internamente

  if(!qry_.exec()){
	errorMessage_ = qry_.lastError().text();
	return false;
  }

  return qry_.first() ? qry_.value(0).toBool() : false;
}

bool HelperDataBase_t::userExists() noexcept {

  qry_.prepare(R"(SELECT fn_any_user_exists())");

  if(!qry_.exec()){
	errorMessage_ = qry_.lastError().text();
	return false;
  }

  return qry_.first() ? qry_.value(0).toBool() : false;
}

bool HelperDataBase_t::categoryExists(QStringView category, uint32_t userId) noexcept {

  qry_.prepare(R"(SELECT fn_category_exists(?, ?))");
  qry_.addBindValue(category.toString());
  qry_.addBindValue(userId);

  if(!qry_.exec()){
	errorMessage_ = qry_.lastError().text();
	return false;
  }

  return qry_.first() ? qry_.value(0).toBool() : false;
}


bool HelperDataBase_t::urlExists(QStringView url, uint32_t categoryid) noexcept {

  qry_.prepare(R"(SELECT fn_url_exists(?, ?))");
  qry_.addBindValue(url.toString().simplified());
  qry_.addBindValue(categoryid);

  if(!qry_.exec()){
	errorMessage_ = qry_.lastError().text();
	return false;
  }

  return qry_.first() ? qry_.value(0).toBool() : false;
}


bool HelperDataBase_t::createUser(QStringView user, QStringView password, QStringView user_prof,
								  QStringView rescue_type, QStringView val1, QStringView val2) noexcept{

  qry_.prepare(R"(SELECT fn_create_user(?,?,?,?,?,?,?))");
  qry_.addBindValue(user.toString());
  qry_.addBindValue(password.toString());
  qry_.addBindValue(user_prof.toString());
  qry_.addBindValue(rescue_type.toString());
  qry_.addBindValue(val1.toString());
  qry_.addBindValue(val2.toString());
  qry_.addBindValue(encryptionKey_);

  bool result = qry_.exec();
  if(!result){
	errorMessage_ = qry_.lastError().text();
  } else {
	errorMessage_.clear();
  }
  return result;
}

bool HelperDataBase_t::logIn(QStringView user, QStringView password) noexcept{

  qry_.prepare(R"(SELECT fn_login(?, ?))");
  qry_.addBindValue(user.toString());
  qry_.addBindValue(password.toString());

  if(!qry_.exec()){
	errorMessage_ = qry_.lastError().text();
	return false;
  }

  if(!qry_.first()) return false;

  // PostgreSQL BOOLEAN → Qt lo devuelve como bool directamente
  return qry_.value(0).toBool();
}

std::optional<SW::SessionPermissions> HelperDataBase_t::getUserPermissions(const QString& username) noexcept {

  qry_.prepare(R"(SELECT user_priv, can_edit, must_change_password FROM fn_get_user_permissions(?))");
  qry_.addBindValue(username);

  if (!qry_.exec()) {
	errorMessage_ = qry_.lastError().text();
	return std::nullopt;
  }

  if (!qry_.first()) return std::nullopt;

  SW::SessionPermissions perms;
  perms.role = (qry_.value(0).toString() == QStringLiteral("ADMIN"))
				 ? SW::UserRole::Role_Admin : SW::UserRole::Role_User;
  perms.canEdit = qry_.value(1).toBool();
  perms.mustChangePassword = qry_.value(2).toBool();

  return perms;
}


bool HelperDataBase_t::saveCategoryData(QStringView catName, QStringView desc, uint32_t userid) noexcept {

  qry_.prepare(R"(SELECT fn_save_category(?, ?, ?))");
  qry_.addBindValue(catName.toString());
  qry_.addBindValue(desc.toString());
  qry_.addBindValue(userid);

  if(!qry_.exec()){
	errorMessage_ = qry_.lastError().text();
	return false;
  }

  errorMessage_.clear();
  return true;
}


bool HelperDataBase_t::updateCategory(QStringView url, QStringView desc, uint32_t category_id, uint32_t user_id) noexcept{

  qry_.prepare(R"(SELECT fn_update_category(?, ?, ?, ?))");
  qry_.addBindValue(url.toString());
  qry_.addBindValue(desc.toString());
  qry_.addBindValue(category_id);
  qry_.addBindValue(user_id);

  if(!qry_.exec()){
	errorMessage_ = qry_.lastError().text();
	return false;
  }
  return qry_.first() ? qry_.value(0).toBool() : false;
}


bool HelperDataBase_t::saveData_url(QStringView url, QStringView desc, uint32_t id) noexcept {

  qry_.prepare(R"(SELECT fn_save_url(?, ?, ?, ?))");
  qry_.addBindValue(url.toString().simplified());
  qry_.addBindValue(desc.toString());
  qry_.addBindValue(id);
  qry_.addBindValue(encryptionKey_);

  if(!qry_.exec()){
	errorMessage_ = qry_.lastError().text();
	return false;
  }

  errorMessage_.clear();
  return true;
}


bool HelperDataBase_t::updateData_url(QStringView url, QStringView desc, uint32_t id, uint32_t categoryId) noexcept{

  qry_.prepare(R"(SELECT fn_update_url(?, ?, ?, ?, ?))");
  qry_.addBindValue(url.toString());
  qry_.addBindValue(desc.toString());
  qry_.addBindValue(id);
  qry_.addBindValue(categoryId);
  qry_.addBindValue(encryptionKey_);

  if(!qry_.exec()){
	errorMessage_ = qry_.lastError().text();
	return false;
  }

  if(!qry_.first()){
	errorMessage_ = "fn_update_url no retornó valor";
	return false;
  }

  bool result = qry_.value(0).toBool();
  if(!result){
	errorMessage_ = "No se actualizó ninguna URL (ID o categoría incorrectos)";
  } else {
	errorMessage_.clear();
  }
  return result;
}


bool HelperDataBase_t::deleteUrls(DeleteUrlMode op, uint32_t categoryId, uint32_t urlId) noexcept{

  if(op == DeleteUrlMode::ByCategory){
	qry_.prepare(R"(SELECT fn_delete_urls_by_category(?))");
	qry_.addBindValue(categoryId);
  }else{
	qry_.prepare(R"(SELECT fn_delete_url_by_id(?))");
	qry_.addBindValue(urlId);
  }

  if(!qry_.exec()){
	errorMessage_ = qry_.lastError().text();
	return false;
  }
  return qry_.first() ? qry_.value(0).toBool() : false;
}

bool HelperDataBase_t::deleteCategory(uint32_t categoryId) noexcept {

  qry_.prepare(R"(SELECT fn_delete_category(?))");
  qry_.addBindValue(categoryId);

  if(!qry_.exec()){
	errorMessage_ = qry_.lastError().text();
	return false;
  }
  return qry_.first() ? qry_.value(0).toBool() : false;
}


bool HelperDataBase_t::validateAnswer(QStringView respuesta, uint32_t userId) noexcept{

  qry_.prepare(R"(SELECT fn_validate_answer(?, ?))");
  qry_.addBindValue(userId);
  qry_.addBindValue(respuesta.toString().simplified());

  if(!qry_.exec()){
	errorMessage_ = qry_.lastError().text();
	return false;
  }
  return qry_.first() ? qry_.value(0).toBool() : false;
}

bool HelperDataBase_t::resetPassword(QStringView password, uint32_t userId) noexcept{

  qry_.prepare(R"(SELECT fn_reset_password(?, ?))");
  qry_.addBindValue(userId);
  qry_.addBindValue(password.toString());

  if(!qry_.exec()){
	errorMessage_ = qry_.lastError().text();
	return false;
  }

  return qry_.first() ? qry_.value(0).toBool() : false;
}


bool HelperDataBase_t::moveUrlToOtherCategory(uint32_t categoryId, uint32_t urlId) noexcept{

  qry_.prepare(R"(SELECT fn_move_url_to_category(?, ?))");
  qry_.addBindValue(urlId);
  qry_.addBindValue(categoryId);

  if(!qry_.exec()){
	errorMessage_ = qry_.lastError().text();
	return false;
  }
  return qry_.first() ? qry_.value(0).toBool() : false;
}


bool HelperDataBase_t::isDataBase_empty() noexcept {

  auto tables = db_.tables();
  uint32_t count{0};
  foreach (const auto& table, tables) {
	if(table == "users"){
	  continue;
	}
	qry_.prepare(QString("SELECT COUNT(*) FROM %1").arg(table));
	if(qry_.exec()){
	  qry_.next();
	  if(qry_.value(0).toInt() == 0) ++count;
	}
  }
  return (count == tables.size()-1);
}


QList<QPair<uint32_t, QString>> HelperDataBase_t::loadList_Category(uint32_t user_id) noexcept{
  QList<QPair<uint32_t, QString>> categoryList{};

  qry_.setForwardOnly(false);   // ← garantiza que size() sea confiable, sin importar el estado previo del objeto compartido
  qry_.prepare(R"(SELECT * FROM fn_load_category_list(?))");
  qry_.addBindValue(user_id);

  if (qry_.exec()) {
	if (const int rowCount = qry_.size(); rowCount > 0)
	  categoryList.reserve(rowCount);

	while (qry_.next()) {
	  categoryList.emplaceBack(qry_.value(0).toUInt(), qry_.value(1).toString());
	}
  } else {
	errorMessage_ = qry_.lastError().text();
  }

  return categoryList;
}

int HelperDataBase_t::getUser_id(const QString& user, SW::User user_profile) noexcept {

  qry_.prepare(R"(SELECT fn_get_user_id(?, ?))");
  qry_.addBindValue(user);
  qry_.addBindValue(SW::Helper_t::currentUser_.value(user_profile).simplified());

  if(!qry_.exec()){
	errorMessage_ = qry_.lastError().text();
	return -1;
  }

  return qry_.first() ? qry_.value(0).toInt() : 0;
}


QStringList HelperDataBase_t::dataCategory(uint32_t category_id) noexcept {
  QStringList dataCategory{};

  qry_.prepare(R"(SELECT * FROM fn_get_category_data(?))");
  qry_.addBindValue(category_id);

  if(qry_.exec() && qry_.next()){
	dataCategory.append(qry_.value(0).toString());
	dataCategory.append(qry_.value(1).toString());
  } else {
	errorMessage_ = qry_.lastError().text();
  }
  return dataCategory;
}


QString HelperDataBase_t::validateRescueType(uint32_t userId) noexcept{
  qry_.prepare(R"(SELECT * FROM fn_validate_rescue_type(?))");
  qry_.addBindValue(userId);

  if(qry_.exec() && qry_.next()){
	return qry_.value(0).toString();
  }
  errorMessage_ = qry_.lastError().text();
  return QString();
}

QString HelperDataBase_t::getQuestion(uint32_t userId) noexcept {

  qry_.prepare(R"(SELECT fn_get_question(?, ?))");
  qry_.addBindValue(userId);
  qry_.addBindValue(encryptionKey_);

  if(!qry_.exec()){
	errorMessage_ = qry_.lastError().text();
	return {};
  }

  return qry_.first() ? qry_.value(0).toString() : QString{};
}

}//namespace SW