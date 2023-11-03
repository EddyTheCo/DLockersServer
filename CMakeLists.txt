cmake_minimum_required(VERSION 3.16)

include(local_conf.cmake OPTIONAL)

include(FetchContent)
FetchContent_Declare(
    ccommon
    GIT_REPOSITORY https://github.com/EddyTheCo/Common.git
    GIT_TAG main
)
FetchContent_MakeAvailable(ccommon)
version_from_git(
    LOG       OFF
    TIMESTAMP "%Y%m%d%H%M%S"
)

project(DLockers VERSION ${VERSION} LANGUAGES CXX)


set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
include(GNUInstallDirs)

set(default_build_type "Release")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
        STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Qml  OPTIONAL_COMPONENTS Positioning Serialport)


qt_standard_project_setup()
FetchContent_Declare(
    qrCode
    GIT_REPOSITORY https://github.com/EddyTheCo/qrCode.git
    GIT_TAG develop
    FIND_PACKAGE_ARGS 0.3 CONFIG
)
set(OpenCV_DOWNLOAD OFF)
FetchContent_MakeAvailable(qrCode)
FetchContent_Declare(
    OMQml
    GIT_REPOSITORY https://github.com/EddyTheCo/OMClient.git
    GIT_TAG fixes
    FIND_PACKAGE_ARGS 0.1 CONFIG
)
FetchContent_MakeAvailable(OMQml)

FetchContent_Declare(
    MyDesigns
    GIT_REPOSITORY https://github.com/EddyTheCo/MyDesigns.git
    GIT_TAG v0.3.1
    FIND_PACKAGE_ARGS 0.3 CONFIG
)
FetchContent_MakeAvailable(MyDesigns)
FetchContent_Declare(
    account
    GIT_REPOSITORY https://github.com/EddyTheCo/account.git
    GIT_TAG v0.4.1
    FIND_PACKAGE_ARGS 0.4 CONFIG
)
FetchContent_MakeAvailable(account)
FetchContent_Declare(
    nodeConection
    GIT_REPOSITORY https://github.com/EddyTheCo/ConectionSettings.git
    GIT_TAG v0.4.1
    FIND_PACKAGE_ARGS 0.4 CONFIG
)
FetchContent_MakeAvailable(nodeConection)

FetchContent_Declare(
    booking_model
    GIT_REPOSITORY https://github.com/EddyTheCo/Booking_model.git
    GIT_TAG v0.2.1
    FIND_PACKAGE_ARGS 0.2 CONFIG
)
FetchContent_MakeAvailable(booking_model)


qt_add_executable(server_main server_main.cpp)
set(RPI_SERVER ON)
if(RPI_SERVER)
    message(STATUS "Setting build RPI_SERVER")
    FetchContent_Declare(
        lgpio
        GIT_REPOSITORY https://github.com/EddyTheCo/lg.git
        GIT_TAG v0.0.1
        FIND_PACKAGE_ARGS 0 CONFIG
    )
FetchContent_MakeAvailable(lgpio)
target_link_libraries(server_main PRIVATE lgpio)
target_compile_definitions(server_main PRIVATE RPI_SERVER
    SERIAL_PORT_NAME=$<IF:$<BOOL:${RPI_SERIAL_PN}>,${RPI_SERIAL_PN},"serial:/dev/ttyUSB1">
    GPIO_NUMBER=$<IF:$<BOOL:${RPI_SERVER_GPIO}>,${RPI_SERVER_GPIO},17>
)

if(NOT TARGET Qt6::Positioning)
    FetchContent_Declare(
        qtpositioning
        GIT_REPOSITORY git://code.qt.io/qt/qtpositioning.git
        GIT_TAG dev
    )
FetchContent_MakeAvailable(qtpositioning)
endif(NOT TARGET Qt6::Positioning)

target_link_libraries(server_main PRIVATE Qt6::Positioning )

endif(RPI_SERVER)

set_source_files_properties(qml/LocalConf.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

qt6_add_qml_module(server_main
    URI server
    VERSION 1.0
    SOURCES
    src/bserver.cpp  include/bserver.hpp
    QML_FILES
    qml/window.qml
    qml/Enter_Pin_server.qml
    qml/Head.qml
    qml/LocalConf.qml
    RESOURCE_PREFIX
    "/esterVtech.com/imports"
    RESOURCES
    "qml/fonts/DeliciousHandrawn-Regular.ttf"
    IMPORT_PATH ${CMAKE_BINARY_DIR}
)

target_link_libraries(server_main PRIVATE booking_model nodeConection account QtQrGen MyDesigns OMQml
    $<$<STREQUAL:$<TARGET_PROPERTY:QtQrGen,TYPE>,STATIC_LIBRARY>:QtQrGenplugin>
    $<$<STREQUAL:$<TARGET_PROPERTY:OMQml,TYPE>,STATIC_LIBRARY>:OMQmlplugin>
    $<$<STREQUAL:$<TARGET_PROPERTY:MyDesigns,TYPE>,STATIC_LIBRARY>:MyDesignsplugin>
    $<$<STREQUAL:$<TARGET_PROPERTY:account,TYPE>,STATIC_LIBRARY>:accountplugin>
    $<$<STREQUAL:$<TARGET_PROPERTY:booking_model,TYPE>,STATIC_LIBRARY>:booking_modelplugin>
)
target_link_libraries(server_main PRIVATE Qt::Core Qt::Gui Qt::Qml Qt::Quick)


target_include_directories(server_main PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

set_target_properties(server_main PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)


install(TARGETS server_main
    BUNDLE  DESTINATION .
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)
if(QTDEPLOY)
    qt_generate_deploy_qml_app_script(
        TARGET server_main
        OUTPUT_SCRIPT deploy_script
    )
install(SCRIPT ${deploy_script})
endif(QTDEPLOY)

if(EMSCRIPTEN)
    prepare_wasm_folder()
    add_custom_command(
        TARGET server_main
        POST_BUILD
        COMMAND ${CMAKE_COMMAND}
        ARGS -E copy ${CMAKE_CURRENT_BINARY_DIR}/server_main.js ${CMAKE_CURRENT_BINARY_DIR}/server_main.wasm ${CMAKE_CURRENT_BINARY_DIR}/qtloader.js  ${CMAKE_CURRENT_SOURCE_DIR}/wasm
    )
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(CPACK_PACKAGE_CONTACT "estervtech")
    set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-v${SEMVER}-${CMAKE_SYSTEM_NAME}_${CMAKE_SYSTEM_VERSION}-${CMAKE_SYSTEM_PROCESSOR}-${CMAKE_CXX_COMPILER_ID}")

    if(USE_THREADS)
        set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}-MThread")
    endif(USE_THREADS)
    include(CPack)
endif()
