#include "cryptomanager.hpp"
#include "util/helper.hpp"

#include <QSqlQuery>
#include <QSqlError>
#include <QSettings>
#include <QLoggingCategory>
#include <QCoreApplication>

#include <openssl/evp.h>
#include <openssl/rand.h>

Q_LOGGING_CATEGORY(lcCrypto, "sw.crypto")

namespace SW {

std::optional<QByteArray> CryptoManager::deriveKEK(const QString& password, const QByteArray& salt,
												   quint32 iterations) noexcept {
  if (password.isEmpty() || salt.size() != kSaltBytes) return std::nullopt;

  const QByteArray pwd = password.toUtf8();
  QByteArray kek(kKeyBytes, 0);

  const int ok = PKCS5_PBKDF2_HMAC(
	pwd.constData(), static_cast<int>(pwd.size()),
	reinterpret_cast<const unsigned char*>(salt.constData()), static_cast<int>(salt.size()),
	static_cast<int>(iterations), EVP_sha256(), kKeyBytes,
	reinterpret_cast<unsigned char*>(kek.data()));

  if (ok != 1) return std::nullopt;
  return kek;
}

std::optional<QByteArray> CryptoManager::generateDEK() noexcept {
  QByteArray dek(kKeyBytes, 0);
  if (RAND_bytes(reinterpret_cast<unsigned char*>(dek.data()), kKeyBytes) != 1) {
	qCCritical(lcCrypto) << "RAND_bytes falló generando la DEK";
	return std::nullopt;
  }
  return dek;
}

bool CryptoManager::hasSecuritySettings(QSqlDatabase& db) noexcept {
  QSqlQuery q(db);
  q.prepare(QStringLiteral("SELECT 1 FROM public.security_settings WHERE id = 1"));
  return q.exec() && q.next();
}

bool CryptoManager::storeDEK(const QByteArray& dek, const QString& masterPassword, QSqlDatabase& db) noexcept {

  QByteArray salt(kSaltBytes, 0), nonce(kNonceBytes, 0);
  if (RAND_bytes(reinterpret_cast<unsigned char*>(salt.data()), kSaltBytes) != 1 ||
	  RAND_bytes(reinterpret_cast<unsigned char*>(nonce.data()), kNonceBytes) != 1) {
	qCCritical(lcCrypto) << "RAND_bytes falló generando salt/nonce";
	return false;
  }

  auto kekOpt = deriveKEK(masterPassword, salt, kIterations);
  if (!kekOpt) return false;
  const QByteArray& kek = *kekOpt;

  QByteArray wrappedDek(kKeyBytes, 0);
  QByteArray tag(kTagBytes, 0);

  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  if (!ctx) return false;

  bool ok = true;
  int outLen = 0;
  ok &= EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr,
						   reinterpret_cast<const unsigned char*>(kek.constData()),
						   reinterpret_cast<const unsigned char*>(nonce.constData())) == 1;
  ok &= EVP_EncryptUpdate(ctx, reinterpret_cast<unsigned char*>(wrappedDek.data()), &outLen,
						  reinterpret_cast<const unsigned char*>(dek.constData()), kKeyBytes) == 1;
  int finalLen = 0;
  ok &= EVP_EncryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(wrappedDek.data()) + outLen, &finalLen) == 1;
  ok &= EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, kTagBytes,
							reinterpret_cast<unsigned char*>(tag.data())) == 1;
  EVP_CIPHER_CTX_free(ctx);

  if (!ok) {
	qCCritical(lcCrypto) << "Error cifrando la DEK con AES-256-GCM";
	return false;
  }

  QSqlQuery q(db);
  q.prepare(QStringLiteral(R"(
	INSERT INTO public.security_settings (id, kdf_salt, dek_nonce, dek_tag, wrapped_dek, kdf_iterations, schema_version, updated_at)
	VALUES (1, ?, ?, ?, ?, ?, ?, CURRENT_TIMESTAMP)
	ON CONFLICT (id) DO UPDATE SET
	  kdf_salt = EXCLUDED.kdf_salt,
	  dek_nonce = EXCLUDED.dek_nonce,
	  dek_tag = EXCLUDED.dek_tag,
	  wrapped_dek = EXCLUDED.wrapped_dek,
	  kdf_iterations = EXCLUDED.kdf_iterations,
	  schema_version = EXCLUDED.schema_version,
	  updated_at = CURRENT_TIMESTAMP
  )"));
  q.addBindValue(salt);
  q.addBindValue(nonce);
  q.addBindValue(tag);
  q.addBindValue(wrappedDek);
  q.addBindValue(static_cast<int>(kIterations));
  q.addBindValue(kSchemaVersion);

  if (!q.exec()) {
	qCCritical(lcCrypto) << "Error guardando security_settings:" << q.lastError().text();
	return false;
  }
  return true;
}

std::optional<QByteArray> CryptoManager::loadDEK(const QString& masterPassword, QSqlDatabase& db) noexcept {

  QSqlQuery q(db);
  q.prepare(QStringLiteral(
	"SELECT kdf_salt, dek_nonce, dek_tag, wrapped_dek, kdf_iterations "
	"FROM public.security_settings WHERE id = 1"));

  if (!q.exec() || !q.next()) {
	qCCritical(lcCrypto) << "No se encontró security_settings:" << q.lastError().text();
	return std::nullopt;
  }

  const QByteArray salt       = q.value(0).toByteArray();
  const QByteArray nonce      = q.value(1).toByteArray();
  const QByteArray tag        = q.value(2).toByteArray();
  const QByteArray wrappedDek = q.value(3).toByteArray();
  const quint32    iterations = q.value(4).toUInt();

  auto kekOpt = deriveKEK(masterPassword, salt, iterations);
  if (!kekOpt) return std::nullopt;
  const QByteArray& kek = *kekOpt;

  QByteArray dek(kKeyBytes, 0);
  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  if (!ctx) return std::nullopt;

  EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr,
					 reinterpret_cast<const unsigned char*>(kek.constData()),
					 reinterpret_cast<const unsigned char*>(nonce.constData()));
  EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, kTagBytes, const_cast<char*>(tag.constData()));

  int outLen = 0;
  EVP_DecryptUpdate(ctx, reinterpret_cast<unsigned char*>(dek.data()), &outLen,
					reinterpret_cast<const unsigned char*>(wrappedDek.constData()), kKeyBytes);
  int finalLen = 0;
  const int valid = EVP_DecryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(dek.data()) + outLen, &finalLen);
  EVP_CIPHER_CTX_free(ctx);

  if (valid != 1) {
	qCWarning(lcCrypto) << "Contraseña maestra incorrecta o security_settings corrompida";
	return std::nullopt;
  }
  return dek;
}

bool CryptoManager::cacheLocalDEK(const QByteArray& dek) noexcept {
  const QByteArray protectedBlob = Helper_t::protectLocal(dek);
  if (protectedBlob.isEmpty()) {
	qCWarning(lcCrypto) << "No se pudo cachear la DEK localmente (DPAPI falló)";
	return false;
  }
  QSettings settings(qApp->organizationName(), qApp->applicationName());
  settings.setValue(QStringLiteral("crypto/localCachedDEK"), protectedBlob.toBase64());
  return true;
}

std::optional<QByteArray> CryptoManager::loadCachedLocalDEK() noexcept {
  QSettings settings(qApp->organizationName(), qApp->applicationName());
  const auto b64 = settings.value(QStringLiteral("crypto/localCachedDEK")).toByteArray();
  if (b64.isEmpty()) return std::nullopt;

  const auto dek = Helper_t::unprotectLocal(QByteArray::fromBase64(b64));
  if (dek.size() != kKeyBytes) return std::nullopt;

  return dek;
}

void CryptoManager::clearCachedLocalDEK() noexcept {
  QSettings settings(qApp->organizationName(), qApp->applicationName());
  settings.remove(QStringLiteral("crypto/localCachedDEK"));
}

bool CryptoManager::rotateMasterPassword(const QString& oldPassword, const QString& newPassword, QSqlDatabase& db) noexcept {
  auto dekOpt = loadDEK(oldPassword, db);
  if (!dekOpt) return false;

  if (!storeDEK(*dekOpt, newPassword, db)) return false;

  clearCachedLocalDEK();
  cacheLocalDEK(*dekOpt);

  return true;
}

} // namespace SW