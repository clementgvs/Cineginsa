import QtQuick
import QtMultimedia

// Écran projecteur. "regie" est injecté depuis le C++ (context property).
Rectangle {
    id: root
    color: "#011329"

    readonly property real u: height / 1080
    readonly property color orange: "#ff7a1a"
    readonly property color orangeClair: "#ffb36b"
    readonly property color beige: "#F7DC98"
    readonly property color marine: "#011329"
    readonly property color carte: "#051c38"
    readonly property bool aAffiche: regie.affiche.toString() !== ""
    readonly property bool aLogo: regie.logo.toString() !== ""

    // raccourcis quand la fenêtre projecteur a le focus (télécommande de présentation)
    focus: true
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_PageDown) {
            regie.sceneSuivante(); event.accepted = true
        } else if (event.key === Qt.Key_PageUp) {
            regie.scenePrecedente(); event.accepted = true
        }
    }

    // briques réutilisables
    component Scene: Item {
        property bool actif: false
        anchors.fill: parent
        opacity: actif ? 1 : 0
        visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: 600; easing.type: Easing.InOutQuad } }
    }

    component Neon: Text {
        color: "#f6e6c4"
        style: Text.Outline
        styleColor: "#55ffb36b"
        font.bold: true
        font.letterSpacing: 4
        wrapMode: Text.WordWrap
    }

    // film (toujours présent, c'est le lecteur C++ qui l'alimente)
    VideoOutput {
        id: video
        anchors.fill: parent
        Component.onCompleted: regie.attacherSortieVideo(video)
        opacity: regie.scene === "film" ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 600 } }
    }

    // décor commun : lueur, logo, bandes de pellicule
    Item {
        anchors.fill: parent
        opacity: regie.scene === "film" ? 0 : 1
        visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: 600 } }

        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#00000000" }
                GradientStop { position: 0.65; color: "#00000000" }
                GradientStop { position: 1.0; color: "#2eff7a1a" }
            }
        }

        Image {
            visible: root.aLogo
            source: regie.logo
            fillMode: Image.PreserveAspectFit
            horizontalAlignment: Image.AlignRight
            mipmap: true
            width: 380 * root.u
            height: 120 * root.u
            anchors.top: parent.top
            anchors.topMargin: 50 * root.u
            anchors.right: parent.right
            anchors.rightMargin: 130 * root.u
        }

        Repeater {
            model: 2
            Rectangle {
                width: 70 * root.u
                height: parent.height
                x: index === 0 ? 0 : parent.width - width
                color: "#0d1a36"
                Rectangle { // liseré néon côté intérieur
                    width: 2 * root.u
                    height: parent.height
                    x: index === 0 ? parent.width - width : 0
                    color: root.orange
                    opacity: 0.85
                }
                Repeater {
                    model: 18
                    Rectangle {
                        width: 26 * root.u
                        height: 38 * root.u
                        radius: 6 * root.u
                        x: (parent.width - width) / 2
                        y: (index + 0.5) * (parent.height / 18) - height / 2
                        color: root.marine
                    }
                }
            }
        }
    }

    // SCÈNE 1 : ATTENTE + SNACKS
    Scene {
        actif: regie.scene === "attente"

        Rectangle {
            x: 170 * root.u
            anchors.verticalCenter: parent.verticalCenter
            width: 520 * root.u
            height: 780 * root.u
            radius: 10 * root.u
            color: root.carte
            border.color: root.orange
            border.width: 3 * root.u

            Image {
                anchors.fill: parent
                anchors.margins: 6 * root.u
                source: regie.affiche
                fillMode: Image.PreserveAspectCrop
                visible: root.aAffiche
            }
            Text {
                anchors.centerIn: parent
                visible: !root.aAffiche
                text: "AFFICHE"
                color: root.orangeClair
                font.pixelSize: 40 * root.u
                font.letterSpacing: 6
            }
        }

        Column {
            x: 770 * root.u
            y: 190 * root.u
            width: parent.width - x - 170 * root.u
            spacing: 18 * root.u

            Text {
                text: "CE SOIR"
                color: root.orangeClair
                font.pixelSize: 32 * root.u
                font.letterSpacing: 10
            }
            Neon {
                width: parent.width
                text: regie.titre.length ? regie.titre.toUpperCase() : "TITRE DU FILM"
                font.pixelSize: 88 * root.u
                maximumLineCount: 2
                elide: Text.ElideRight
            }
            Text {
                text: regie.duree + "   ·   Début à " + regie.heureDebut
                color: root.beige
                font.pixelSize: 38 * root.u
            }
            Rectangle { width: 160 * root.u; height: 4 * root.u; color: root.orange }

            Item { width: 1; height: 6 * root.u }

            Text {
                visible: regie.snacks.length > 0
                text: "SNACKS"
                color: root.orangeClair
                font.pixelSize: 32 * root.u
                font.letterSpacing: 8
            }
            Grid {
                columns: 2
                spacing: 16 * root.u
                Repeater {
                    model: regie.snacks.slice(0, 8)
                    Rectangle {
                        width: (parent.parent.width - 16 * root.u) / 2
                        height: 76 * root.u
                        radius: 12 * root.u
                        color: root.carte
                        border.color: root.orange
                        border.width: 2 * root.u

                        readonly property bool aImage: modelData.img !== ""
                        Rectangle {
                            id: vignette
                            visible: parent.aImage
                            x: 8 * root.u
                            anchors.verticalCenter: parent.verticalCenter
                            width: parent.height - 16 * root.u
                            height: width
                            radius: 8 * root.u
                            color: root.marine
                            clip: true
                            Image {
                                anchors.fill: parent
                                anchors.margins: 4 * root.u
                                source: modelData.img
                                fillMode: Image.PreserveAspectFit
                                mipmap: true
                            }
                        }
                        Text {
                            anchors.left: parent.left
                            anchors.leftMargin: (parent.aImage ? parent.height : 22 * root.u) + 6 * root.u
                            anchors.verticalCenter: parent.verticalCenter
                            width: parent.width - anchors.leftMargin - 190 * root.u
                            text: modelData.nom
                            color: root.beige
                            font.pixelSize: 32 * root.u
                            elide: Text.ElideRight
                        }
                        Text {
                            anchors.right: parent.right
                            anchors.rightMargin: 22 * root.u
                            anchors.verticalCenter: parent.verticalCenter
                            text: modelData.prix
                            color: root.orangeClair
                            font.pixelSize: 34 * root.u
                            font.bold: true
                        }
                    }
                }
            }
        }
    }

    // SCÈNE 2 : INFOS + COMPTE À REBOURS
    Scene {
        actif: regie.scene === "timer"

        Column {
            anchors.centerIn: parent
            width: parent.width - 400 * root.u
            spacing: 26 * root.u

            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                text: "LA SÉANCE COMMENCE DANS"
                color: root.orangeClair
                font.pixelSize: 40 * root.u
                font.letterSpacing: 8
            }
            Neon {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                text: regie.compteARebours
                font.pixelSize: 300 * root.u
                font.letterSpacing: 8
            }
            Rectangle {   // barre de progression néon
                anchors.horizontalCenter: parent.horizontalCenter
                width: 900 * root.u
                height: 10 * root.u
                radius: height / 2
                color: root.carte
                Rectangle {
                    width: parent.width * regie.timerProgression
                    height: parent.height
                    radius: height / 2
                    color: root.orange
                }
            }
            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                text: regie.titre.toUpperCase() + "   ·   " + regie.langue + "   ·   " + regie.duree
                color: root.beige
                font.pixelSize: 46 * root.u
                elide: Text.ElideRight
            }
            Rectangle {
                visible: regie.messageInfos.length > 0
                anchors.horizontalCenter: parent.horizontalCenter
                width: Math.min(parent.width, msg.implicitWidth + 100 * root.u)
                height: msg.implicitHeight + 50 * root.u
                radius: 14 * root.u
                color: root.carte
                border.color: root.orange
                border.width: 2 * root.u
                Text {
                    id: msg
                    anchors.centerIn: parent
                    width: parent.width - 60 * root.u
                    horizontalAlignment: Text.AlignHCenter
                    text: regie.messageInfos
                    color: root.beige
                    font.pixelSize: 44 * root.u
                    wrapMode: Text.WordWrap
                }
            }
        }
    }

    //  FIN
    Scene {
        actif: regie.scene === "fin"

        Column {
            anchors.centerIn: parent
            width: parent.width - 400 * root.u
            spacing: 30 * root.u

            Neon {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                text: "MERCI !"
                font.pixelSize: 200 * root.u
            }
            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                text: "Bonne soirée et à bientôt"
                color: root.orangeClair
                font.pixelSize: 56 * root.u
            }
        }
    }

    /*
    // sous-titres : le VideoOutput ne dessine pas le texte tout seul
    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 80 * root.u
        width: 1600 * root.u
        opacity: regie.scene === "film" ? 1 : 0
        text: video.videoSink ? video.videoSink.subtitleText : ""
        color: "white"
        style: Text.Outline
        styleColor: "black"
        font.pixelSize: 48 * root.u
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
    }
    */
}
