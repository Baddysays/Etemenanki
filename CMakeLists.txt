cmake_minimum_required(VERSION 3.21)
project(Etemenanki VERSION 1.0.3 LANGUAGES CXX RC)

set(ETE_VERSION ${PROJECT_VERSION})
set(ETE_VERSION_CODE 103)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Network Pdf PdfQuick Concurrent)

set(ETE_HAVE_VENV 0)
set(ETE_PYTHON_EXE "")
if(EXISTS "${CMAKE_SOURCE_DIR}/.venv/Scripts/python.exe")
    set(ETE_HAVE_VENV 1)
    file(TO_NATIVE_PATH "${CMAKE_SOURCE_DIR}/.venv/Scripts/python.exe" ETE_PYTHON_EXE)
    string(REPLACE "\\" "/" ETE_PYTHON_EXE "${ETE_PYTHON_EXE}")
endif()

set(ETE_PROJECT_SOURCE_DIR "${CMAKE_SOURCE_DIR}")
string(REPLACE "\\" "/" ETE_PROJECT_SOURCE_DIR "${ETE_PROJECT_SOURCE_DIR}")

configure_file(
    ${CMAKE_SOURCE_DIR}/cpp/runtime_config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/runtime_config.h
    @ONLY
)

configure_file(
    ${CMAKE_SOURCE_DIR}/cpp/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/version.h
    @ONLY
)

qt_standard_project_setup()

if(MSVC)
    add_compile_options(/utf-8)
endif()

qt_add_executable(Etemenanki
    cpp/main.cpp
    cpp/app_settings.cpp
    cpp/app_ui_strings.cpp
    cpp/app_settings.h
    cpp/document_format.cpp
    cpp/document_format.h
    cpp/document_loader.cpp
    cpp/document_loader.h
    cpp/external_pdf_translate_worker.cpp
    cpp/external_pdf_translate_worker.h
    cpp/translation_workflow.cpp
    cpp/translation_workflow.h
    cpp/translator_backend.cpp
    cpp/translator_backend.h
    cpp/setup_manager.cpp
    cpp/setup_manager.h
)

if(WIN32)
    set(APP_ICO "${CMAKE_SOURCE_DIR}/assets/branding/app.ico")
    if(NOT EXISTS "${APP_ICO}")
        message(WARNING "Missing ${APP_ICO} — run: python tools/build_brand_assets.py")
    endif()
    file(TO_CMAKE_PATH "${APP_ICO}" APP_ICO_PATH)
    string(REPLACE "\\" "/" APP_ICO_NATIVE_PATH "${APP_ICO_PATH}")
    configure_file(
        ${CMAKE_SOURCE_DIR}/cpp/app_icon.rc.in
        ${CMAKE_CURRENT_BINARY_DIR}/generated/app_icon.rc
        @ONLY
    )
    target_sources(Etemenanki PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated/app_icon.rc)
    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/generated/app_icon.rc
        PROPERTIES OBJECT_DEPENDS "${APP_ICO}")
endif()

qt_add_qml_module(Etemenanki
    URI Etemenanki
    VERSION 1.0
    QML_FILES
        qml_cpp/Main.qml
        qml_cpp/LangPicker.qml
        qml_cpp/EtePalette.qml
        qml_cpp/SettingsWindow.qml
        qml_cpp/HelpWindow.qml
        qml_cpp/SetupWizard.qml
        qml_cpp/TranslationHub.qml
)

target_include_directories(Etemenanki PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated)

target_link_libraries(Etemenanki
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::Network
        Qt6::Pdf
        Qt6::PdfQuick
        Qt6::Concurrent
)

qt_import_qml_plugins(Etemenanki)

add_custom_command(TARGET Etemenanki POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:Etemenanki>/assets
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:Etemenanki>/tools
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/assets/models_catalog.json
        $<TARGET_FILE_DIR:Etemenanki>/assets/models_catalog.json
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/tools/extract_document.py
        $<TARGET_FILE_DIR:Etemenanki>/tools/extract_document.py
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/tools/export_document.py
        $<TARGET_FILE_DIR:Etemenanki>/tools/export_document.py
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/tools/pdf_layout.py
        $<TARGET_FILE_DIR:Etemenanki>/tools/pdf_layout.py
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/tools/pdf_engines.py
        $<TARGET_FILE_DIR:Etemenanki>/tools/pdf_engines.py
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/tools/pdf2zh_runner.py
        $<TARGET_FILE_DIR:Etemenanki>/tools/pdf2zh_runner.py
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/tools/ensure_pdf2zh_compat.py
        $<TARGET_FILE_DIR:Etemenanki>/tools/ensure_pdf2zh_compat.py
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:Etemenanki>/tools/bootstrap
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/tools/bootstrap/bootstrap.py
        $<TARGET_FILE_DIR:Etemenanki>/tools/bootstrap/bootstrap.py
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/tools/bootstrap/install_manifest.json
        $<TARGET_FILE_DIR:Etemenanki>/tools/bootstrap/install_manifest.json
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/tools/setup_pdf2zh.ps1
        $<TARGET_FILE_DIR:Etemenanki>/tools/setup_pdf2zh.ps1
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/tools/install_deps.ps1
        $<TARGET_FILE_DIR:Etemenanki>/tools/install_deps.ps1
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/tools/install_deps_ui.ps1
        $<TARGET_FILE_DIR:Etemenanki>/tools/install_deps_ui.ps1
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/tools/embedded_llm.py
        $<TARGET_FILE_DIR:Etemenanki>/tools/embedded_llm.py
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/tools/requirements-embedded.txt
        $<TARGET_FILE_DIR:Etemenanki>/tools/requirements-embedded.txt
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/tools/requirements-embedded-download.txt
        $<TARGET_FILE_DIR:Etemenanki>/tools/requirements-embedded-download.txt
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/tools/requirements-pdf.txt
        $<TARGET_FILE_DIR:Etemenanki>/tools/requirements-pdf.txt
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:Etemenanki>/releases
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/releases/update.json
        $<TARGET_FILE_DIR:Etemenanki>/releases/update.json
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/engines
        $<TARGET_FILE_DIR:Etemenanki>/engines
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets/branding
        $<TARGET_FILE_DIR:Etemenanki>/assets/branding
)

if(ETE_HAVE_VENV)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/python_path.txt "${ETE_PYTHON_EXE}\n")
    add_custom_command(TARGET Etemenanki POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_BINARY_DIR}/python_path.txt
            $<TARGET_FILE_DIR:Etemenanki>/tools/python_path.txt
    )
endif()

if(ETE_HAVE_VENV)
    add_custom_command(TARGET Etemenanki POST_BUILD
        COMMAND "${CMAKE_SOURCE_DIR}/.venv/Scripts/python.exe" -m pip install -q -r "${CMAKE_SOURCE_DIR}/tools/requirements-pdf.txt"
        COMMAND "${CMAKE_SOURCE_DIR}/.venv/Scripts/python.exe" "${CMAKE_SOURCE_DIR}/tools/ensure_pdf2zh_compat.py"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Ensuring Python deps for PDF engines (pdf2zh + NumPy compat)"
    )
endif()
