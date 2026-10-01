#include "dataimporterexporter.hpp"
#include "xlsxdocument.h"

#include <QAbstractItemModel>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHeaderView>
#include <QRegularExpression>
#include <QSqlRecord>
#include <QSqlTableModel>
#include <QTableView>
#include <QTextStream>

namespace SW {

QList<SW::UrlImportData> DataImporterExporter::importFromFile(const QString &filePath, QString* errorOut) {

  if (errorOut) errorOut->clear();
  QFileInfo fi(filePath);

  if (!fi.exists()) {
	if (errorOut) *errorOut = tr("El archivo especificado no existe.");
	return {};
  }

  QString ext = fi.suffix().toLower();

  if (ext == QStringLiteral("xlsx")) {
	return importFromXlsx(filePath, errorOut);
  } else {
	return importFromTextFormat(filePath, errorOut);
  }
}

QList<SW::UrlImportData> DataImporterExporter::importFromXlsx(const QString &filePath, QString* errorOut) {

  QList<SW::UrlImportData> list;
  QXlsx::Document xlsx(filePath);

  if (!xlsx.load()) {
	if (errorOut) *errorOut = tr("No se pudo cargar el archivo Excel (.xlsx).");
	return list;
  }

  int rowCount = xlsx.dimension().lastRow();

  for (int row = 1; row <= rowCount; ++row) {
	QString url = xlsx.read(row, 1).toString().trimmed();
	QString desc = xlsx.read(row, 2).toString().trimmed();

	url.remove('\"');
	desc.remove('\"');

	// Omitir cabecera (uso de QString normal en lugar de QStringLiteral)
	if (url.contains("Dirección URL", Qt::CaseInsensitive) ||
		url.contains("Direccion URL", Qt::CaseInsensitive) ||
		url.contains("url", Qt::CaseInsensitive)) {
	  continue;
	}

	if (!url.isEmpty()) {
	  SW::UrlImportData item;
	  item.url = url;
	  item.description = desc;
	  list.append(item);
	}
  }

  return list;
}

QList<SW::UrlImportData> DataImporterExporter::importFromTextFormat(const QString &filePath, QString* errorOut) {

  QList<SW::UrlImportData> list;
  QFile file(filePath);

  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
	if (errorOut) *errorOut = tr("Error: No se pudo abrir el archivo para lectura.");
	return list;
  }

  QTextStream in(&file);
  QChar delimiter = ',';

  while (!in.atEnd()) {
	QString line = in.readLine().trimmed();
	if (line.isEmpty()) continue;

	if (list.isEmpty()) {
	  if (line.contains(';')) delimiter = ';';
	  else if (line.contains('\t')) delimiter = '\t';
	}

	QStringList fields = line.split(delimiter);
	if (fields.isEmpty()) continue;

	SW::UrlImportData importItem;

	QString rawUrl = fields.at(0);
	rawUrl.remove('\"');
	importItem.url = rawUrl.trimmed();

	QString rawDesc;
	if (fields.size() > 1) {
	  rawDesc = fields.at(1);
	  rawDesc.remove('\"');
	}
	importItem.description = rawDesc.trimmed();

	// Omitir cabeceras comunes (uso de QString normal)
	if (importItem.url.contains("Dirección URL", Qt::CaseInsensitive) ||
		importItem.url.contains("Direccion URL", Qt::CaseInsensitive) ||
		importItem.url.contains("url", Qt::CaseInsensitive)) {
	  continue;
	}

	if (!importItem.url.isEmpty()) {
	  list.append(importItem);
	}
  }

  file.close();
  return list;
}


QString DataImporterExporter::escapeCsvField(const QString &field, QChar delimiter) {

  QString result = field;
  bool needsQuotes = result.contains(delimiter) || result.contains('"') || result.contains('\n') || result.contains('\r');

  if (result.contains('"')) {
	result.replace('"', "\"\"");
  }

  if (needsQuotes) {
	result = QString("\"%1\"").arg(result);
  }

  return result;
}

bool DataImporterExporter::exportToTextFormat(QTableView *tableView, const QString &filePath, QChar delimiter, QString* errorOut) {

  QFile file(filePath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
	if (errorOut) *errorOut = tr("Error al crear el archivo: %1").arg(file.errorString());
	return false;
  }

  QTextStream out(&file);
  out.setGenerateByteOrderMark(true); // BOM UTF-8

  auto *model = tableView->model();
  QSet<QString> excludedColumns = {"url_id", "categoryid"};

  QStringList headers;
  for (int col = 0; col < model->columnCount(); ++col) {
	if (tableView->isColumnHidden(col)) continue;

	QString fieldName;
	if (auto sqlModel = qobject_cast<QSqlTableModel*>(model))
	  fieldName = sqlModel->record().fieldName(col).toLower();
	if (excludedColumns.contains(fieldName)) continue;

	headers.append(escapeCsvField(model->headerData(col, Qt::Horizontal).toString(), delimiter));
  }
  out << headers.join(delimiter) << "\n";

  for (int row = 0; row < model->rowCount(); ++row) {
	QStringList rowData;
	for (int col = 0; col < model->columnCount(); ++col) {
	  if (tableView->isColumnHidden(col)) continue;

	  QString fieldName;
	  if (auto sqlModel = qobject_cast<QSqlTableModel*>(model))
		fieldName = sqlModel->record().fieldName(col).toLower();
	  if (excludedColumns.contains(fieldName)) continue;

	  QVariant data = model->data(model->index(row, col), Qt::DisplayRole);
	  QString cellText = (data.isValid() && !data.isNull()) ? data.toString() : QString();
	  rowData.append(escapeCsvField(cellText, delimiter));
	}
	out << rowData.join(delimiter) << "\n";
  }

  file.close();
  return true;
}

bool DataImporterExporter::exportToXlsx(QTableView *tableView, const QString &filePath, QString* errorOut) {

  auto *model = tableView->model();
  QSet<QString> excludedColumns = {"url_id", "categoryid"};

  QXlsx::Document xlsxDocument;

  QXlsx::Format headerFormat;
  headerFormat.setFontBold(true);
  headerFormat.setVerticalAlignment(QXlsx::Format::AlignVCenter);
  headerFormat.setHorizontalAlignment(QXlsx::Format::AlignHCenter);

  QXlsx::Format dataFormat;
  dataFormat.setVerticalAlignment(QXlsx::Format::AlignTop);

  int colExcel = 1;
  for (int col = 0; col < model->columnCount(); ++col) {
	if (tableView->isColumnHidden(col)) continue;

	QString fieldName;
	if (auto sqlModel = qobject_cast<QSqlTableModel*>(model))
	  fieldName = sqlModel->record().fieldName(col).toLower();
	if (excludedColumns.contains(fieldName)) continue;

	const double excelWidth = tableView->columnWidth(col) / 7.0;
	xlsxDocument.setColumnWidth(colExcel, excelWidth);

	xlsxDocument.write(1, colExcel++,
					   model->headerData(col, Qt::Horizontal).toString(),
					   headerFormat);
  }

  for (int row = 0; row < model->rowCount(); ++row) {
	colExcel = 1;
	for (int col = 0; col < model->columnCount(); ++col) {
	  if (tableView->isColumnHidden(col)) continue;

	  QString fieldName;
	  if (auto sqlModel = qobject_cast<QSqlTableModel*>(model))
		fieldName = sqlModel->record().fieldName(col).toLower();
	  if (excludedColumns.contains(fieldName)) continue;

	  QVariant data = model->data(model->index(row, col), Qt::DisplayRole);

	  if (!data.isValid() || data.isNull())
		xlsxDocument.write(row + 2, colExcel++, "", dataFormat);
	  else
		xlsxDocument.write(row + 2, colExcel++, data, dataFormat);
	}
  }

  const int rowHeight = tableView->verticalHeader()->defaultSectionSize();
  const double excelRowHeight = rowHeight / 1.33;

  xlsxDocument.setRowHeight(1, excelRowHeight);

  for (int row = 0; row < model->rowCount(); ++row) {
	xlsxDocument.setRowHeight(row + 2, excelRowHeight);
  }

  const bool saved = xlsxDocument.saveAs(filePath);

  if (!saved && errorOut) {
	*errorOut = tr("No se pudo guardar el archivo Excel en la ruta indicada.");
  }
  return saved;
}

bool DataImporterExporter::exportTableView(QTableView *tableView, const QString &filePath, QString* errorOut) {

  if (errorOut) errorOut->clear();

  if (!tableView) {
	if (errorOut) *errorOut = tr("Error: El TableView es nulo.");
	return false;
  }

  auto *model = tableView->model();
  if (!model) {
	if (errorOut) *errorOut = tr("Error: La tabla no tiene un modelo de datos asignado.");
	return false;
  }

  QFileInfo fi(filePath);
  QString ext = fi.suffix().toLower();

  if (ext == "csv") {
	return exportToTextFormat(tableView, filePath, ',', errorOut);
  } else if (ext == "tsv" || ext == "txt") {
	return exportToTextFormat(tableView, filePath, '\t', errorOut);
  } else {
	QString savePath = filePath;
	if (ext.isEmpty()) savePath += ".xlsx";
	return exportToXlsx(tableView, savePath, errorOut);
  }
}

}// namespace SW










