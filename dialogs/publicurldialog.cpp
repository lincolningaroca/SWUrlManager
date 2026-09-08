#include "publicurldialog.hpp"
#include "ui_publicurldialog.h"

#include "helperdatabase/helperdb.hpp"
#include "maintenanceurldialog.hpp"
#include "swwidgets/switemdelegate.hpp"
#include "swwidgets/swtablemodel.hpp"

#include <QAction>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include <QSqlError>

PublicUrlDialog::PublicUrlDialog(Qt::ColorScheme colorScheme, QWidget *parent) :
  QDialog(parent),
  ui(new Ui::PublicUrlDialog),
  colorScheme_(colorScheme),
  db_{QSqlDatabase::database(QStringLiteral("xxxConection"))}{

  ui->setupUi(this);

  setMaximumSize(QSize(950,500));

  model = new SWTableModel(this);
  ui->urlTableView->setModel(model);

  loadDataComboBox();

  readSettings();

  setupContextMenu();
  applyIcons(colorScheme);

  ui->urlTableView->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(ui->urlTableView, &QTableView::customContextMenuRequested, this, &PublicUrlDialog::on_showContextMenu);

  QObject::connect(ui->categoryComboBox, &QComboBox::currentIndexChanged, this, &PublicUrlDialog::on_categorySelectedChanged);


  QObject::connect(ui->newToolButton, &QToolButton::clicked, this, [this](){on_showMaintenanceDialog(SW::OpenMode::New);});
  QObject::connect(ui->editToolButton, &QToolButton::clicked, this, [this](){on_showMaintenanceDialog(SW::OpenMode::Edit);});
  QObject::connect(ui->quitarToolButton, &QToolButton::clicked, this, PublicUrlDialog::on_deleteUrl);
  QObject::connect(ui->openPushButton, &QToolButton::clicked, this, &PublicUrlDialog::on_openUrl);

  applyTablePalette(colorScheme);

}

PublicUrlDialog::~PublicUrlDialog(){
  delete ui;
}

void PublicUrlDialog::applyTablePalette(Qt::ColorScheme scheme) {

  QPalette tablePalette = ui->urlTableView->palette();
  if (scheme == Qt::ColorScheme::Dark || scheme == Qt::ColorScheme::Unknown) {
	tablePalette.setColor(QPalette::AlternateBase, QColor(35, 35, 35));
  } else {
	tablePalette.setColor(QPalette::AlternateBase, QColor(245, 245, 245));
  }
  ui->urlTableView->setPalette(tablePalette);
}

void PublicUrlDialog::setShowGrid(bool checked){

  ui->urlTableView->setShowGrid(checked);
  ui->urlTableView->viewport()->update();
}


uint32_t PublicUrlDialog::currentCategoryId() const noexcept {
  return ui->categoryComboBox->currentData().isValid() ? ui->categoryComboBox->currentData().toUInt() : 1;
}

void PublicUrlDialog::loadDataComboBox(){

  QSignalBlocker blocker(ui->categoryComboBox);

  ui->categoryComboBox->clear();
  auto user_id = helperdb_.getUser_id(SW::Helper_t::defaultUser, SW::User::U_public);
 auto data_i = helperdb_.loadList_Category(user_id);

  // Recorremos la lista manteniendo el orden exacto
  for (const auto& [id, name] : std::as_const(data_i)) {
	ui->categoryComboBox->addItem(name, id);
  }

}

void PublicUrlDialog::on_categorySelectedChanged(int index){
  Q_UNUSED(index);
  on_loadDataTableView();
}

void PublicUrlDialog::on_deleteUrl(){

  if(!ui->urlTableView->selectionModel()->hasSelection()){
	QMessageBox::warning(this, qApp->applicationName(), "Seleccione una fila.");
	return;
  }

  auto currentRow = ui->urlTableView->currentIndex().row();

  const auto url = ui->urlTableView->model()->index(currentRow, 1).data().toString();

  QMessageBox msgBox(this);
  msgBox.setText(QString("<span>Confirma que desea eliminar esta dirección:<br>"
						 " <cite style='color:#ff0800;'><strong>%1</strong></cite></span>").arg(url));
  msgBox.setIcon(QMessageBox::Question);
  msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
  msgBox.button(QMessageBox::Yes)->setText("Eliminar");
  msgBox.button(QMessageBox::No)->setText("Cancelar");

  if(msgBox.exec() == QMessageBox::Yes){
	const auto urlId=ui->urlTableView->model()->index(currentRow, 0).data().toInt();
	if(helperdb_.deleteUrls(SW::DeleteUrlMode::ByUrlId, 0, urlId)){

	  on_loadDataTableView();
	}

  }

}

void PublicUrlDialog::on_showMaintenanceDialog(SW::OpenMode mode){

  QList<QVariant> dataUrl{};
  if(mode == SW::OpenMode::Edit){

	if(!ui->urlTableView->selectionModel()->hasSelection()){
	  QMessageBox::warning(this, qApp->applicationName(), "Seleccione una fila.");
	  return;
	}

	auto currentIndex = ui->urlTableView->currentIndex();
	auto id = ui->urlTableView->model()->index(currentIndex.row(), 0).data().toInt();

	// Los datos ya llegan descifrados desde fn_get_urls — sin decrypt()
	QSqlQuery query(db_);
	query.prepare(R"(SELECT url_id, url_text, url_desc FROM fn_get_urls_by_id(?, ?))");
	query.addBindValue(id);
	query.addBindValue(helperdb_.encryptionKey());

	if(!query.exec() || !query.next()){
	  QMessageBox::critical(this, qApp->applicationName(), "Error al leer los datos.");
	  return;
	}

	dataUrl.append(query.value(0).toInt());
	dataUrl.append(query.value(1).toString()); // url — ya descifrada
	dataUrl.append(query.value(2).toString()); // desc — ya descifrada
  }

  MaintenanceUrlDialog maintenanceDialog(colorScheme_, mode, dataUrl, currentCategoryId(), this);
  if(maintenanceDialog.exec() == QDialog::Accepted){
	on_loadDataTableView();
  }
}

void PublicUrlDialog::on_loadDataTableView(){

  const auto categoryId_ = currentCategoryId();

  QSqlQuery qry(db_);
  qry.prepare(R"(SELECT * FROM fn_get_urls(?, ?))");
  qry.addBindValue(categoryId_);
  qry.addBindValue(helperdb_.encryptionKey());

  if(!qry.exec()){
	qDebug() << "fn_get_urls error:" << qry.lastError().text();
  }

  model->setQuery(std::move(qry));

  ui->urlTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);

  ui->urlTableView->hideColumn(0);  // url_id

  model->setHeaderData(1, Qt::Horizontal, "Dirección URL");
  model->setHeaderData(2, Qt::Horizontal, "Descripción");

  ui->urlTableView->setItemDelegate(new SWItemDelegate(ui->urlTableView));
  ui->urlTableView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
  ui->urlTableView->verticalHeader()->setDefaultSectionSize(20);
  ui->urlTableView->setAlternatingRowColors(true);
  ui->urlTableView->setMouseTracking(true);
}

void PublicUrlDialog::on_showContextMenu(const QPoint &pos){

  auto index = ui->urlTableView->indexAt(pos);

  QMenu mainMenu(this);

  mainMenu.addAction(newUrl_);

  if(index.isValid()){

	mainMenu.addAction(editUrl_);
	mainMenu.addAction(deleteUrl_);
	mainMenu.addSeparator();
	mainMenu.addAction(openUrl_);
  }

  mainMenu.exec(ui->urlTableView->mapToGlobal(pos));

}

void PublicUrlDialog::writeSettings(){

  QSettings settings(qApp->organizationName(), SW::Helper_t::appName());
  settings.beginGroup("Public_url_dialog");

  settings.setValue("form_geometry", this->saveGeometry());
  settings.setValue("Header_state", ui->urlTableView->horizontalHeader()->saveState());

  if(ui->categoryComboBox->count() > 1){

	settings.setValue("lastCategory", ui->categoryComboBox->currentText());
  }
  settings.endGroup();

}

void PublicUrlDialog::readSettings(){

  QSettings settings(qApp->organizationName(), SW::Helper_t::appName());

  settings.beginGroup("Public_url_dialog");
  const auto formGeometry = settings.value("form_geometry", QByteArray()).toByteArray();
  const auto headerState = settings.value("Header_state", QByteArray()).toByteArray();
  const auto lastCategory = settings.value("lastCategory", QString()).toString();
  settings.endGroup();

  this->restoreGeometry(formGeometry);

  if(!lastCategory.isEmpty() && ui->categoryComboBox->count() > 1){

	auto foundIndex = ui->categoryComboBox->findText(lastCategory);
	if(foundIndex != -1){

	  QSignalBlocker blocker(ui->categoryComboBox);
	  ui->categoryComboBox->setCurrentIndex(foundIndex);
	}
  }

  on_loadDataTableView();

  if (!headerState.isEmpty()) {
	ui->urlTableView->horizontalHeader()->restoreState(headerState);
  }

}

void PublicUrlDialog::setupContextMenu(){

  newUrl_ = new QAction("Agregar nueva url", this);
  editUrl_ = new QAction("Editar datos de la url", this);
  deleteUrl_ = new QAction("Eliminar url", this);
  openUrl_ = new QAction("Abrir url en el navegador", this);


  connect(newUrl_, &QAction::triggered, this, [this](){on_showMaintenanceDialog(SW::OpenMode::New);});
  connect(editUrl_, &QAction::triggered, this, [this](){on_showMaintenanceDialog(SW::OpenMode::Edit);});
  connect(deleteUrl_, &QAction::triggered, this, &PublicUrlDialog::on_deleteUrl);
  connect(openUrl_, &QAction::triggered, this, &PublicUrlDialog::on_openUrl);

}

void PublicUrlDialog::on_openUrl(){

  if(!ui->urlTableView->selectionModel()->hasSelection()){
	QMessageBox::warning(this, SW::Helper_t::appName(), QStringLiteral("Seleccione una fila!\n"));
	return;
  }


  const auto row_ = ui->urlTableView->currentIndex().row();
  const auto url_ = ui->urlTableView->model()->index(row_, 1).data().toString();

  if(!QDesktopServices::openUrl(QUrl(url_))){
	QMessageBox::critical(this, SW::Helper_t::appName(), QStringLiteral("Error al abrir la dirección url.\n"));
	return;
  }

}

void PublicUrlDialog::closeEvent(QCloseEvent *event){

  writeSettings();
  QDialog::closeEvent(event);
}

void PublicUrlDialog::showEvent(QShowEvent *event){

  QDialog::showEvent(event);
  auto headerState = SW::Helper_t::nativeRegistryKeyExists("Public_url_dialog/Header_state");

  if(!headerState){
	int availableWidth = ui->urlTableView->viewport()->width();
	auto headerWidth = availableWidth/2;
	ui->urlTableView->setColumnWidth(1, headerWidth);
	ui->urlTableView->setColumnWidth(2, headerWidth);
  }

}

void PublicUrlDialog::applyIcons(Qt::ColorScheme scheme) noexcept {

  const auto iconColor = SW::Helper_t::currentIconColor(scheme);

  if(newUrl_) newUrl_->setIcon(SW::Helper_t::svgIcon(":/img/link-new.svg", iconColor));
  if(editUrl_) editUrl_->setIcon(SW::Helper_t::svgIcon(":/img/link-edit.svg", iconColor));
  if(deleteUrl_) deleteUrl_->setIcon(SW::Helper_t::svgIcon(":/img/link-delete.svg", iconColor));
  if(openUrl_) openUrl_->setIcon(SW::Helper_t::svgIcon(":/img/link-open.svg", iconColor));

  ui->newToolButton->setIcon(SW::Helper_t::svgIcon(":/img/link-new.svg", iconColor));
  ui->editToolButton->setIcon(SW::Helper_t::svgIcon(":/img/link-edit.svg", iconColor));
  ui->quitarToolButton->setIcon(SW::Helper_t::svgIcon(":/img/link-delete.svg", iconColor));
  ui->openPushButton->setIcon(SW::Helper_t::svgIcon(":/img/link-open.svg", iconColor));

}
