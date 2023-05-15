import QtQuick.Controls
import QtQuick
import QtQuick.Layouts
import nodeConection
import account
import server
import booking_model
import MyDesigns

ApplicationWindow {
    id:window
    visible: true
    background: Rectangle
    {
        color:CustomStyle.backColor1
    }


    Notification
    {
        id:noti
        width:300
        height:100
        x:(window.width-width)*0.5
        y: window.height*(1-0.05)-height
    }
    Connections {
        target: Node_Conection
        function onStateChanged() {
            if(Node_Conection.state==Node_Conection.Connected)
            {
                noti.show({"message":"Conected to "+ Node_Conection.nodeaddr });
            }
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

    Drawer {
        id:settings
        width:300

        height:parent.height
        focus:true
        modal:true


        background: Rectangle
        {
            color:CustomStyle.backColor1
        }
        ColumnLayout
        {
            anchors.fill:parent


            Rectangle
            {
                color:CustomStyle.backColor2
                radius:Math.min(width,height)*0.05
                Layout.minimumWidth: 100
                Layout.maximumHeight: (Node_Conection.state)?200:100
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: (Node_Conection.state)? 120:50
                Layout.alignment: Qt.AlignHCenter|Qt.AlignTop
                Layout.margins: 20
                border.color:CustomStyle.midColor1
                border.width: 1
                ColumnLayout
                {
                    anchors.fill:parent
                    TextAddress
                    {
                        visible:Node_Conection.state
                        description:qsTr("<b>Server id</b>")
                        address:Book_Server.serverId
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        Layout.alignment: Qt.AlignTop
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumHeight: 30
                        Layout.margins: 5
                    }
                    Text
                    {
                        font:(Node_Conection.state)?CustomStyle.h3:CustomStyle.h2
                        text:(Node_Conection.state)?qsTr("Available Balance: "):qsTr("Waiting for node")
                        horizontalAlignment:Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        color: CustomStyle.frontColor1
                        fontSizeMode:Text.Fit
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignCenter
                        Layout.fillHeight: true
                    }
                    AmountText
                    {
                        visible:Node_Conection.state
                        font:CustomStyle.h2
                        jsob:Book_Server.funds
                        horizontalAlignment:Text.AlignHCenter
                        fontSizeMode:Text.Fit
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignCenter
                        Layout.fillHeight: true
                    }
                    Text
                    {
                        visible:Number(Book_Server.funds.largeValue.value)<Number(Book_Server.minfunds.largeValue.value)
                        font:CustomStyle.h4
                        text: "Transfer at least:" + (Number(Book_Server.minfunds.largeValue.value)-Number(Book_Server.funds.largeValue.value)) + '<font color=\"'+CustomStyle.frontColor2+'\">'+Book_Server.minfunds.largeValue.unit+'</font>'
                        horizontalAlignment:Text.AlignHCenter
                        color: CustomStyle.frontColor1
                        fontSizeMode:Text.Fit
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignCenter
                        Layout.fillHeight: true
                    }

                }

            }

            Node_Connections
            {
                id:conn_
                Layout.fillWidth: true
                Layout.minimumHeight: 30
                collapsed:1.0
            }
            AccountQml
            {
                id:acc_
                Layout.fillWidth: true
                Layout.minimumHeight: 30
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
            Layout.preferredHeight: 100
            Layout.minimumWidth: 300
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            butt.text:(init)?"Open box":"back"
            butt.enabled:Node_Conection.state

            butt.onClicked:
            {
                if(head.init)Book_Server.try_to_open();
                if(!head.init)Book_Server.open=false;
                head.init=!head.init

            }
        }


        Day_swipe_view {
            id: dayview
            Layout.fillWidth: true
            Layout.fillHeight:  true
            Layout.minimumWidth: 300
            Layout.maximumWidth: 700
            Layout.alignment: Qt.AlignTop|Qt.AlignHCenter
            clip:true
            can_book:false
            Layout.leftMargin: seetbutt.width
            Layout.rightMargin: seetbutt.width
            visible:head.init
        }
        Enter_Pin_server
        {
            visible:!head.init
            Layout.fillWidth: true
            Layout.fillHeight:  true
            Layout.minimumWidth: 300
            Layout.maximumWidth: 700
            Layout.alignment: Qt.AlignTop|Qt.AlignHCenter
            Layout.leftMargin: seetbutt.width
            Layout.rightMargin: seetbutt.width
        }



    }

    MySettButton
    {
        id:seetbutt
        width: 40 + ((window.width>400)?10:0) + ((window.width>800)?20:0)
        x:settings.width*settings.position
        y:(window.height-height)*0.5
        height:width
        onClicked:
        {
            settings.open()
        }
        animate: settings.position>0.1

    }

}

