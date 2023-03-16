import server
import QtQuick 2.0
import QtQuick.Layouts
import QtQuick.Controls
import MyDesigns


ColumnLayout
{
    id: ep

    spacing:0
    Popup {
        id:result
        anchors.centerIn: Overlay.overlay
        visible:false
        closePolicy: Popup.CloseOnPressOutside
        property alias description:label.text;
        background:Rectangle
        {
            color:"#0f171e"
            border.width: 1
            border.color: "white"
            radius:8
        }
        ColumnLayout
        {
            anchors.fill:parent
            MyLabel
            {
                id:label
                Layout.maximumHeight: 200
                Layout.fillHeight:  true
                Layout.maximumWidth: 300
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignCenter
                Layout.preferredWidth: 300
                Layout.preferredHeight: 200
                font.pointSize:20
                wrapMode:Text.WrapAnywhere
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

        }

    }
    MyLabel
    {
        text: qsTr("Enter pin")
        Layout.alignment: Qt.AlignCenter
        font.pointSize: 16
    }

    TextInput {
        id:numbers_
        Layout.maximumHeight: 200
        Layout.fillHeight:  true
        Layout.maximumWidth: 300
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignCenter
        font.letterSpacing :20
        font.pointSize: 28
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        color:"white"
        inputMask: "99999"
        text: "12345"
    }


    MyButton
    {
        Layout.maximumWidth: 150
        Layout.maximumHeight: 50
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.alignment: Qt.AlignCenter
        text: "Open"
        onClicked:
        {

            if(Book_Server.try_to_open(numbers_.text))
            {
                result.description=qsTr("The box is opened")
                result.visible=true;
            }
            else
            {
                result.description=qsTr("Nice try but no")
                result.visible=true;
            }
        }
    }


}







