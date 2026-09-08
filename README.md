# SWUrlManager 🚀

[![Estado](https://img.shields.io/badge/Estado-Alpha%202%20--%20Beta%201-blue)](#)
[![Lenguaje](https://img.shields.io/badge/Lenguaje-C%2B%2B17-00599C)](#)
[![Framework](https://img.shields.io/badge/Framework-Qt-41CD52)](#)
[![Database](https://img.shields.io/badge/Base%20de%20Datos-PostgreSQL-4169E1)](#)

**SWUrlManager** es una aplicación de escritorio desarrollada en **C++ (Qt)** integrada con **PostgreSQL**, enfocada en la gestión segura de usuarios, control de acceso y almacenamiento cifrado de datos.

---

## 📌 Estado Actual del Proyecto

La **migración de la base de datos a PostgreSQL está completa** — la aplicación ya no depende de SQLite en ningún punto. La app corre como una instalación local e independiente por usuario, sin depender de un servidor central compartido entre varias máquinas.

### 🛠️ Características Implementadas

**Gestión Segura de Usuarios**
- Autenticación con hash seguro de contraseñas y verificación en el servidor.
- Roles diferenciados **Administrador** / **Usuario**, con permisos de edición configurables por cuenta y estado de cuenta activa/inactiva.
- Cuenta de administrador generada automáticamente en la primera instalación, con una contraseña temporal única por instalación (mostrada una sola vez, con opción de copiarla), y cambio de contraseña obligatorio en el primer inicio de sesión.
- Mecanismo de rescate de clave con los datos protegidos mediante cifrado.

**Cifrado de datos**
- Los datos sensibles se protegen con una clave de cifrado propia de cada instalación, generada de forma aleatoria y nunca embebida en el ejecutable.
- Esa clave se resguarda mediante una contraseña maestra definida por el administrador al configurar la instalación, y se cachea de forma segura en el equipo para no solicitarla en cada apertura de la aplicación.

**Gestión de URLs**
- Organización por categorías, con soporte de usuario público (URLs visibles sin necesidad de iniciar sesión) y usuarios autenticados con sus propias categorías privadas.
- Datos cifrados en el servidor, con verificación de existencia sin necesidad de descifrar.
- Importación desde Excel (`.xlsx`), CSV, TSV y texto plano, con detección de duplicados (omitir o reemplazar) y arrastrar-y-soltar (drag & drop) directo sobre la tabla.
- Exportación a los mismos formatos.
- La importación corre en segundo plano con diálogo de progreso — la interfaz no se congela con archivos grandes.
- Carga paginada de la tabla de URLs para categorías con gran volumen de registros.

**Copias de Seguridad**
- Backup y restauración completa de la base de datos, disponible solo para el rol Administrador.
- Backup y restauración de datos por usuario individual, protegido con una contraseña propia del archivo, portable entre distintas instalaciones y máquinas — incluye la opción de sumar las URLs públicas al respaldo.

**Interfaz**
- Soporte de tema claro/oscuro con detección del esquema del sistema operativo.
- Widget central (`MidleWidget`) reutilizado entre formularios para evitar duplicación de UI.
- Reporte de problemas integrado: genera un Issue prellenado en el repositorio de GitHub del proyecto, incluyendo información del sistema para facilitar el diagnóstico.

---

## 🚧 En Desarrollo Activo

- **Mejoras de estabilidad general:** revisión y corrección de casos borde en la app (manejo de errores en operaciones de base de datos, robustez ante fallos de conexión).
- **Módulo de administración de usuarios:** interfaz dedicada para que el administrador gestione cuentas, permisos y estado de otros usuarios.

---

## 🗺️ Hoja de Ruta (Roadmap)

### 📦 Beta 1 (Próximo Lanzamiento)
- [ ] Empaquetado portable de la aplicación (`windeployqt` + dependencias necesarias).
- [ ] Pruebas de estabilidad de conexión a la base de datos.
- [ ] Corrección de errores y refinamiento de la interfaz de usuario en Qt.

### 🔮 Futuras Versiones (v1.1 / v2.0)
- [ ] Sistema de auditoría de inicio/cierre de sesión.
- [ ] Módulo de reportes.
- [ ] Capa adicional de cifrado del archivo de backup completo en tránsito.

---

## 💻 Requisitos para Compilación y Desarrollo

- **C++:** Estándar C++17 o superior.
- **Framework:** Qt 6.x
- **Base de Datos:** PostgreSQL 17+ con extensión `pgcrypto`.
- **Herramientas:** CMake, MSYS2/MinGW (Windows).

---

## ⚙️ Configuración Inicial de la Base de Datos

El esquema (tablas, índices y funciones) se aplica automáticamente al primer arranque de la aplicación, siempre que el usuario configurado tenga permisos para crear la base de datos y las extensiones necesarias.

Para aplicarlo manualmente:

```sql
CREATE EXTENSION IF NOT EXISTS pgcrypto;
```

El script completo se encuentra en `database/tableAndFunctions.sql`, embebido también como recurso de la aplicación para la inicialización automática.

---

## 🐛 Reportar un problema

Desde el menú de la aplicación se puede abrir el diálogo de reporte de errores, que arma un Issue prellenado listo para enviar en [este repositorio](https://github.com/lincolningaroca/SWUrlManager/issues).