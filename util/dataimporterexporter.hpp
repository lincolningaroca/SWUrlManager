#pragma once

#include "util/sw_types.hpp"

#include <QCoreApplication>
#include <QList>
#include <QString>

class QTableView;

namespace SW {

class DataImporterExporter {

public:

  enum class ExportFormat {
	Xlsx,
	Csv,
	Tsv,
	Txt
  };

  Q_DECLARE_TR_FUNCTIONS(DataImporterExporter)

public:

  explicit DataImporterExporter() = default;

  // --- MÉTODOS DE EXPORTACIÓN ---
  [[nodiscard]] static bool exportTableView(QTableView *tableView, const QString &filePath, QString* errorOut = nullptr);

  // --- MÉTODOS DE IMPORTACIÓN ---
  [[nodiscard]] static QList<SW::UrlImportData> importFromFile(const QString &filePath, QString* errorOut = nullptr);



private:
  // Auxiliares de Exportación
  static bool exportToXlsx(QTableView *tableView, const QString &filePath, QString* errorOut);
  static bool exportToTextFormat(QTableView *tableView, const QString &filePath, QChar delimiter, QString* errorOut);
  static QString escapeCsvField(const QString &field, QChar delimiter);

  // Auxiliares de Importación
  static QList<SW::UrlImportData> importFromXlsx(const QString &filePath, QString* errorOut);
  static QList<SW::UrlImportData> importFromTextFormat(const QString &filePath, QString* errorOut);

};

} // namespace SW