cmake_minimum_required(VERSION 3.16)

include(local_conf.cmake OPTIONAL)

project(DLockers VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)
qt_standard_project_setup()

qt_add_executable(server_main server_main.cpp)
qt6_add_qml_module(server_main
	URI binter 
	VERSION 1.0
	SOURCES
	binter/src/Hour_model.cpp binter/src/Day_model.cpp  binter/include/Day_model.hpp binter/include/Hour_model.hpp
	binter/src/bclient.cpp binter/src/bserver.cpp  binter/include/bserver.hpp binter/include/bclient.hpp
	QML_FILES
	binter/qml/Check_Open_Window.qml
	binter/qml/Enter_Pin_server.qml
	binter/qml/Enter_Pin_client.qml
	binter/qml/Pin_box.qml
	binter/qml/Main_view_server.qml
	binter/qml/window_server.qml
	binter/qml/CButton.qml
	binter/qml/Day_swipe_view.qml
	binter/qml/Dia_delegate.qml
	binter/qml/Horario_delegate.qml
	binter/qml/Horario_list_view.qml
	binter/qml/Main_view_client.qml
	binter/qml/window_client.qml
	binter/qml/Logo.qml
	binter/qml/Init_popup_server.qml
	binter/qml/Init_popup_client.qml
	binter/qml/Address_qr.qml
	binter/qml/Pay_popup.qml
	binter/qml/Book_popup.qml
	)
target_compile_options(server_main PRIVATE -pthread)
target_link_options(server_main PRIVATE -pthread -sPTHREAD_POOL_SIZE=8)
target_link_libraries(server_main PRIVATE Qt6::Gui Qt6::Quick QtQrGen )

target_include_directories(server_main PUBLIC binter/include)

include(FetchContent)
FetchContent_Declare(
	qrCode
	GIT_REPOSITORY git@github.com:EddyTheCo/qrCode.git
	GIT_TAG main
	)
FetchContent_MakeAvailable(qrCode)

include(FetchContent)
FetchContent_Declare(
	qclient
	GIT_REPOSITORY git@github.com:EddyTheCo/Qclient-IOTA.git
	GIT_TAG main
	)
FetchContent_MakeAvailable(qclient)
FetchContent_Declare(
	qclientMQTT
	GIT_REPOSITORY git@github.com:EddyTheCo/QclientMqtt-IOTA.git
	GIT_TAG main
	)
FetchContent_MakeAvailable(qclientMQTT)

FetchContent_Declare(
        booking
        GIT_REPOSITORY git@github.com:EddyTheCo/booking.git
        GIT_TAG main
        )
FetchContent_MakeAvailable(booking)

FetchContent_Declare(
        account
        GIT_REPOSITORY git@github.com:EddyTheCo/account.git
        GIT_TAG main
        )
FetchContent_MakeAvailable(account)


target_link_libraries(server_main PRIVATE  qclient qclientMQTT  booking  account)

target_compile_definitions(server_main
	PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)

add_custom_command(
  TARGET server_main
  POST_BUILD
  COMMAND ${CMAKE_COMMAND}
  ARGS -E copy ${CMAKE_CURRENT_BINARY_DIR}/server_main.js ${CMAKE_CURRENT_BINARY_DIR}/server_main.wasm ${CMAKE_CURRENT_BINARY_DIR}/qtloader.js ${CMAKE_CURRENT_SOURCE_DIR}/wasm
  )
