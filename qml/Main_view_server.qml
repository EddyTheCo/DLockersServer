import QtQuick 2.0
import QtQuick.Layouts
import QtQuick.Controls
import booking_model

ColumnLayout
{

    id:mv
    required property StackView stack
    spacing:5

    Init_popup_server
    {
        anchors.centerIn: Overlay.overlay
        visible:!Book_Server.publishing
        width: 360
        height: 460
        closePolicy: Popup.NoAutoClose
    }

    Rectangle
    {
        id:bottom_
        color:"#0f171e"

        Layout.fillHeight:  true
        Layout.minimumHeight: 400
        Layout.minimumWidth: 360
        Layout.maximumWidth: 600
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignCenter




    }




}
