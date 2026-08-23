cmake_minimum_required(VERSION 3.19)
project(xxxApp LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Sql Network Svg)

qt_standard_project_setup()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(APP_ICON "${PROJECT_SOURCE_DIR}/appIcon.rc")

include(cmake/ExternalLibraries.cmake)

qt_add_executable(${PROJECT_NAME}
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainform.cpp
    mainform.hpp
    mainform.ui
    rsc.qrc
    helperdatabase/helperdb.cpp helperdatabase/helperdb.hpp util/dataimporterexporter.cpp util/dataimporterexporter.hpp util/helper.cpp util/helper.hpp
    acercadedialog.hpp acercadedialog.cpp acercadedialog.ui
    categorydialog.cpp categorydialog.hpp categorydialog.ui
    dlgnewcategory.cpp dlgnewcategory.hpp dlgnewcategory.ui
    logindialog.cpp logindialog.hpp logindialog.ui
    resetpassworddialog.cpp resetpassworddialog.hpp resetpassworddialog.ui
    publicurldialog.cpp publicurldialog.hpp publicurldialog.ui
    swwidgets/switemdelegate.cpp swwidgets/switemdelegate.hpp swwidgets/swlabel.cpp swwidgets/swlabel.hpp swwidgets/swtablemodel.cpp swwidgets/swtablemodel.hpp
    ${APP_ICON}
    configdialog.hpp configdialog.cpp configdialog.ui
    swwidgets/swtextedit.hpp swwidgets/swtextedit.cpp
    changepwddialog.hpp changepwddialog.cpp changepwddialog.ui
    midlewidget.hpp midlewidget.cpp midlewidget.ui

    maintenanceurldialog.hpp maintenanceurldialog.cpp maintenanceurldialog.ui
    util/urlimportworker.hpp util/urlimportworker.cpp
    util/backupcrypto.hpp util/backupcrypto.cpp

)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    Qt::Core
    Qt::Widgets
    Qt::Sql
    Qt::Network
    Qt::Svg
    QXlsx::QXlsx
    OpenSSL::SSL    # Nombre oficial de CMake
    OpenSSL::Crypto # Nombre oficial de CMake
)

include(GNUInstallDirs)


target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wpedantic
    -Wextra
    -Wconversion
    -Wshadow
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Wcast-align
    -Wunused
    -Woverloaded-virtual
    -Wno-unused-parameter

)

if(WIN32 AND TARGET OpenSSL::SSL)
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:OpenSSL::Crypto>
        $<TARGET_FILE:OpenSSL::SSL>
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "Copiando DLLs de OpenSSL al directorio de salida"
    )
endif()

install(TARGETS ${PROJECT_NAME}
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET ${PROJECT_NAME}
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})


if(WIN32)
    # 1. Intentamos localizar la carpeta bin de PostgreSQL usando psql.exe
    find_program(POSTGRES_PSQL_EXE NAMES psql)

    if(POSTGRES_PSQL_EXE)
        # Si psql está en el PATH, extraemos la ruta de bin
        get_filename_component(POSTGRES_BIN_DIR "${POSTGRES_PSQL_EXE}" DIRECTORY)
    else()
        # Si no está en el PATH, buscamos la instalación de 64 bits en el Registro de Windows
        get_filename_component(PG_REG_DIR
            "[HKEY_LOCAL_MACHINE\\SOFTWARE\\PostgreSQL\\Installations;]@BaseDirectory" ABSOLUTE
        )
    if(EXISTS "${PG_REG_DIR}/bin")
        set(POSTGRES_BIN_DIR "${PG_REG_DIR}/bin")
    endif()
endif()

# 2. Copiamos las DLLs necesarias solo si encontramos el directorio bin
if(POSTGRES_BIN_DIR AND EXISTS "${POSTGRES_BIN_DIR}")
    message(STATUS "PostgreSQL Binaries encontrados en: ${POSTGRES_BIN_DIR}")

    # Copiamos la librería principal libpq.dll
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${POSTGRES_BIN_DIR}/libpq.dll"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    )

# Copiamos dinámicamente cualquier dependencia libintl*.dll que exista en ese bin
file(GLOB LIBINTL_DLLS "${POSTGRES_BIN_DIR}/libintl*.dll")
foreach(dll ${LIBINTL_DLLS})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${dll}"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    )
endforeach()

# Copiamos bibliotecas de OpenSSL que utiliza PostgreSQL
file(GLOB CRYPTO_DLLS "${POSTGRES_BIN_DIR}/libcrypto*.dll" "${POSTGRES_BIN_DIR}/libssl*.dll")
foreach(dll ${CRYPTO_DLLS})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${dll}"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    )
endforeach()

file(GLOB CRYPTO_DLLS "${POSTGRES_BIN_DIR}/libiconv*.dll")
foreach(dll ${CRYPTO_DLLS})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${dll}"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    )
endforeach()
else()
    message(WARNING "No se encontró el directorio de binarios de PostgreSQL. Las DLLs no se copiarán automáticamente.")
endif()
endif()

