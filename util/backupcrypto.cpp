#include "backupcrypto.hpp"

#include <QFile>
#include <QJsonParseError>

extern "C"{
#include <openssl/evp.h>
#include <openssl/rand.h>
}

namespace SW {

const QByteArray BackupCrypto::kMagic = QByteArrayLiteral("SWBK");

std::optional<QByteArray> BackupCrypto::deriveKey(const QString& password, const QByteArray& salt) {
  if (password.isEmpty() || salt.size() != kSaltBytes) return std::nullopt;

  const QByteArray pwd = password.toUtf8();
  QByteArray key(kKeyBytes, 0);

  const int ok = PKCS5_PBKDF2_HMAC(
	pwd.constData(), static_cast<int>(pwd.size()),
	reinterpret_cast<const unsigned char*>(salt.constData()), static_cast<int>(salt.size()),
	static_cast<int>(kIterations), EVP_sha256(), kKeyBytes,
	reinterpret_cast<unsigned char*>(key.data()));

  if (ok != 1) return std::nullopt;
  return key;
}

bool BackupCrypto::encryptToFile(const QJsonDocument& doc, const QString& filePath,
								 const QString& password, QString* errorOut) {

  QByteArray salt(kSaltBytes, 0), nonce(kNonceBytes, 0);
  if (RAND_bytes(reinterpret_cast<unsigned char*>(salt.data()), kSaltBytes) != 1 ||
	  RAND_bytes(reinterpret_cast<unsigned char*>(nonce.data()), kNonceBytes) != 1) {
	if (errorOut) *errorOut = QStringLiteral("No se pudo generar material aleatorio para el cifrado.");
	return false;
  }

  auto keyOpt = deriveKey(password, salt);
  if (!keyOpt) {
	if (errorOut) *errorOut = QStringLiteral("No se pudo derivar la clave de cifrado.");
	return false;
  }
  const QByteArray& key = *keyOpt;

  const QByteArray plain = doc.toJson(QJsonDocument::Compact);
  QByteArray cipherText(plain.size(), 0);
  QByteArray tag(kTagBytes, 0);

  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  if (!ctx) { if (errorOut) *errorOut = QStringLiteral("Fallo interno de OpenSSL."); return false; }

  bool ok = true;
  int outLen = 0;
  ok &= EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr,
						   reinterpret_cast<const unsigned char*>(key.constData()),
						   reinterpret_cast<const unsigned char*>(nonce.constData())) == 1;
  ok &= EVP_EncryptUpdate(ctx, reinterpret_cast<unsigned char*>(cipherText.data()), &outLen,
						  reinterpret_cast<const unsigned char*>(plain.constData()), static_cast<int>(plain.size())) == 1;
  int finalLen = 0;
  ok &= EVP_EncryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(cipherText.data()) + outLen, &finalLen) == 1;
  ok &= EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, kTagBytes,
							reinterpret_cast<unsigned char*>(tag.data())) == 1;
  EVP_CIPHER_CTX_free(ctx);

  if (!ok) {
	if (errorOut) *errorOut = QStringLiteral("Error al cifrar los datos del backup.");
	return false;
  }

  QFile file(filePath);
  if (!file.open(QIODevice::WriteOnly)) {
	if (errorOut) *errorOut = QStringLiteral("No se pudo crear el archivo: %1").arg(file.errorString());
	return false;
  }

  file.write(kMagic);
  file.write(reinterpret_cast<const char*>(&kSchemaVersion), 1);
  file.write(salt);
  file.write(nonce);
  file.write(tag);
  file.write(cipherText);
  file.close();

  return true;
}

std::optional<QJsonDocument> BackupCrypto::decryptFromFile(const QString& filePath,
														   const QString& password, QString* errorOut) {

  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly)) {
	if (errorOut) *errorOut = QStringLiteral("No se pudo abrir el archivo: %1").arg(file.errorString());
	return std::nullopt;
  }

  const QByteArray raw = file.readAll();
  file.close();

  const int headerSize = static_cast<int>(kMagic.size()) + 1 + kSaltBytes + kNonceBytes + kTagBytes;
  if (raw.size() <= headerSize || !raw.startsWith(kMagic)) {
	if (errorOut) *errorOut = QStringLiteral("El archivo no es un backup válido.");
	return std::nullopt;
  }

  int pos = static_cast<int>(kMagic.size());
  const quint8 version = static_cast<quint8>(raw[pos]); pos += 1;
  if (version != kSchemaVersion) {
	if (errorOut) *errorOut = QStringLiteral("Versión de backup no soportada.");
	return std::nullopt;
  }

  const QByteArray salt  = raw.mid(pos, kSaltBytes);  pos += kSaltBytes;
  const QByteArray nonce = raw.mid(pos, kNonceBytes); pos += kNonceBytes;
  const QByteArray tag   = raw.mid(pos, kTagBytes);   pos += kTagBytes;
  const QByteArray cipherText = raw.mid(pos);

  auto keyOpt = deriveKey(password, salt);
  if (!keyOpt) {
	if (errorOut) *errorOut = QStringLiteral("No se pudo derivar la clave de descifrado.");
	return std::nullopt;
  }
  const QByteArray& key = *keyOpt;

  QByteArray plain(cipherText.size(), 0);
  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  if (!ctx) { if (errorOut) *errorOut = QStringLiteral("Fallo interno de OpenSSL."); return std::nullopt; }

  EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr,
					 reinterpret_cast<const unsigned char*>(key.constData()),
					 reinterpret_cast<const unsigned char*>(nonce.constData()));
  EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, kTagBytes, const_cast<char*>(tag.constData()));

  int outLen = 0;
  EVP_DecryptUpdate(ctx, reinterpret_cast<unsigned char*>(plain.data()), &outLen,
					reinterpret_cast<const unsigned char*>(cipherText.constData()), static_cast<int>(cipherText.size()));
  int finalLen = 0;
  const int valid = EVP_DecryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(plain.data()) + outLen, &finalLen);
  EVP_CIPHER_CTX_free(ctx);

  if (valid != 1) {
	if (errorOut) *errorOut = QStringLiteral("Contraseña incorrecta o archivo dañado.");
	return std::nullopt;
  }

  QJsonParseError parseError;
  const auto doc = QJsonDocument::fromJson(plain, &parseError);
  if (parseError.error != QJsonParseError::NoError) {
	if (errorOut) *errorOut = QStringLiteral("El contenido del backup está corrupto.");
	return std::nullopt;
  }

  return doc;
}

} // namespace SW