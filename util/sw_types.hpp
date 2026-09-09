#pragma once

#include <QString>
#include <cstdint>

namespace SW {

struct DbConfig {
  QString host{"localhost"};
  QString dbName{"xdatabase"};
  QString userName{"postgres"};
  QString password{};
  int     port{5432};
};

struct PgCheckResult {
  bool isInstalled{false};
  int majorVersion{0};
  QString rawOutput{};
};

enum class [[deprecated("Usar mejor Qt::ColorScheme")]] Theme{ Light_Mode, Dark_Mode };
enum class SessionStatus{ Session_start, Session_closed };
enum class User{ U_public, U_user };
enum class AuthType{ Numeric_pin, Secret_Question };
enum class UserRole : uint8_t { Role_User, Role_Admin };
enum class OpenMode{ New, Edit };

struct SessionPermissions {
  UserRole role{UserRole::Role_User};
  bool canEdit{true};
  bool mustChangePassword{false};
};

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

} // namespace SW
