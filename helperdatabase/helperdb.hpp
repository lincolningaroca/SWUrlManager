#pragma once

#include "util/helper.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <functional>
#include <optional>

class QStringView;
namespace SW {

struct UrlImportData {
  QString url;
  QString description;
};

// En helperdb.hpp, antes del struct
enum class DeleteUrlMode : uint8_t {
  ByCategory = 1,
  ByUrlId    = 2
};


enum class DuplicateAction {
  Omit,
  Replace
};

struct HelperDataBase_t{

  explicit HelperDataBase_t();
  explicit HelperDataBase_t(QSqlDatabase db) noexcept;

  HelperDataBase_t(const HelperDataBase_t&) = delete;
  HelperDataBase_t(HelperDataBase_t&&) = delete;
  HelperDataBase_t& operator=(const HelperDataBase_t&) = delete;
  HelperDataBase_t& operator=(HelperDataBase_t&&) = delete;


  bool importUrlsBatch(uint32_t categoryId,
					   const QList<UrlImportData>& items,
					   DuplicateAction action,
					   int* insertedCount = nullptr,
					   int* updatedCount = nullptr,
					   const std::function<void (int, int)> &onProgress = nullptr) noexcept;

  [[nodiscard]] QJsonArray exportUserCategories(uint32_t userId) noexcept;

  bool restoreUserCategories(uint32_t targetUserId, const QJsonArray& categories,
							 DuplicateAction action, QString* errorOut = nullptr) noexcept;

  static bool ensureDatabaseAndSchemaReady(DbConfig& config, QWidget* parent = nullptr);
  /**
	 * @brief Encuentra la ruta absoluta de ejecutable de Postgres (pg_dump, pg_restore)
	 * dinámicamente mediante Registro de Windows, PATH o escaneo de directorios.
	 */
  static QString getPostgresToolPath(const QString& toolName, bool* found = nullptr);
  // helperdb.hpp — en la sección public
  [[nodiscard]] QString encryptionKey() const noexcept { return encryptionKey_; }
  bool userExists(QStringView user) noexcept;
  bool userExists() noexcept;
  bool categoryExists(QStringView category, uint32_t userId) noexcept;
  bool urlExists(QStringView url, uint32_t categoryid) noexcept;
  bool createUser(QStringView, QStringView, QStringView user_prof,
				  QStringView rescue_type, QStringView val1, QStringView val2) noexcept;
  bool logIn(QStringView user, QStringView password) noexcept;
  ///////////******************///////////////////////////////////////////////////////////////////////////////////////
  //funciones del formulario principal
  bool saveCategoryData(QStringView catName, QStringView desc, uint32_t userid) noexcept;
  bool updateCategory( QStringView url,  QStringView desc, uint32_t category_id, uint32_t user_id) noexcept;
  bool saveData_url(QStringView url, QStringView desc, std::uint32_t id) noexcept;
  bool updateData_url(QStringView url, QStringView desc, std::uint32_t id, std::uint32_t categoryId) noexcept;
  bool deleteUrls(DeleteUrlMode op, uint32_t categoryId=0, uint32_t urlId=0) noexcept;
  bool deleteCategory(uint32_t categoryId) noexcept;
  bool validateAnswer(QStringView respuesta, uint32_t userId) noexcept;
  bool resetPassword(QStringView password, uint32_t userId) noexcept;
  bool moveUrlToOtherCategory(uint32_t categoryId, uint32_t urlId) noexcept;

  bool isDataBase_empty() noexcept;

  QList<QPair<uint32_t, QString>> loadList_Category(uint32_t user_id) noexcept;
  int getUser_id(const QString &user, SW::User user_profile) noexcept;
  QStringList dataCategory(uint32_t category_id) noexcept;
  QString validateRescueType(uint32_t userId) noexcept;
  QString getQuestion(uint32_t userId) noexcept;

  // Devuelve nullopt si la consulta falla o el usuario no existe.
  [[nodiscard]] std::optional<SW::SessionPermissions> getUserPermissions(const QString& username) noexcept;

  const QString& errorMessage() const noexcept {return errorMessage_;}

private:
  QSqlDatabase db_{};
  QString errorMessage_{};
  QSqlQuery qry_{};

  QString encryptionKey_{};


};

} // namespace SW