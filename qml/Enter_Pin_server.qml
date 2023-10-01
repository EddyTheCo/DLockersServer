import server
import QtQuick 2.0
import QtQuick.Layouts
import QtQuick.Controls
import MyDesigns
import QtQrGen


ColumnLayout
{

    Connections {
        target: Book_Server
        function onNftAddress(addr) {
            nftaddr.address=addr;
        }
    }

    MyLabel
    {

        Layout.fillWidth: true
        text:qsTr("Send the nft to:")
        font:CustomStyle.h2
        color:CustomStyle.midColor1
        horizontalAlignment:Text.AlignLeft
        fontSizeMode:Text.Fit
        Layout.alignment: Qt.AlignTop
    }
    QrLabel
    {
        id:nftaddr
        Layout.fillWidth: true
        description:"address"
        address:""
        Layout.alignment: Qt.AlignTop
        Layout.bottomMargin: 10
    }
    Rectangle
    {
        color:CustomStyle.backColor2
        Layout.fillHeight:  true
        border.width:2
        border.color:CustomStyle.midColor1
        Layout.fillWidth: true
        Layout.maximumWidth: parent.width*0.8
        Layout.minimumHeight: 50
        Layout.maximumHeight: 200
        radius:Math.min(width,height)*0.07
        Layout.alignment: Qt.AlignCenter
        Layout.margins: 30
        MyLabel
        {
            anchors.centerIn: parent
            width: parent.width*0.85
            height:parent.height
            text:qsTr("The box is " + (Book_Server.open?"open":"closed"))
            font:CustomStyle.h1
            color:(Book_Server.open)?CustomStyle.frontColor1:CustomStyle.midColor1
            horizontalAlignment:Text.AlignHCenter
            verticalAlignment:Text.AlignVCenter
            fontSizeMode:Text.Fit
        }
    }




}









