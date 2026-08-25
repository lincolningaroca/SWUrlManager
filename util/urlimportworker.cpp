#include "urlimportworker.hpp"

#include "util/dataimporterexporter.hpp"
#include "util/helper.hpp"

#include <QSqlError>
#include <QMetaObject>
#include <QUuid>

namespace SW {

void UrlImportWorker::doImport(const QString& filePath, uint32_t categoryId, const QByteArray& dek) {

  // 1. Conexión a BD EXCLUSIVA de este hilo — obligatorio en Qt SQL
  const QString connName = QStringLiteral("ImportWorker_%1").arg(QUuid::createUuid().toString());
  const auto config = SW::Helper_t::loadDbConfig();

  QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QPSQL"), connName);
  db.setHostName(config.host);
  db.setPort(config.port);
  db.setDatabaseName(config.dbName);
  db.setUserName(config.userName);
  db.setPassword(config.password);

  if (!db.open()) {
	emit finished(false, 0, 0, 0,
				  QObject::tr("No se pudo conectar a la base de datos: %1").arg(db.lastError().text()));
	QSqlDatabase::removeDatabase(connName);
	return;
  }

  SW::HelperDataBase_t helperdb(db);
  helperdb.setEncryptionKeyRaw(dek);

  // 2. Parseo del archivo
  emit progressChanged(0, 0, QObject::tr("Leyendo archivo..."));

  if (cancelled_.loadRelaxed()) {
	emit finished(false, 0, 0, 0, QString());
	QSqlDatabase::removeDatabase(connName);
	return;
  }

  QString importError;
  const QList<SW::UrlImportData> rawItems = SW::DataImporterExporter::importFromFile(filePath, &importError);
  if (rawItems.isEmpty()) {
	emit finished(false, 0, 0, 0,
				  importError.isEmpty()
					? QObject::tr("El archivo está vacío o no se pudieron extraer registros.")
					: importError);
	QSqlDatabase::removeDatabase(connName);
	return;
  }

  // 3. Validación de estructura (misma lógica que tenía processImportFile)
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
	QSqlDatabase::removeDatabase(connName);
	return;
  }

  // 4. Detección de duplicados — con progreso real
  QStringList duplicateUrls;
  const int total = static_cast<int>(validItems.size());
  for (int i = 0; i < total; ++i) {
	if (cancelled_.loadRelaxed()) {
	  emit finished(false, 0, 0, 0, QString());
	  QSqlDatabase::removeDatabase(connName);
	  return;
	}
	if (helperdb.urlExists(validItems[i].url, categoryId))
	  duplicateUrls.append(validItems[i].url.trimmed());

	if (i % 5 == 0 || i == total - 1)
	  emit progressChanged(i + 1, total, QObject::tr("Verificando duplicados..."));
  }

  // 5. Si hay duplicados, preguntar en el hilo PRINCIPAL (bloqueante solo para el worker)
  SW::DuplicateAction action = SW::DuplicateAction::Omit;
  if (!duplicateUrls.isEmpty()) {
	int actionInt = static_cast<int>(SW::DuplicateAction::Omit);

	const bool invoked = QMetaObject::invokeMethod(
	  parent(), "resolveDuplicatesDialog",
	  Qt::BlockingQueuedConnection,
	  Q_RETURN_ARG(int, actionInt),
	  Q_ARG(QStringList, duplicateUrls));

	if (!invoked || actionInt < 0) {
	  emit finished(false, 0, 0, 0, QString()); // cancelado por el usuario, o fallo de invocación
	  QSqlDatabase::removeDatabase(connName);
	  return;
	}
	action = static_cast<SW::DuplicateAction>(actionInt);
  }

  // 6. Inserción/actualización en lote — reutiliza importUrlsBatch tal cual
  const int totalToSave = static_cast<int>(validItems.size()); // cota superior; el real puede ser menor tras deduplicar
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

  QSqlDatabase::removeDatabase(connName);
}

} // namespace SW