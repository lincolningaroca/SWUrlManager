#pragma once

#include <QByteArray>
#include <QJsonDocument>
#include <QString>
#include <optional>

namespace SW {

struct BackupCrypto {

  explicit BackupCrypto() = delete;

  // Cifra el JSON con AES-256-GCM, clave derivada por PBKDF2 de la contraseña
  // dada. Salt/nonce se generan al azar y quedan en el propio archivo.
  [[nodiscard]] static bool encryptToFile(const QJsonDocument& doc,
										  const QString& filePath,
										  const QString& password,
										  QString* errorOut = nullptr);

  // Descifra un archivo .swbak. Devuelve std::nullopt si la contraseña es
  // incorrecta, el archivo está corrupto, o no se pudo abrir.
  [[nodiscard]] static std::optional<QJsonDocument> decryptFromFile(const QString& filePath,
																	const QString& password,
																	QString* errorOut = nullptr);

private:
  static constexpr quint32 kIterations = 600'000;
  static constexpr int     kSaltBytes  = 16;
  static constexpr int     kKeyBytes   = 32;
  static constexpr int     kNonceBytes = 12;
  static constexpr int     kTagBytes   = 16;
  static constexpr quint8  kSchemaVersion = 1;
  static const QByteArray  kMagic;

  [[nodiscard]] static std::optional<QByteArray> deriveKey(const QString& password, const QByteArray& salt);
};

} // namespace SW