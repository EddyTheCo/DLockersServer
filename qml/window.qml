import QtQuick.Controls
import QtQuick
import QtQuick.Layouts
import nodeConection
import account
import server
import booking_model
import MyDesigns

ApplicationWindow {
    id:mv
    visible: true



    Connections {
        target: Node_Conection
        function onStateChanged() {
            Book_Server.restart();
        }
    }
    Connections {
        target: Account
        function onSeedChanged() {
            Book_Server.restart();
        }
    }
    Connections {
        target: Book_Server
        function onGot_new_booking(boo) {
            Day_model.add_booking(boo,false);
        }
    }

    Popup {
        id:settings
        anchors.centerIn: Overlay.overlay
        visible:true
        closePolicy: Popup.NoAutoClose
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
            Node_Connections
            {
                id:nodeco
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignCenter
                Layout.preferredWidth: 350
                Layout.preferredHeight: 250
            }
            AccountQml
            {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.maximumHeight: 500
                Layout.alignment: Qt.AlignTop
                Layout.preferredHeight: 300
                Layout.preferredWidth:nodeco.width
            }

            MyButton
            {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.maximumHeight: 50
                Layout.maximumWidth: 75
                Layout.preferredWidth: 50
                Layout.minimumHeight: 25
                Layout.minimumWidth:50
                Layout.alignment: Qt.AlignHCenter
                text:qsTr("Start")
                enabled: Node_Conection.state

                onClicked:{
                    settings.close();
                    if(Book_Server.transfer_funds)
                    {
                        payserver.init=true;
                        payserver.open();
                    }
                }
            }
        }
    }
    MyPayPopUp
    {
        id:payserver
        property bool init:false
        descr_:""
        addr_: ""
        url_:""
        anchors.centerIn: Overlay.overlay
        visible:Book_Server.transfer_funds&&payserver.init
        closePolicy: Popup.NoAutoClose
        background:Rectangle
        {
            color:"#0f171e"
            border.width: 1
            border.color: "white"
            radius:8
        }
        onVisibleChanged:
        {
            if(visible)
            {
                payserver.addr_= Account.addr_bech32([0,0,0],Node_Conection.info().protocol.bech32Hrp)
                payserver.descr_=qsTr("Transfer at least "+ Book_Server.transfer_funds +" "+Node_Conection.info().baseToken.subunit+ " to:\n"+payserver.addr_);
                payserver.url_="firefly:v1/wallet/send?recipient="+payserver.addr_+"&amount="+Book_Server.transfer_funds
            }
        }
    }






    ColumnLayout
    {
        id:column
        anchors.fill: parent
        spacing: 0
        Head
        {
            id:head
            property bool init:true
            Layout.maximumHeight: 300
            Layout.minimumHeight: 100
            Layout.fillHeight:  true
            Layout.minimumWidth: 300
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignCenter
            butt.text:(init)?"Open box":"back"
            butt.enabled:Node_Conection.state&&!Book_Server.transfer_funds

            butt.onClicked:
            {
                head.init=!head.init
            }
        }

        Rectangle
        {
            id:bott
            color:"#0f171e"
            Layout.minimumHeight: 300
            Layout.preferredHeight: 400
            Layout.fillHeight:  true
            Layout.minimumWidth: 200
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignCenter
            Day_swipe_view {
                id: dayview
                clip:true
                can_book:false
                anchors.fill:parent
                visible:head.init
            }
            Enter_Pin_server
            {
                visible:!head.init
                anchors.fill:parent
            }
        }

    }




}

