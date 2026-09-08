#include "reportbugdialog.hpp"

#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFormLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QScreen>
#include <QStandardPaths>
#include <QSizePolicy>
#include <QSysInfo>
#include <QTabWidget>
#include <QUrl>
#include <QUrlQuery>
#include <QVBoxLayout>
#include <QComboBox>
#include <QPushButton>
#include <QTextEdit>

namespace {
constexpr int kMaxUrlBodyLength = 6000; // margen de seguridad bajo el límite práctico de URL de la mayoría de navegadores
const auto kGitHubIssueUrl = QStringLiteral("https://github.com/lincolningaroca/SWUrlManager/issues/new");
}

ReportBugDialog::ReportBugDialog(QWidget *parent)
  : QDialog(parent)
{
  setWindowFlags(windowFlags() | Qt::MSWindowsFixedSizeDialogHint);
  setWindowTitle(QStringLiteral("Reportar un error"));
  setMinimumWidth(580);
  setupUi();
  setupConnections();
  collectSystemInfo();
  adjustSize();
}

ReportBugDialog::~ReportBugDialog() = default;

void ReportBugDialog::setupUi()
{
  auto *mainLayout = new QVBoxLayout(this);

  // --- Encabezado ---
  auto *header = new QLabel(
	QStringLiteral("<h3>🐛 Reportar un problema o bug</h3>"
				   "<p>Describe el problema con el mayor detalle posible. "
				   "El reporte se abrirá como un Issue en GitHub, listo para enviar.</p>"),
	this);
  header->setWordWrap(true);
  mainLayout->addWidget(header);

  auto *tabs = new QTabWidget(this);

  // ==== TAB 1: Formulario ====
  auto *formPage = new QWidget(this);
  auto *formLayout = new QFormLayout(formPage);
  formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

  m_subjectEdit = new QLineEdit(this);
  m_subjectEdit->setPlaceholderText(QStringLiteral("Ej: La aplicación se cierra al exportar a CSV"));
  formLayout->addRow(QStringLiteral("*Asunto:"), m_subjectEdit);

  m_severityCombo = new QComboBox(this);
  m_severityCombo->addItems({
							 QStringLiteral("Bajo (cosmético / menor)"),
							 QStringLiteral("Medio (afecta el uso pero hay workaround)"),
							 QStringLiteral("Alto (funcionalidad rota)"),
							 QStringLiteral("Crítico (pérdida de datos / crash)")});

  m_severityCombo->setCurrentIndex(1);
  formLayout->addRow(QStringLiteral("Severidad:"), m_severityCombo);

  m_descriptionEdit = new QTextEdit(this);
  m_descriptionEdit->setPlaceholderText(
	QStringLiteral("Describe qué ocurrió, qué esperabas que ocurriera, "
				   "los pasos para reproducirlo, y cualquier detalle relevante..."));
  m_descriptionEdit->setFixedHeight(160);
  formLayout->addRow(QStringLiteral("*Descripción:"), m_descriptionEdit);

  auto *attachNote = new QLabel(
	QStringLiteral("<i>💡 Si desea adjuntar capturas de pantalla, GIFs o videos del problema, "
				   "puede arrastrarlos directamente dentro del cuadro de texto del Issue "
				   "una vez que se abra en GitHub.</i>"),
	this);
  attachNote->setWordWrap(true);
  formLayout->addRow(attachNote);

  tabs->addTab(formPage, QStringLiteral("📝 Reporte"));

  // ==== TAB 2: Info del sistema ====
  auto *infoPage = new QWidget(this);
  auto *infoLayout = new QVBoxLayout(infoPage);
  auto *infoNote = new QLabel(
	QStringLiteral("<i>Esta información se incluirá en tu reporte para facilitar el diagnóstico. "
				   "Puede editarla o borrar cualquier dato que prefieras no compartir antes de enviar.</i>"),
	infoPage);
  infoNote->setWordWrap(true);
  infoLayout->addWidget(infoNote);

  m_systemInfoEdit = new QTextEdit(infoPage);
  m_systemInfoEdit->setReadOnly(false);
  m_systemInfoEdit->setFont(QFont(QStringLiteral("Consolas"), 9));
  m_systemInfoEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  infoLayout->addWidget(m_systemInfoEdit);

  tabs->addTab(infoPage, QStringLiteral("💻 Info del sistema"));

  mainLayout->addWidget(tabs);

  // --- Botones de acción ---
  auto *actionLayout = new QHBoxLayout();
  m_btnReport = new QPushButton(QStringLiteral("📤 Reportar en GitHub"), this);
  auto *btnClose = new QPushButton(QStringLiteral("Cerrar"), this);

  actionLayout->addWidget(m_btnReport);
  actionLayout->addStretch();
  actionLayout->addWidget(btnClose);
  mainLayout->addLayout(actionLayout);

  connect(btnClose, &QPushButton::clicked, this, &QDialog::reject);
}

void ReportBugDialog::setupConnections()
{
  connect(m_btnReport, &QPushButton::clicked, this, &ReportBugDialog::on_reportOnGitHub);
}

void ReportBugDialog::collectSystemInfo()
{
  m_appVersion = QStringLiteral("%1 (build %2)")
  .arg(QApplication::applicationVersion(),
	   QStringLiteral(__DATE__ " " __TIME__));

  QStringList lines;
  lines << QStringLiteral("=== Información del sistema ===")
		<< QStringLiteral("Fecha del reporte : %1")
			 .arg(QDateTime::currentDateTime().toString(Qt::ISODate))
		<< QStringLiteral("Aplicación        : %1").arg(QApplication::applicationName())
		<< QStringLiteral("Versión           : %1").arg(m_appVersion)
		<< QStringLiteral("Organización      : %1").arg(QApplication::organizationName())
		<< QStringLiteral("")
		<< QStringLiteral("--- Plataforma ---")
		<< QStringLiteral("SO                : %1").arg(QSysInfo::prettyProductName())
		<< QStringLiteral("Arquitectura      : %1").arg(QSysInfo::currentCpuArchitecture())
		<< QStringLiteral("Kernel            : %1").arg(QSysInfo::kernelType() + " " + QSysInfo::kernelVersion())
		<< QStringLiteral("")
		<< QStringLiteral("--- Qt ---")
		<< QStringLiteral("Qt compile-time   : %1").arg(QStringLiteral(QT_VERSION_STR))
		<< QStringLiteral("Qt run-time       : %1").arg(qVersion())
		<< QStringLiteral("Platform plugin   : %1")
			 .arg(QGuiApplication::platformName())
		<< QStringLiteral("Style             : %1").arg(qApp->style()->objectName())
		<< QStringLiteral("")
		<< QStringLiteral("--- Pantalla ---");

  const auto screens = QGuiApplication::screens();
  for (int i = 0; i < screens.size(); ++i) {
	const auto *s = screens.at(i);
	const auto g = s->geometry();
	lines << QStringLiteral("  Monitor %1: %2x%3 @ %4% (DPI lógico %5)")
			   .arg(i).arg(g.width()).arg(g.height())
			   .arg(int(s->logicalDotsPerInch()))
			   .arg(s->devicePixelRatio());
  }

  lines << ""
		<< "--- Base de datos ---"
		<< QStringLiteral("Driver SQL        : %1").arg(QStringLiteral("QPSQL"))
		<< ""
		<< "--- Directorios ---"
		<< QStringLiteral("Config            : %1")
			 .arg(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation))
		<< QStringLiteral("Datos             : %1")
			 .arg(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
		<< QStringLiteral("Temp              : %1")
			 .arg(QDir::tempPath());

  m_systemInfo = lines.join('\n');
  m_systemInfoEdit->setPlainText(m_systemInfo);
}

QString ReportBugDialog::severityLabel(int idx) const
{
  switch (idx) {
	case 0: return QStringLiteral("Bajo");
	case 1: return QStringLiteral("Medio");
	case 2: return QStringLiteral("Alto");
	case 3: return QStringLiteral("Crítico");
	default: return QStringLiteral("Desconocido");
  }
}

QString ReportBugDialog::buildInfoText() const
{
  QString out;
  out += QStringLiteral("ASUNTO: %1\n").arg(m_subjectEdit->text());
  out += QStringLiteral("SEVERIDAD: %1\n")
		   .arg(severityLabel(m_severityCombo->currentIndex()));
  out += QStringLiteral("\n--- DESCRIPCIÓN ---\n%1\n")
		   .arg(m_descriptionEdit->toPlainText());
  out += QStringLiteral("\n%1\n").arg(m_systemInfoEdit->toPlainText());
  return out;
}

bool ReportBugDialog::validateFields()
{
  if (m_subjectEdit->text().trimmed().isEmpty()) {
	QMessageBox::warning(this, QStringLiteral("Falta información"),
						 QStringLiteral("Por favor, ingresa un asunto para el reporte."));
	m_subjectEdit->setFocus();
	return false;
  }
  if (m_descriptionEdit->toPlainText().trimmed().isEmpty()) {
	QMessageBox::warning(this, QStringLiteral("Falta información"),
						 QStringLiteral("Por favor, describe el problema."));
	m_descriptionEdit->setFocus();
	return false;
  }
  return true;
}

// ----------------- Envío del reporte -----------------

void ReportBugDialog::on_reportOnGitHub()
{
  if (!validateFields()) return;

  const QString fullText = buildInfoText();

  QGuiApplication::clipboard()->setText(fullText);

  QString bodyForUrl = fullText;
  bool wasTruncated = false;
  if (bodyForUrl.size() > kMaxUrlBodyLength) {
	bodyForUrl = bodyForUrl.left(kMaxUrlBodyLength);
	bodyForUrl += QStringLiteral("\n\n[...] El reporte completo es más largo de lo que permite "
								 "esta URL — ya se copió completo a tu portapapeles, pégalo aquí "
								 "reemplazando este texto.");
	wasTruncated = true;
  }

  QUrl issueUrl(kGitHubIssueUrl);
  QUrlQuery q;
  q.addQueryItem(QStringLiteral("title"),
				 QStringLiteral("[%1] %2").arg(severityLabel(
												 m_severityCombo->currentIndex()), m_subjectEdit->text()));
  q.addQueryItem(QStringLiteral("body"), bodyForUrl);
  issueUrl.setQuery(q);

  if (!QDesktopServices::openUrl(issueUrl)) {
	QMessageBox::warning(this, QStringLiteral("Error"),
						 QStringLiteral("No se pudo abrir el navegador.\n"
										"El reporte ya se copió a tu portapapeles: puedes pegarlo "
										"manualmente en %1").arg(kGitHubIssueUrl));
	return;
  }

  if (wasTruncated) {
	QMessageBox::information(this, QStringLiteral("Un paso más"),
							 QStringLiteral("Se abrió GitHub en tu navegador.\n\n"
											"El reporte era muy largo: ya está completo en tu "
											"portapapeles, pégalo (Ctrl+V) reemplazando el texto truncado."));
  }
}