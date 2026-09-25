#include "acercadedialog.hpp"
#include "ui_acercadedialog.h"

#include "util/helper.hpp"

#include <QFile>
#include <QMessageBox>
#include <QSettings>

AcercaDeDialog::AcercaDeDialog(Qt::ColorScheme colorMode, QWidget *parent)
  : QDialog(parent),
  ui(new Ui::AcercaDeDialog),
  colorMode_(colorMode),
  customFont_()
{

  ui->setupUi(this);
  setWindowFlags(windowFlags() | Qt::MSWindowsFixedSizeDialogHint);
  setupUI();

  const auto scheme = (colorMode_ == Qt::ColorScheme::Unknown)
						? SW::Helper_t::detectSystemColorScheme()
						: colorMode_;

  setImage(scheme);
  setupCustomFont();  
  readSettings();

  setupUiConnections();

}

AcercaDeDialog::~AcercaDeDialog()
{
  delete ui;
}

void AcercaDeDialog::writeSettings() const{

  QSettings settings(qApp->organizationName(), SW::Helper_t::appName());
  settings.beginGroup("abaut_dialog");
  settings.setValue("form_geometry", this->pos());
  settings.endGroup();

}

void AcercaDeDialog::readSettings()
{
  QSettings settings(qApp->organizationName(), SW::Helper_t::appName());
  settings.beginGroup("abaut_dialog");
  const auto pos = settings.value("form_geometry").toPoint();
  this->move(pos);
  settings.endGroup();
}


void AcercaDeDialog::loadInfo_app() const noexcept{

  ui->tbLicencia->setFont(customFont_);
  ui->tbLicencia->setAcceptRichText(true);
  ui->tbLicencia->setOpenExternalLinks(true);
  ui->tbLicencia->setHtml(QStringLiteral(
	"<p style='text-align: justify;'>SWUrlManager:<br><br>Es software libre, puede "
	"redistribuirlo y/o modificarlo bajo los términos de la Licencia Pública "
	"General de GNU según se encuentra publicada por la <a "
	"href=\"https://www.fsf.org\">Free Software "
	"Foundation</a>, bien de la versión 3 de dicha Licencia o bien (según su "
	"elección) de cualquier versión posterior.<br><br>"
	"Este programa se distribuye con la esperanza de que sea útil, pero <strong>SIN "
	"NINGUNA "
	"GARANTÍA</strong>, incluso sin la garantía <strong>MERCANTIL</strong> implícita ni la de "
	"garantizar la <strong>ADECUACIÓN A UN PROPÓSITO PARTICULAR.</strong> Véase la <a "
	"href=\"https://www.gnu.org/licenses/\">Licencia "
	"Pública General</a> de GNU para más detalles.</p>"));

}

void AcercaDeDialog::setTextToAbout() const{

  ui->tbAcercaDe->setFont(customFont_);
  ui->tbAcercaDe->setOpenExternalLinks(true);
  ui->tbAcercaDe->setHtml(QStringLiteral(
	"<p>Powered by:"
	"<ul>"
	"<li>Lincoln Ingaroca De La Cruz.</li>"
	"<li>SWSystem's.</li>"
	"</ul>"
	"Contacto:"
	"<ul>"
	"<li>lincolningaroca@gmail.com</li>"
	"</ul>"
	"Lincoln Ingaroca:"
	"<ul>"
	"<li>Analista de sistemas informáticos.</li>"
	"<li>Software development.</li>"
	"</ul><br>"
	"Bibliotecas:"
	"<p>SWUrlManager incluye código fuente de los siguientes proyectos:</p>"
	"<ul>"
	"<li><a href=\"https://www.openssl.org/\">OpenSSL.</a></li>"
	"<li><a href=\"https://www.qt.io//\">QtFrameWork and QtWidgets.</a></li>"
	"<li><a href=\"https://www.sqlite.org/index.html\">SQLite.</a></li>"
	"<li><a href=\"https://github.com/QtExcel/QXlsx\">QXlsx library.</a></li>"
	"<li><a href=\"https://www.postgresql.org/\">PostgreSQL.</a></li>"
	"</ul>"
	"</p>"
	"<p>Repositorio del programa:"
	"<ul><li><a href=\"https://github.com/lincolningaroca/SWUrlManager\">SWUrlManager</a></li></ul>"
	"</p>"));

}

void AcercaDeDialog::setImage(Qt::ColorScheme colorMode) {
  const bool isDark = (colorMode == Qt::ColorScheme::Dark);
  const QColor logoColor = isDark ? QColor(220, 220, 220) : QColor(30, 30, 30);

  const QPixmap logoPix = SW::Helper_t::svgIcon(
							":/img/logoSWSystems.svg",
							logoColor,
							QSize(400, 400)  // ajusta al tamaño del label
							).pixmap(QSize(400, 400));

  ui->lblLogo->setPixmap(logoPix);
  ui->lblLogo->setAlignment(Qt::AlignCenter);
}

void AcercaDeDialog::setupCustomFont() {

  customFont_ = SW::Helper_t::monospaceFont(9);

  ui->tbAcercaDe->setFont(customFont_);
  ui->tbLicencia->setFont(customFont_);
}


void AcercaDeDialog::setupUI(){

  ui->tabWidget->setCurrentIndex(0);

  setTextToAbout();
  loadInfo_app();

  // Configurar links
  ui->lblLicencia->setText(QStringLiteral("<a href='license'>Ver licencia</a>"));
  ui->lblAcercaQt->setText(QStringLiteral("<a href='qt'>Acerca de Qt</a>"));

}

void AcercaDeDialog::setupUiConnections(){

  connect(ui->btnCerrar, &QPushButton::clicked, this, &AcercaDeDialog::close);
  connect(ui->lblLicencia, &QLabel::linkActivated, this, &AcercaDeDialog::showLicense);
  connect(ui->lblAcercaQt, &QLabel::linkActivated, this, [this]() {
	QMessageBox::aboutQt(this, SW::Helper_t::appName());
  });

}

void AcercaDeDialog::showLicense(){

  QDialog licenciaDlg(this);

  licenciaDlg.setFixedSize(this->size());
  licenciaDlg.setWindowTitle(SW::Helper_t::appName() + " - Licencia");

  auto* teLicencia = new QTextBrowser(&licenciaDlg);

  teLicencia->setFont(customFont_);
  teLicencia->setAcceptRichText(true);
  teLicencia->setOpenExternalLinks(true);
  teLicencia->setReadOnly(true);

  QFile fileName(QStringLiteral(":/licencia/gnu-gpl-v3-license.html"));
  if (!fileName.open(QFile::ReadOnly | QFile::Text)) {
	QMessageBox::warning(this, SW::Helper_t::appName(),
						 tr("Error al abrir el archivo de licencia:\n%1")
						   .arg(fileName.errorString()));
	return;
  }

  teLicencia->setHtml(fileName.readAll());
  fileName.close();

  auto* mainLayout = new QVBoxLayout(&licenciaDlg);
  mainLayout->addWidget(teLicencia);
  mainLayout->setContentsMargins(5,5,5,5);

  licenciaDlg.exec();

}

void AcercaDeDialog::closeEvent(QCloseEvent *event){

  writeSettings();
  QDialog::closeEvent(event);

}
