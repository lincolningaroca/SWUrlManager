#pragma once

#include "helperdatabase/helperdb.hpp"
#include "util/dataimporterexporter.hpp"
#include "util/helper.hpp"
#include "util/urlimportworker.hpp"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QMainWindow>
#include <QMimeData>
#include <QPointer>
#include <QProgressDialog>
#include <QSqlDatabase>
#include <QThread>
#include <QUrl>

class QAction;
class QLabel;
class QMenu;
class MidleWidget;
struct SWTableModel;

QT_BEGIN_NAMESPACE
namespace Ui { class MainForm; }
QT_END_NAMESPACE

class MainForm : public QMainWindow
{
  Q_OBJECT

public:
  MainForm(QWidget *parent = nullptr);
  ~MainForm();

  void setEncryptionKey(const QByteArray& dek) noexcept { helperdb_.setEncryptionKeyRaw(dek); }

private:
  Ui::MainForm *ui;

  MidleWidget *midleWidget{nullptr};
  QString defaultStyleName_{};

  QHash<SW::OpenMode, QString> openMode{
	{SW::OpenMode::New, "Agregar"},
	{SW::OpenMode::Edit, "Actualizar"}
  };
  //metodos privados de la aplicacion
private:

  int warningMessage(QWidget* parent, const QString& tittle, const QString& Msg);

  void loadListCategory(uint32_t user_id) noexcept;
  void applyPreferredTheme(Qt::ColorScheme scheme);
  void has_data() noexcept;
  void hastvUrlData() noexcept;
  void checkStatusContextMenu();
  void canRestoreDataBase() const noexcept;
  void verifyUserState();
  void initFrm() noexcept;
  void setUpTable(uint32_t categoryId = 1) noexcept;
  void setUpTableHeaders() const noexcept;
  void canCreateBackUp() const noexcept;
  void canStartSession() noexcept;

  void setUptvUrlContextMenu() noexcept;
  void setUpMainContextMenu() noexcept;
  void openUrl() noexcept;
  void readSettings() noexcept;
  void setCboCategoryToolTip() noexcept;
  void editAction(bool op) noexcept;
  void writeSettings() const noexcept;
  void setUpShowMenuAction();
  void setUpStatusBar();

  void writeUserPreferences() const;
  void readUserPreferences();

  static bool isSupportedImportFile(const QString& filePath) {

	const QFileInfo info(filePath);
	const QString ext = info.suffix().toLower();
	return (ext == QStringLiteral("xlsx") ||
			ext == QStringLiteral("csv")  ||
			ext == QStringLiteral("tsv")  ||
			ext == QStringLiteral("txt"));
  }

  void processImportFile(const QString& filePath);
  void exportData(SW::DataImporterExporter::ExportFormat format);

  void startImportWorker(const QString& filePath);
  Q_INVOKABLE int resolveDuplicatesDialog(const QStringList& duplicates);

  void updateLblInfo() noexcept;

  void applyIcons(Qt::ColorScheme scheme) noexcept;

  bool hasValidTableData() const noexcept;
  bool hasValidUserTableData(int userId) const noexcept;
  bool validateSelectedRow() noexcept;
  bool deleteAll() noexcept;

  QColor getEnfasisColor() const;

  uint32_t currentCategoryId() const noexcept;

  //variables y estructuras privadas de la aplicacion

  const QSqlDatabase db_{};

  QAction* openUrl_{ nullptr };
  QAction* editUrl_{ nullptr };
  QAction* quitUrl_{ nullptr };
  QAction* showDescDetail_{ nullptr };
  QAction* showPublicUrl_{ nullptr };
  QAction* moveUrl_{ nullptr };
  QAction* importFromFile_{nullptr};


  QMenu* exportMenu_{nullptr};
  QAction* exportToXlsx_{nullptr};
  QAction* exportToCsv_{nullptr};
  QAction* exportToTsv_{nullptr};
  QAction* exportToTxt_{nullptr};

  const QHash<Qt::ColorScheme, QString> themeType_{
	{Qt::ColorScheme::Unknown, "Predeterminado del sistema"},
	{Qt::ColorScheme::Light, "Modo Claro"},
	{Qt::ColorScheme::Dark, "Modo Oscuro"}
  };

  SW::HelperDataBase_t helperdb_{};
  inline static int userId_{0};
  SWTableModel* xxxModel_{ nullptr };

  SW::SessionPermissions sessionPerms_{};

  QLabel *lblIcon_{nullptr};
  QLabel *lblState_{nullptr};
  QLabel *lblInfo_{nullptr};
  Qt::ColorScheme currentScheme_{Qt::ColorScheme::Unknown};

  QThread* importThread_{nullptr};
  QPointer<QProgressDialog> importProgressDialog_;

private slots:

  void on_showAboutDialog();
  void on_showNewCategoryDialog();
  void on_loadLoginForm();
  void on_showResetPasswordDialog();
  void on_deleteCategory();
  void on_addNewUrl();
  void on_editCategory();
  void on_quitUrl();
  void on_btnEdit();
  void on_categorySelectedChanged(int index);
  void on_callLogout();
  void on_makeBackup();
  void on_restoreDatabase();
  void on_cancelAction();
  void on_showAllDescription();
  void on_showPublicUrlDialog();
  void on_moveUrl();
  void on_firstTimeLoginDialog();
  void on_showSettingsDialog();
  void on_showChangePasswordDialog();
  void on_showTableContextMenu(const QPoint &p);
  void on_showMainContextMenu(const QPoint &p);
  void on_styleChanged(bool style);
  void onImportFromExcelFileTriggered();

  void on_makeUserBackup();
  void on_restoreUserBackup();


  // QWidget interface
protected:
  virtual void closeEvent(QCloseEvent *event) override;
  virtual void showEvent(QShowEvent *event) override;
  virtual void changeEvent(QEvent *event) override;

  bool eventFilter(QObject *watched, QEvent *event) override;
};
