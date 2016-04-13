import QtQuick 2.0

Item {
    id: root
    width: 800
    height: 800

    MouseArea {
        id: ma
        anchors.fill: parent
        onClicked: {
            ma.visible = false
            ma.enabled = false
            Harness.continueTwoStage(root)
        }
    }
}
