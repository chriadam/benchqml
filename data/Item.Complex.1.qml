import QtQuick 2.0

Item {
    id: root
    width: 400
    height: 700

    Grid {
        id: grid
        width: 400
        rows: 2
        Rectangle {
            id: r1
            color: "blue"
            width: 200
            height: 200
        }
        Rectangle {
            id: r2
            color: "black"
            width: 200
            height: 200
        }
        Rectangle {
            id: r3
            color: "red"
            width: 200
            height: 200
        }
        Rectangle {
            id: r4
            color: "green"
            width: 200
            height: 200

            Rectangle {
                id: r5
                color: "yellow"
                width: 50
                height: 50
            }
        }
    }

    Rectangle {
        id: r6
        x: 50
        y: 450
        width: 200
        height: 150
        color: "purple"
    }
}
