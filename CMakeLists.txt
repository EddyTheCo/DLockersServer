cmake_minimum_required(VERSION 3.16)

include(local_conf.cmake OPTIONAL)

project(DLockers VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Qml)


qt_add_executable(server_main server_main.cpp)


qt6_add_qml_module(server_main
	URI server
	VERSION 1.0
	SOURCES
	src/bserver.cpp  include/bserver.hpp
	QML_FILES
	qml/window.qml
	qml/Enter_Pin_server.qml
	qml/Head.qml
        RESOURCE_PREFIX
         "/esterVtech.com/imports"
        IMPORTS MyDesigns nodeConection account booking_model
	IMPORT_PATH
        ${CMAKE_BINARY_DIR}/_deps/mydesigns-build
	${CMAKE_BINARY_DIR}/_deps/nodeconection-build
	${CMAKE_BINARY_DIR}/_deps/account-build
        ${CMAKE_BINARY_DIR}/_deps/booking_model-build
	)


target_link_libraries(server_main PRIVATE Qt::Core Qt::Gui Qt::Qml Qt::Quick)

target_include_directories(server_main PUBLIC include)

include(FetchContent)
FetchContent_Declare(
	qrCode
	GIT_REPOSITORY git@github.com:EddyTheCo/qrCode.git
	GIT_TAG main
	)
FetchContent_MakeAvailable(qrCode)

FetchContent_Declare(
        booking_model
        GIT_REPOSITORY git@github.com:EddyTheCo/Booking_model.git
	GIT_TAG main
	)
FetchContent_MakeAvailable(booking_model)

FetchContent_Declare(
        nodeConection
        GIT_REPOSITORY git@github.com:EddyTheCo/ConectionSettings.git
	GIT_TAG main
	)
FetchContent_MakeAvailable(nodeConection)

FetchContent_Declare(
        account
        GIT_REPOSITORY git@github.com:EddyTheCo/account.git
	GIT_TAG main
	)
FetchContent_MakeAvailable(account)

target_link_libraries(server_main PRIVATE booking_model nodeConection account QtQrGen)
target_compile_definitions(server_main
	PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
if(EMSCRIPTEN)
add_custom_command(
	TARGET server_main
	POST_BUILD
	COMMAND ${CMAKE_COMMAND}
	ARGS -E copy ${CMAKE_CURRENT_BINARY_DIR}/server_main.js ${CMAKE_CURRENT_BINARY_DIR}/server_main.wasm ${CMAKE_CURRENT_BINARY_DIR}/qtloader.js  ${CMAKE_CURRENT_SOURCE_DIR}/wasm
	)
endif()
