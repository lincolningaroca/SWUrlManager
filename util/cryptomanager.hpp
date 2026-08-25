#pragma once

#include <QByteArray>
#include <QString>
#include <QSqlDatabase>
#include <optional>

namespace SW {

struct CryptoManager {

  explicit CryptoManager() = delete;

  [[nodiscard]] static std::optional<QByteArray> generateDEK() noexcept;

  [[nodiscard]] static bool storeDEK(const QByteArray& dek,
									 const QString& masterPassword,
									 QSqlDatabase& db) noexcept;

  [[nodiscard]] static std::optional<QByteArray> loadDEK(const QString& masterPassword,
														 QSqlDatabase& db) noexcept;

  [[nodiscard]] static bool hasSecuritySettings(QSqlDatabase& db) noexcept;

  static bool cacheLocalDEK(const QByteArray& dek) noexcept;
  [[nodiscard]] static std::optional<QByteArray> loadCachedLocalDEK() noexcept;
  static void clearCachedLocalDEK() noexcept;

  [[nodiscard]] static bool rotateMasterPassword(const QString& oldPassword,
												 const QString& newPassword,
												 QSqlDatabase& db) noexcept;

private:
  [[nodiscard]] static std::optional<QByteArray> deriveKEK(const QString& password,
														   const QByteArray& salt,
														   quint32 iterations) noexcept;

  static constexpr quint32 kIterations  = 600'000;
  static constexpr int     kSaltBytes   = 16;
  static constexpr int     kKeyBytes    = 32;
  static constexpr int     kNonceBytes  = 12;
  static constexpr int     kTagBytes    = 16;
  static constexpr quint8  kSchemaVersion = 1;
};

} // namespace SW