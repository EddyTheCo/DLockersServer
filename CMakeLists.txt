cmake_minimum_required(VERSION 3.16)

if(NODE_JWT)
    file(WRITE qml/LocalConf.qml "pragma Singleton \n import QtQuick 2.15 \n QtObject { \n property string nodeaddr:\"https://3216aae.online-server.cloud\" \n property string jwt:\"${NODE_JWT}\"\n }" )
endif(NODE_JWT)

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

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Qml )

qt_standard_project_setup()
FetchContent_Declare(
	qrCode
	GIT_REPOSITORY https://github.com/EddyTheCo/qrCode.git
	GIT_TAG v0.0.2
	FIND_PACKAGE_ARGS 0.0 CONFIG
	)
FetchContent_MakeAvailable(qrCode)

FetchContent_Declare(
	MyDesigns
	GIT_REPOSITORY https://github.com/EddyTheCo/MyDesigns.git
	GIT_TAG v0.2.3
	FIND_PACKAGE_ARGS 0.2 CONFIG
	)
FetchContent_MakeAvailable(MyDesigns)
FetchContent_Declare(
	account
	GIT_REPOSITORY https://github.com/EddyTheCo/account.git
	GIT_TAG v0.2.2
	FIND_PACKAGE_ARGS 0.2 CONFIG
	)
FetchContent_MakeAvailable(account)
FetchContent_Declare(
	nodeConection
	GIT_REPOSITORY https://github.com/EddyTheCo/ConectionSettings.git
	GIT_TAG v0.2.3
	FIND_PACKAGE_ARGS 0.2 CONFIG
	)
FetchContent_MakeAvailable(nodeConection)

FetchContent_Declare(
	booking_model
	GIT_REPOSITORY https://github.com/EddyTheCo/Booking_model.git
	GIT_TAG v0.1.2
	FIND_PACKAGE_ARGS 0.1 CONFIG
	)
FetchContent_MakeAvailable(booking_model)


qt_add_executable(server_main server_main.cpp)

if(RPI_SERVER)
    FetchContent_Declare(
            lgpio
            GIT_REPOSITORY https://github.com/EddyTheCo/lg.git
            GIT_TAG v0.0.1
            FIND_PACKAGE_ARGS 0 CONFIG
            )
    FetchContent_MakeAvailable(lgpio)
    target_link_libraries(server_main PRIVATE lgpio)
    target_compile_definitions(server_main PRIVATE RPI_SERVER)

        if(RPI_SERVER_GPIO)
            target_compile_definitions(server_main PRIVATE GPIO_NUMBER=${RPI_SERVER_GPIO})
        else()
            target_compile_definitions(server_main PRIVATE GPIO_NUMBER=17)
        endif()

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
	IMPORTS MyDesigns nodeConection account booking_model
	IMPORT_PATH ${QML_IMPORT_PATH}
	)

qt_add_resources(server_main "server_main.fonts"
	PREFIX "/esterVtech.com/imports/server"
	FILES
	"qml/fonts/DeliciousHandrawn-Regular.ttf"
	)


target_link_libraries(server_main PRIVATE booking_model nodeConection account qrCode::QtQrGen MyDesigns)
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
