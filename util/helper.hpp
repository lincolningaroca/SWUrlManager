#pragma once

#include "util/sw_types.hpp"

#include <QApplication>
#include <QCryptographicHash>
#include <QDesktopServices>
#include <QMessageLogContext>
#include <QPalette>
#include <QStandardPaths>
#include <QStringView>
#include <QtGlobal>

namespace SW {

/**
 * @brief Manejador personalizado de mensajes para redirigir qInfo, qWarning, etc. a un archivo .log
 */
void customLogHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);

struct Helper_t{

  explicit Helper_t() = delete;

  Helper_t(const Helper_t&) = delete;
  Helper_t(Helper_t&&) = delete;
  Helper_t& operator=(const Helper_t&) = delete;
  Helper_t& operator=(Helper_t&&) = delete;

  static QColor currentIconColor(Qt::ColorScheme scheme) noexcept;
  static QColor currentIconColor() noexcept; // sobrecarga sin parámetro

  /**
   * @brief Verifica si PostgreSQL está instalado en el sistema cliente y obtiene su versión.
   */
  [[nodiscard]] static PgCheckResult checkPostgresqlInstallation() noexcept;

  [[nodiscard]]static bool verify_Values(const QStringView text1, const QStringView text2) noexcept{return (text1.toString() == text2.toString());}
  [[nodiscard]]static bool open_Url(const QUrl& url) noexcept{return QDesktopServices::openUrl(url);}
  [[nodiscard]]static bool urlValidate(QStringView url) noexcept;

  [[nodiscard]]static bool createDataBase_dir() noexcept;

  [[nodiscard]]static QString generateSecurePassword(uint32_t length = 8) noexcept;
  [[nodiscard]]static bool isPasswordSecure(const QString& password) noexcept;

  static void set_Theme(Qt::ColorScheme theme) noexcept;
  [[nodiscard]] static QIcon svgIcon(const QString& resourcePath,
									 const QColor& color) noexcept;


  [[nodiscard]] static QIcon svgIcon(const QString& resourcePath,
									 const QColor& color,
									 const QSize &size) noexcept;


  [[nodiscard]]static Qt::ColorScheme detectSystemColorScheme();

  [[nodiscard]]static QString AppLocalDataLocation(){return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation).append(dbDir_name);}
  [[nodiscard]]static QString app_pathLocation(){return QApplication::applicationDirPath();}
  [[nodiscard]]static QString appName(){return QApplication::applicationName();}

  static void setLastOpenedDirectory(const QString &directory);
  [[nodiscard]]static QString getLastOpenedDirectory();

  [[nodiscard]] static QVariant readData(QByteArray&& data);
  [[nodiscard]]static QByteArray writeData(const QVariant& data);

  static bool nativeRegistryKeyExists(const QString &path);

  [[nodiscard]] static QByteArray protectLocal(const QByteArray& plain) noexcept;
  [[nodiscard]] static QByteArray unprotectLocal(const QByteArray& cipher) noexcept;

  inline static const QHash<SW::User, QString> currentUser_{
	{SW::User::U_public, "PUBLIC"},
	{SW::User::U_user, "USER"}
  };

  inline static const QString defaultUser{QStringLiteral("public")};
  inline static QString current_user_{defaultUser};
  inline static SW::SessionStatus sessionStatus_{SW::SessionStatus::Session_closed};

  inline static QByteArray sessionEncryptionKey_{};

  static void saveDbConfig(const DbConfig& config) noexcept;
  static DbConfig loadDbConfig() noexcept;
  static bool hasDbConfig() noexcept;


private:

  inline static const QString dbDir_name{"/xxxdatabase"};
  inline static const QPalette standardPalette_{QGuiApplication::palette()};

};

} // namespace SW
