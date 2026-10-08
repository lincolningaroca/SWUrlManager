#include "urlimportworker.hpp"

#include "util/dataimporterexporter.hpp"
#include "util/helper.hpp"

#include <QSqlError>
#include <QMetaObject>
#include <QUuid>

namespace SW {

void UrlImportWorker::doImport(const QString& filePath, uint32_t categoryId, const QByteArray& dek, QObject* context) {
  const QString connName = QStringLiteral("ImportWorker_%1").arg(QUuid::createUuid().toString());

  QSqlDatabase db;
  {
	const auto config = SW::Helper_t::loadDbConfig();
	db = QSqlDatabase::addDatabase(QStringLiteral("QPSQL"), connName);
	db.setHostName(config.host);
	db.setPort(config.port);
	db.setDatabaseName(config.dbName);
	db.setUserName(config.userName);
	db.setPassword(config.password);

	if (!db.open()) {
	  emit finished(false, 0, 0, 0,
					QObject::tr("No se pudo conectar a la base de datos: %1").arg(db.lastError().text()));
	  db = QSqlDatabase();
	  QSqlDatabase::removeDatabase(connName);
	  return;
	}

	// Configurar codificación
	{
	  QSqlQuery qSetEncoding(db);
	  qSetEncoding.exec(QStringLiteral("SET client_encoding TO 'UTF8';"));
	}

	// SCOPE EXPLÍCITO: helperdb se destruye AQUÍ, liberando todos sus queries internos
	// ANTES de que cerremos la conexión db.
	{
	  SW::HelperDataBase_t helperdb(db);
	  helperdb.setEncryptionKeyRaw(dek);

	  emit progressChanged(0, 0, QObject::tr("Leyendo archivo..."));
	  if (cancelled_.loadRelaxed()) {
		emit finished(false, 0, 0, 0, QString());
		return;
	  }

	  QString importError;
	  const QList<SW::UrlImportData> rawItems = SW::DataImporterExporter::importFromFile(filePath, &importError);
	  if (rawItems.isEmpty()) {
		emit finished(false, 0, 0, 0,
					  importError.isEmpty()
						? QObject::tr("El archivo está vacío o no se pudieron extraer registros.")
						: importError);
		return;
	  }

	  QList<SW::UrlImportData> validItems;
	  validItems.reserve(rawItems.size());
	  for (const auto& item : std::as_const(rawItems)) {
		const QString cleanedUrl = item.url.trimmed();
		if (cleanedUrl.isEmpty()) continue;
		if (SW::Helper_t::urlValidate(cleanedUrl))
		  validItems.append({cleanedUrl, item.description});
	  }

	  if (validItems.isEmpty()) {
		emit finished(false, 0, 0, 0,
					  QObject::tr("El archivo no contiene un formato de datos adecuado para la importación."));
		return;
	  }

	  // 4. Detección de duplicados
	  QStringList duplicateUrls;
	  const int total = static_cast<int>(validItems.size());
	  for (int i = 0; i < total; ++i) {
		if (cancelled_.loadRelaxed()) {
		  emit finished(false, 0, 0, 0, QString());
		  db.close();
		  return;
		}
		if (helperdb.urlExists(validItems[i].url, categoryId))
		  duplicateUrls.append(validItems[i].url.trimmed());
		if (i % 5 == 0 || i == total - 1)
		  emit progressChanged(i + 1, total, QObject::tr("Verificando duplicados..."));
	  }

	  // 5. Diálogo en hilo principal
	  SW::DuplicateAction action = SW::DuplicateAction::Omit;
	  if (!duplicateUrls.isEmpty()) {
		// Cerrar el progressDialog antes de mostrar el QMessageBox
		emit requestCloseProgressDialog();

		int actionInt = static_cast<int>(SW::DuplicateAction::Omit);
		const bool invoked = QMetaObject::invokeMethod(
		  context, "resolveDuplicatesDialog",
		  Qt::BlockingQueuedConnection,
		  Q_RETURN_ARG(int, actionInt),
		  Q_ARG(QStringList, duplicateUrls));

		if (!invoked || actionInt < 0) {
		  emit finished(false, 0, 0, 0, QString());
		  db.close();
		  return;
		}
		action = static_cast<SW::DuplicateAction>(actionInt);

		// Reabrir el progressDialog para la fase de guardado
		emit requestShowProgressDialog(QObject::tr("Preparando importación..."));
	  }

	  const int totalToSave = static_cast<int>(validItems.size());
	  emit progressChanged(0, totalToSave, QObject::tr("Guardando en la base de datos..."));
	  int insertedCount = 0, updatedCount = 0;

	  const bool ok = helperdb.importUrlsBatch(categoryId, validItems, action, &insertedCount, &updatedCount,
											   [this](int done, int total_i) {
												 emit progressChanged(done, total_i,
																	  QObject::tr("Guardando en la base de datos... (%1/%2)").arg(done).arg(total_i));
											   });

	  const QString err = ok ? QString() : helperdb.errorMessage();
	  emit finished(ok, insertedCount, updatedCount,
					static_cast<int>(validItems.size()) - (insertedCount + updatedCount), err);
	} // <-- helperdb se destruye aquí, liberando queries

	db.close();
  } // <-- db se destruye aquí

  // Ahora es 100% seguro remover la conexión porque no quedan objetos QSqlDatabase ni queries vivos
  QSqlDatabase::removeDatabase(connName);
}

} // namespace SW