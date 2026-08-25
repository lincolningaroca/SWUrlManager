#pragma once

#include "helperdatabase/helperdb.hpp"

#include <QObject>
#include <QString>
#include <QStringList>
#include <QAtomicInt>

namespace SW {

class UrlImportWorker : public QObject {
  Q_OBJECT
public:
  explicit UrlImportWorker(QObject* parent = nullptr) : QObject(parent) {}

public slots:

  void doImport(const QString& filePath, uint32_t categoryId, const QByteArray& dek);
  void cancel() noexcept { cancelled_.storeRelaxed(1); }

signals:
  void progressChanged(int value, int max, const QString& message);
  void finished(bool ok, int inserted, int updated, int skipped, const QString& errorMessage);

private:
  QAtomicInt cancelled_{0};
};

} // namespace SW