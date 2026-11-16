# Composants tiers

Cineginsa utilise les composants suivants. À fournir avec chaque exécutable distribué (dans le zip, l'installeur ou le dossier de l'application).

## Qt

Cette application utilise la bibliothèque **Qt** (https://www.qt.io), distribuée sous licence **GNU LGPL v3**.

- Qt est utilisé en **liaison dynamique** : les bibliothèques Qt livrées avec l'application peuvent être remplacées par une autre version compatible.
- Le code source de Qt est disponible sur https://download.qt.io/ et https://code.qt.io/.
- Le texte de la licence LGPL v3 est disponible sur https://www.gnu.org/licenses/lgpl-3.0.html (à copier dans un fichier `LICENSES/LGPL-3.0.txt` livré avec l'application).

Modules utilisés : Qt Core, Gui, Widgets, Quick (QML), Multimedia.

## FFmpeg (via Qt Multimedia)

Le backend multimédia de Qt 6.5+ s'appuie sur les bibliothèques **FFmpeg**. Vérifie la licence de la version embarquée dans le kit Qt que tu utilises pour compiler (https://ffmpeg.org/legal.html) et mentionne-la ici.

## Polices

À compléter : nom, auteur et licence de chaque police embarquée (par exemple SIL Open Font License pour les polices Google Fonts).
