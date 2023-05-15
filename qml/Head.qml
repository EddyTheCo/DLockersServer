import server
import QtQuick 2.0
import QtQuick.Layouts
import QtQuick.Controls
import MyDesigns
import nodeConection
import account



ColumnLayout
{
    id:head

    property alias  butt:button
    property bool init:true
    MyPayPopUp
    {
        id:serverid
        address: ""
        description:""
        anchors.centerIn: Overlay.overlay
        visible:false
        closePolicy: Popup.CloseOnPressOutside

    }
    MyButton
    {
        id:button
        Layout.maximumHeight: 60
        Layout.minimumHeight: 50
        Layout.fillHeight:  true
        Layout.minimumWidth: 35
        Layout.maximumWidth: 120
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignCenter
    }

    Rectangle
    {
        id:line
        Layout.alignment: Qt.AlignHCenter|Qt.AlignBottom
        Layout.maximumHeight: 2
        Layout.fillHeight: true
        Layout.fillWidth: true
        color: "#21be2b"
    }

}


