#include "swtablemodel.hpp"

#include <QTextDocument>

SWTableModel::SWTableModel(QObject* parent)
  : QSqlQueryModel{parent}
{
}

QVariant SWTableModel::data(const QModelIndex& index, int role) const {

  if((role == Qt::DisplayRole || role == Qt::ToolTipRole) && (index.column() == 1 || index.column() == 2)){
	QTextDocument doc;
	doc.setHtml(QSqlQueryModel::data(index, Qt::DisplayRole).toString());
	if(doc.toPlainText().isEmpty()){
	  return tr("Sin descripción disponible.");
	}
	return doc.toPlainText();
  }

  // Todo lo demás — datos llegan descifrados desde fn_get_urls
  return QSqlQueryModel::data(index, role);
}

QVariant SWTableModel::headerData(int section, Qt::Orientation orientation, int role) const {

  Q_UNUSED(section)

  if(orientation == Qt::Vertical && role == Qt::SizeHintRole)
	return QSize(0, 0);

  if(orientation == Qt::Vertical && role == Qt::DisplayRole)
	return QVariant();

  return QSqlQueryModel::headerData(section, orientation, role);
}