import server
import QtQuick 2.0
import QtQuick.Layouts
import QtQuick.Controls
import MyDesigns
import nodeConection
import account

Rectangle
{
    id:head
    color:"#0f171e"
    property alias  butt:button
    MyPayPopUp
    {
        id:serverid
        descr_: ""
        addr_: ""
        url_:""
        anchors.centerIn: Overlay.overlay
        visible:false
        closePolicy: Popup.CloseOnPressOutside
        background:Rectangle
        {
            color:"#0f171e"
            border.width: 1
            border.color: "white"
            radius:4
        }

    }

    ColumnLayout
    {
        anchors.fill:head
        MyButton
        {
            id:button
            Layout.maximumHeight: 75
            Layout.minimumHeight: 50
            Layout.fillHeight:  true
            Layout.minimumWidth: 35
            Layout.maximumWidth: 150
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignCenter
        }
        Text
        {
            id:labelidserver
            Layout.minimumHeight: 50
            Layout.fillHeight:  true
            Layout.minimumWidth: 100
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            text: (Node_Conection.state)?"<b>Server id:</b>"+ Account.addr_bech32([0,0,0],Node_Conection.info().protocol.bech32Hrp):""
            visible:Node_Conection.state
            color:"white"
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            ToolTip
            {
                id:tooltip
                visible: false
                text:qsTr("Show")
            }

            MouseArea {
                anchors.fill: parent
                hoverEnabled :true
                onEntered: tooltip.visible=!tooltip.visible
                onExited: tooltip.visible=!tooltip.visible
                onClicked:
                {
                    serverid.descr_=Account.addr_bech32([0,0,0],Node_Conection.info().protocol.bech32Hrp)
                    serverid.addr_= Account.addr_bech32([0,0,0],Node_Conection.info().protocol.bech32Hrp)
                    serverid.url_="https://eddytheco.github.io/DLockersClient/wasm/?serverid="+Account.addr_bech32([0,0,0],Node_Conection.info().protocol.bech32Hrp)
                    serverid.visible=true;
                }
            }
        }
    }
    Rectangle
    {
        id:line
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width
        height:2
        color: "#21be2b"
    }
}
