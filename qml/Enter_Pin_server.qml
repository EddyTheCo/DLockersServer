import server
import QtQuick 2.0
import QtQuick.Layouts
import QtQuick.Controls
import MyDesigns


ColumnLayout
{
    id: ep
    required property  StackView stack
    required property int headh
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
    Head
    {
        id:head
        Layout.preferredHeight:ep.headh
        Layout.maximumHeight: ep.headh
        Layout.minimumHeight: 100
        Layout.fillHeight:  true
        Layout.minimumWidth: 300
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignTop
        butt.text:"Back"
        butt.onClicked:ep.stack.pop()
    }


    Rectangle
    {
        id:center_
        color:"#0f171e"
        Layout.fillHeight:  true
        Layout.minimumWidth: 300
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignTop

        ColumnLayout
        {
            spacing:20
            anchors.fill: parent
            MyPinBox
            {
                id:pin_box_

                Layout.maximumHeight: 200
                Layout.fillHeight:  true
                Layout.maximumWidth: 300
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignCenter

                description:qsTr("Enter pin:")
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
                    var component;
                    var next
                    if(Book_Server.try_to_open(pin_box_.pin.text))
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


    }



}
