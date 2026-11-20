#include "mainwindow.h"
#include "regie.h"

#include <QAudioOutput>
#include <QButtonGroup>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGridLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMediaMetaData>
#include <QMediaPlayer>
#include <QPushButton>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickView>
#include <QScreen>
#include <QSettings>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>
#include <QStatusBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QTimeEdit>
#include <QUrl>
#include <QVBoxLayout>

namespace {
    const char *kStyle = R"(
        QWidget { background: #011329; color: #F7DC98; font-size: 14px; }
        QTabWidget::pane { border: 1px solid #0d2b4d; }
        QTabBar::tab { background: #051c38; color: #F7DC98; padding: 8px 18px; border: 1px solid #0d2b4d; }
        QTabBar::tab:selected { color: #ff7a1a; border-bottom: 2px solid #ff7a1a; }
        QLineEdit, QComboBox, QTimeEdit, QTableWidget { background: #051c38; color: #F7DC98; border: 1px solid #163b66; padding: 4px; }
        QHeaderView::section { background: #051c38; color: #F7DC98; border: 0; padding: 4px; }
        QPushButton { background: #072345; color: #F7DC98; border: 1px solid #ff7a1a; border-radius: 6px; padding: 6px 14px; }
        QPushButton:hover { background: #0e3361; }
        QPushButton:checked { background: #ff7a1a; color: #011329; font-weight: bold; }
        QSlider::groove:horizontal { height: 6px; background: #0d2b4d; border-radius: 3px; }
        QSlider::sub-page:horizontal { background: #ff7a1a; border-radius: 3px; }
        QSlider::handle:horizontal { background: #F7DC98; width: 14px; margin: -5px 0; border-radius: 7px; }
        QStatusBar { color: #F7DC98; }
    )";
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    m_regie = new Regie(this);

    construireInterface();
    creerAffichage();
    brancherLecteur();
    chargerReglages();

    appliquerSeance();
    appliquerSnacks();
    placerAffichage();
    m_regie->setScene("attente");
    m_boutonsScene.value("attente")->setChecked(true);
}

MainWindow::~MainWindow()
{
    delete m_affichage;   // QQuickView sans parent
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    enregistrerReglages();
    m_affichage->close();
    event->accept();
}

void MainWindow::construireInterface()
{
    setStyleSheet(kStyle);
    setWindowTitle("Cineginsa – Régie");
    resize(940, 660);

    auto *onglets = new QTabWidget;
    onglets->addTab(ongletSeance(), "Séance");
    onglets->addTab(ongletSnacks(), "Snacks");
    onglets->addTab(ongletPilotage(), "Régie");
    onglets->addTab(ongletReglages(), "Réglages");
    onglets->setCurrentIndex(2);
    setCentralWidget(onglets);
    statusBar();
}

QWidget *MainWindow::ongletSeance()
{
    auto *w = new QWidget;
    auto *form = new QFormLayout(w);

    m_editTitre = new QLineEdit;
    m_editTitre->setPlaceholderText("Titre du film");
    m_editDuree = new QLineEdit;
    m_editDuree->setPlaceholderText("ex : 2h 15min");
    m_comboLangue = new QComboBox;
    m_comboLangue->addItems({"VF", "VOST FR"});
    m_timeDebut = new QTimeEdit(QTime(20, 30));
    m_timeDebut->setDisplayFormat("HH:mm");
    m_editMessage = new QLineEdit("Merci d'éteindre vos téléphones");

    m_labelAffiche = new QLabel("(aucune)");
    m_labelFilm = new QLabel("(aucun)");
    m_labelLogo = new QLabel("(aucun)");
    auto *btnLogo = new QPushButton("Choisir le logo…");
    auto *ligneLogo = new QHBoxLayout;
    ligneLogo->addWidget(btnLogo);
    ligneLogo->addWidget(m_labelLogo, 1);
    auto *btnAffiche = new QPushButton("Choisir l'affiche…");
    auto *btnFilm = new QPushButton("Choisir le film…");

    auto *ligneAffiche = new QHBoxLayout;
    ligneAffiche->addWidget(btnAffiche);
    ligneAffiche->addWidget(m_labelAffiche, 1);
    auto *ligneFilm = new QHBoxLayout;
    ligneFilm->addWidget(btnFilm);
    ligneFilm->addWidget(m_labelFilm, 1);

    form->addRow("Titre", m_editTitre);
    form->addRow("Durée", m_editDuree);
    form->addRow("Langue", m_comboLangue);
    form->addRow("Heure de début", m_timeDebut);
    form->addRow("Message (scène Infos)", m_editMessage);
    form->addRow("Affiche", ligneAffiche);
    form->addRow("Logo (en haut à droite)", ligneLogo);
    form->addRow("Film", ligneFilm);

    connect(m_editTitre, &QLineEdit::textChanged, this, &MainWindow::appliquerSeance);
    connect(m_editDuree, &QLineEdit::textChanged, this, &MainWindow::appliquerSeance);
    connect(m_editMessage, &QLineEdit::textChanged, this, &MainWindow::appliquerSeance);
    connect(m_comboLangue, &QComboBox::currentIndexChanged, this, &MainWindow::appliquerSeance);
    connect(m_timeDebut, &QTimeEdit::timeChanged, this, &MainWindow::appliquerSeance);
    connect(btnAffiche, &QPushButton::clicked, this, &MainWindow::choisirAffiche);
    connect(btnFilm, &QPushButton::clicked, this, &MainWindow::choisirFilm);
    connect(btnLogo, &QPushButton::clicked, this, &MainWindow::choisirLogo);
    return w;
}

QWidget *MainWindow::ongletSnacks()
{
    auto *w = new QWidget;
    auto *lay = new QVBoxLayout(w);

    m_tableSnacks = new QTableWidget(0, 3);
    m_tableSnacks->setHorizontalHeaderLabels({"Nom", "Prix", "Image"});
    m_tableSnacks->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_tableSnacks->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_tableSnacks->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableSnacks->verticalHeader()->setDefaultSectionSize(36);

    auto *btnAjout = new QPushButton("Ajouter");
    auto *btnSuppr = new QPushButton("Supprimer");
    auto *boutons = new QHBoxLayout;
    boutons->addWidget(btnAjout);
    boutons->addWidget(btnSuppr);
    boutons->addStretch();

    //lay->addWidget(new QLabel("Affichés dans la scène « Attente & snacks » (8 max)."));
    lay->addWidget(m_tableSnacks, 1);
    lay->addLayout(boutons);

    connect(btnAjout, &QPushButton::clicked, this, [this] {
        ajouterSnack("Nouveau", "0,00 €", "");
        appliquerSnacks();
    });
    connect(btnSuppr, &QPushButton::clicked, this, [this] {
        const int ligne = m_tableSnacks->currentRow();
        if (ligne >= 0) {
            m_tableSnacks->removeRow(ligne);
            appliquerSnacks();
        }
    });
    connect(m_tableSnacks, &QTableWidget::itemChanged, this, &MainWindow::appliquerSnacks);
    return w;
}

QWidget *MainWindow::ongletPilotage()
{
    auto *w = new QWidget;
    auto *lay = new QVBoxLayout(w);

    //  scènes
    const QList<QPair<QString, QString>> scenes = {
        {"attente", "Attente & snacks"}, {"timer", "Infos & compte à rebours"},
        {"film", "Film"}, {"fin", "Fin"}};

    m_groupeScenes = new QButtonGroup(this);
    m_groupeScenes->setExclusive(true);
    auto *grille = new QGridLayout;
    int i = 0;
    for (const auto &s : scenes) {
        auto *b = new QPushButton(QString("%1\nF%2").arg(QString(s.second).replace("&", "&&")).arg(i + 1));
        b->setCheckable(true);
        b->setMinimumHeight(60);
        b->setShortcut(QKeySequence(Qt::Key_F1 + i));
        m_groupeScenes->addButton(b);
        m_boutonsScene.insert(s.first, b);
        grille->addWidget(b, 0, i);
        const QString id = s.first;
        connect(b, &QPushButton::clicked, this, [this, id] { m_regie->setScene(id); });
        ++i;
    }
    lay->addLayout(grille);

    // scène précédente / suivante
    auto *btnPrec = new QPushButton("◀  Scène précédente  (PgUp)");
    auto *btnSuiv = new QPushButton("Scène suivante  (PgDn)  ▶");
    btnPrec->setMinimumHeight(44);
    btnSuiv->setMinimumHeight(44);
    btnPrec->setShortcut(QKeySequence(Qt::Key_Left));
    btnSuiv->setShortcut(QKeySequence(Qt::Key_Right));
    auto *nav = new QHBoxLayout;
    nav->addWidget(btnPrec);
    nav->addWidget(btnSuiv);
    lay->addLayout(nav);
    connect(btnPrec, &QPushButton::clicked, m_regie, &Regie::scenePrecedente);
    connect(btnSuiv, &QPushButton::clicked, m_regie, &Regie::sceneSuivante);

    m_labelScene = new QLabel;
    QFont f = m_labelScene->font();
    f.setPointSize(16);
    f.setBold(true);
    m_labelScene->setFont(f);
    m_labelScene->setStyleSheet("color: #ff7a1a;");
    lay->addWidget(m_labelScene);

    //  timer
    auto *boite = new QWidget;
    boite->setStyleSheet("QWidget#boiteTimer { border: 1px solid #2a3c68; border-radius: 6px; }");
    boite->setObjectName("boiteTimer");
    auto *tl = new QVBoxLayout(boite);

    m_spinMin = new QSpinBox;
    m_spinMin->setRange(0, 999);
    m_spinMin->setSuffix(" min");
    m_spinSec = new QSpinBox;
    m_spinSec->setRange(0, 59);
    m_spinSec->setSuffix(" s");
    auto *btnCaler = new QPushButton("Caler sur l'heure de début");
    m_labelTimer = new QLabel;
    m_labelTimer->setStyleSheet("color: #ffb36b; font-size: 30px; font-weight: bold;");

    auto *ligne1 = new QHBoxLayout;
    ligne1->addWidget(new QLabel("Durée du timer :"));
    ligne1->addWidget(m_spinMin);
    ligne1->addWidget(m_spinSec);
    ligne1->addWidget(btnCaler);
    ligne1->addStretch();
    ligne1->addWidget(m_labelTimer);
    tl->addLayout(ligne1);

    m_btnTimer = new QPushButton("Lancer le timer  (T)");
    m_btnTimer->setShortcut(QKeySequence(Qt::Key_T));
    auto *btnReset = new QPushButton("Réinitialiser");
    auto *ligne2 = new QHBoxLayout;
    ligne2->addWidget(m_btnTimer, 1);
    ligne2->addWidget(btnReset);
    tl->addLayout(ligne2);

    m_checkAutoFilm = new QCheckBox("Lancer le film automatiquement à 00:00");
    m_checkAutoFilm->setChecked(true);
    m_checkAfficherTimer = new QCheckBox("Passer sur la scène « Infos & compte à rebours » au lancement");
    m_checkAfficherTimer->setChecked(true);
    tl->addWidget(m_checkAutoFilm);
    tl->addWidget(m_checkAfficherTimer);
    lay->addWidget(boite);

    //  lecture
    auto *btnPlay = new QPushButton("Lecture / Pause  (P)");
    btnPlay->setShortcut(QKeySequence(Qt::Key_P));
    auto *btnReculer = new QPushButton("−10 s");
    auto *btnAvancer = new QPushButton("+10 s");
    auto *lecture = new QHBoxLayout;
    lecture->addWidget(btnReculer);
    lecture->addWidget(btnPlay, 1);
    lecture->addWidget(btnAvancer);
    lay->addLayout(lecture);

    m_sliderPosition = new QSlider(Qt::Horizontal);
    m_labelPosition = new QLabel("00:00 / 00:00");
    auto *pos = new QHBoxLayout;
    pos->addWidget(m_sliderPosition, 1);
    pos->addWidget(m_labelPosition);
    lay->addLayout(pos);

    m_sliderVolume = new QSlider(Qt::Horizontal);
    m_sliderVolume->setRange(0, 100);
    m_sliderVolume->setValue(80);
    m_comboAudio = new QComboBox;
    m_comboSous = new QComboBox;
    auto *form = new QFormLayout;
    form->addRow("Volume", m_sliderVolume);
    form->addRow("Piste audio", m_comboAudio);
    form->addRow("Sous-titres", m_comboSous);
    lay->addLayout(form);
    lay->addStretch();

    // connexions scène
    connect(m_regie, &Regie::sceneChanged, this, [this] {
        const QString s = m_regie->scene();
        m_labelScene->setText("Scène : " + s.toUpper());
        if (auto *b = m_boutonsScene.value(s))
            b->setChecked(true);
    });

    // connexions timer
    connect(m_spinMin, &QSpinBox::valueChanged, this, &MainWindow::appliquerTimer);
    connect(m_spinSec, &QSpinBox::valueChanged, this, &MainWindow::appliquerTimer);
    connect(btnCaler, &QPushButton::clicked, this, [this] {
        int s = QTime::currentTime().secsTo(m_regie->debut());
        if (s < 0) s = 0;
        const QSignalBlocker b1(m_spinMin);
        const QSignalBlocker b2(m_spinSec);
        m_spinMin->setValue(s / 60);
        m_spinSec->setValue(s % 60);
        appliquerTimer();
    });
    connect(m_btnTimer, &QPushButton::clicked, this, [this] {
        const bool allaitLancer = !m_regie->timerActif();
        m_regie->basculerTimer();
        if (allaitLancer && m_checkAfficherTimer->isChecked())
            m_regie->setScene("timer");
    });
    connect(btnReset, &QPushButton::clicked, m_regie, &Regie::reinitialiserTimer);
    connect(m_checkAutoFilm, &QCheckBox::toggled, this, [this](bool v) { m_regie->setAutoFilm(v); });
    connect(m_regie, &Regie::compteChanged, this, [this] {
        m_labelTimer->setText(m_regie->compteARebours());
    });
    connect(m_regie, &Regie::timerChanged, this, [this] {
        m_btnTimer->setText(m_regie->timerActif() ? "Pause du timer  (T)" : "Lancer le timer  (T)");
    });
    connect(m_regie, &Regie::timerTermine, this, [this] {
        statusBar()->showMessage("Timer terminé", 8000);
    });
    m_labelTimer->setText(m_regie->compteARebours());

    // connexions lecteur
    auto *p = m_regie->player();
    connect(btnPlay, &QPushButton::clicked, this, [p] {
        if (p->playbackState() == QMediaPlayer::PlayingState)
            p->pause();
        else
            p->play();
    });
    connect(btnReculer, &QPushButton::clicked, this, [p] { p->setPosition(qMax<qint64>(0, p->position() - 10000)); });
    connect(btnAvancer, &QPushButton::clicked, this, [p] { p->setPosition(p->position() + 10000); });
    connect(m_sliderPosition, &QSlider::sliderMoved, this, [p](int v) { p->setPosition(v); });
    connect(m_sliderVolume, &QSlider::valueChanged, this, [this](int v) {
        m_regie->audio()->setVolume(v / 100.0f);
    });
    connect(m_comboAudio, &QComboBox::currentIndexChanged, this, [p](int i) {
        if (i >= 0) p->setActiveAudioTrack(i);
    });
    connect(m_comboSous, &QComboBox::currentIndexChanged, this, [p](int i) {
        if (i >= 0) p->setActiveSubtitleTrack(i - 1);   // 0 = « Aucun » -> -1
    });
    return w;
}

QWidget *MainWindow::ongletReglages()
{
    auto *w = new QWidget;
    auto *form = new QFormLayout(w);
    m_comboEcran = new QComboBox;
    m_checkFenetre = new QCheckBox("Mode fenêtré (test, sans plein écran)");
    auto *btnActualiser = new QPushButton("Actualiser les écrans");
    form->addRow("Écran projecteur", m_comboEcran);
    form->addRow(m_checkFenetre);
    form->addRow(btnActualiser);

    connect(m_comboEcran, &QComboBox::currentIndexChanged, this, [this] {
        if (m_affichage) placerAffichage();
    });
    connect(m_checkFenetre, &QCheckBox::toggled, this, [this] {
        if (m_affichage) placerAffichage();
    });
    connect(btnActualiser, &QPushButton::clicked, this, [this] {
        remplirEcrans();
        placerAffichage();
    });
    remplirEcrans();
    return w;
}

// écran projecteur

void MainWindow::creerAffichage()
{
    m_affichage = new QQuickView;
    m_affichage->setTitle("Cineginsa – Projecteur");
    m_affichage->setResizeMode(QQuickView::SizeRootObjectToView);
    // le contexte doit être posé AVANT setSource
    m_affichage->rootContext()->setContextProperty("regie", m_regie);
    m_affichage->setSource(QUrl("qrc:/display.qml"));

    if (m_affichage->status() == QQuickView::Error) {
        statusBar()->showMessage("Erreur de chargement du QML (voir la sortie de l'application)");
        return;
    }
    if (auto *racine = m_affichage->rootObject()) {
        if (auto *video = racine->findChild<QObject *>("video"))
            m_regie->player()->setVideoOutput(video);
    }
}

void MainWindow::remplirEcrans()
{
    const QSignalBlocker blocker(m_comboEcran);
    const int ancien = m_comboEcran->currentIndex();
    m_comboEcran->clear();
    const auto ecrans = QGuiApplication::screens();
    for (int i = 0; i < ecrans.size(); ++i) {
        const QRect g = ecrans.at(i)->geometry();
        m_comboEcran->addItem(QString("%1 : %2 (%3×%4)")
                                  .arg(i + 1).arg(ecrans.at(i)->name()).arg(g.width()).arg(g.height()));
    }
    if (ancien >= 0 && ancien < ecrans.size())
        m_comboEcran->setCurrentIndex(ancien);
    else
        m_comboEcran->setCurrentIndex(ecrans.size() > 1 ? 1 : 0);
}

void MainWindow::placerAffichage()
{
    const auto ecrans = QGuiApplication::screens();
    int i = m_comboEcran->currentIndex();
    if (i < 0 || i >= ecrans.size())
        i = 0;
    QScreen *ecran = ecrans.at(i);
    const QRect g = ecran->geometry();

    // un seul écran -> toujours fenêtré, sinon le plein écran cacherait la régie
    const bool fenetre = m_checkFenetre->isChecked() || ecrans.size() < 2;

    m_affichage->setScreen(ecran);
    if (fenetre) {
        m_affichage->showNormal();
        m_affichage->setGeometry(g.x() + 60, g.y() + 60, 960, 540);
        m_affichage->show();
    } else {
        m_affichage->setGeometry(g);
        m_affichage->showFullScreen();
    }
}

//  logique

void MainWindow::brancherLecteur()
{
    auto *p = m_regie->player();

    connect(p, &QMediaPlayer::errorOccurred, this, [this](QMediaPlayer::Error, const QString &msg) {
        statusBar()->showMessage("Erreur lecteur : " + msg, 10000);
    });
    connect(p, &QMediaPlayer::tracksChanged, this, &MainWindow::majPistes);
    connect(p, &QMediaPlayer::durationChanged, this, [this](qint64 d) {
        m_sliderPosition->setRange(0, int(d));
    });
    connect(p, &QMediaPlayer::positionChanged, this, [this, p](qint64 pos) {
        if (!m_sliderPosition->isSliderDown())
            m_sliderPosition->setValue(int(pos));
        m_labelPosition->setText(formatMs(pos) + " / " + formatMs(p->duration()));
    });
}

void MainWindow::majPistes()
{
    auto *p = m_regie->player();
    const QSignalBlocker b1(m_comboAudio);
    const QSignalBlocker b2(m_comboSous);

    auto nom = [](const QMediaMetaData &md, int i, const QString &defaut) {
        QStringList parts;
        const QVariant lang = md.value(QMediaMetaData::Language);
        if (lang.isValid()) {
            const auto l = lang.value<QLocale::Language>();
            if (l != QLocale::AnyLanguage)
                parts << QLocale::languageToString(l);
        }
        const QString titre = md.stringValue(QMediaMetaData::Title);
        if (!titre.isEmpty())
            parts << titre;
        return parts.isEmpty() ? QString("%1 %2").arg(defaut).arg(i + 1) : parts.join(" – ");
    };

    m_comboAudio->clear();
    const auto audio = p->audioTracks();
    for (int i = 0; i < audio.size(); ++i)
        m_comboAudio->addItem(nom(audio.at(i), i, "Piste"));
    m_comboAudio->setCurrentIndex(p->activeAudioTrack());

    m_comboSous->clear();
    m_comboSous->addItem("Aucun");
    const auto sous = p->subtitleTracks();
    for (int i = 0; i < sous.size(); ++i)
        m_comboSous->addItem(nom(sous.at(i), i, "Sous-titres"));
    m_comboSous->setCurrentIndex(p->activeSubtitleTrack() + 1);

    statusBar()->showMessage(QString("Pistes audio : %1 | sous-titres : %2")
                                 .arg(audio.size()).arg(sous.size()), 8000);
}

void MainWindow::appliquerSeance()
{
    if (!m_regie) return;
    m_regie->definirSeance(m_editTitre->text(), m_editDuree->text(),
                           m_comboLangue->currentText(), m_timeDebut->time(),
                           m_editMessage->text());
}

void MainWindow::appliquerSnacks()
{
    QVariantList liste;
    for (int r = 0; r < m_tableSnacks->rowCount(); ++r) {
        const auto *n = m_tableSnacks->item(r, 0);
        const auto *pr = m_tableSnacks->item(r, 1);
        const auto *img = m_tableSnacks->item(r, 2);
        if (!n || n->text().trimmed().isEmpty())
            continue;
        QVariantMap m;
        m["nom"] = n ? n->text() : QString();
        m["prix"] = pr ? pr->text() : QString();
        m["img"] = (img && !img->text().isEmpty())
                       ? QUrl::fromLocalFile(img->text()).toString()
                       : QString();
        liste << m;
    }
    m_regie->definirSnacks(liste);
}

void MainWindow::ajouterSnack(const QString &nom, const QString &prix, const QString &imageUrl)
{
    const QSignalBlocker blocker(m_tableSnacks);
    const int r = m_tableSnacks->rowCount();
    m_tableSnacks->insertRow(r);
    m_tableSnacks->setItem(r, 0, new QTableWidgetItem(nom));
    m_tableSnacks->setItem(r, 1, new QTableWidgetItem(prix));

    // Item invisible qui porte la valeur (chemin) : c'est lui que lit appliquerSnacks()
    auto *imgItem = new QTableWidgetItem(imageUrl);
    imgItem->setFlags(imgItem->flags() & ~Qt::ItemIsEditable);
    m_tableSnacks->setItem(r, 2, imgItem);

    // Widget affiché par-dessus la cellule : nom du fichier + bouton
    auto *cellule = new QWidget;
    auto *ligne = new QHBoxLayout(cellule);
    ligne->setContentsMargins(4, 0, 4, 0);

    auto *label = new QLabel(imageUrl.isEmpty() ? "(aucune)" : QFileInfo(imageUrl).fileName());
    auto *bouton = new QPushButton("Trouver une image…");
    ligne->addWidget(label, 1);
    ligne->addWidget(bouton);

    connect(bouton, &QPushButton::clicked, this, [this, imgItem, label] {
        const QString chemin = choisirImageSnack(imgItem->tableWidget()
                                                 ? m_tableSnacks->item(imgItem->row(), 0)->text() : QString());
        if (chemin.isEmpty())
            return;
        imgItem->setText(chemin);              // déclenche itemChanged -> appliquerSnacks()
        label->setText(QFileInfo(chemin).fileName());
    });

    m_tableSnacks->setCellWidget(r, 2, cellule);
}

QString MainWindow::choisirImageSnack(const QString & nom)
{
    const QString chemin = QFileDialog::getOpenFileName(
        this, "Choisir l'image pour " + nom, nullptr, "Images (*.png *.jpg *.jpeg *.webp)");
    if (chemin.isEmpty())
        return "";
    return chemin;
}

void MainWindow::choisirAffiche()
{
    const QString chemin = QFileDialog::getOpenFileName(
        this, "Choisir l'affiche", m_cheminAffiche, "Images (*.png *.jpg *.jpeg *.webp)");
    if (chemin.isEmpty())
        return;
    m_cheminAffiche = chemin;
    m_labelAffiche->setText(QFileInfo(chemin).fileName());
    m_regie->definirAffiche(QUrl::fromLocalFile(chemin));
}

void MainWindow::choisirFilm()
{
    const QString chemin = QFileDialog::getOpenFileName(
        this, "Choisir le film", m_cheminFilm,
        "Vidéos (*.mkv *.mp4 *.avi *.mov *.m4v);;Tous les fichiers (*)");
    if (chemin.isEmpty())
        return;
    m_cheminFilm = chemin;
    m_labelFilm->setText(QFileInfo(chemin).fileName());
    m_regie->chargerFilm(QUrl::fromLocalFile(chemin));
}

void MainWindow::choisirLogo()
{
    const QString chemin = QFileDialog::getOpenFileName(
        this, "Choisir le logo", m_cheminLogo, "Images (*.png *.jpg *.jpeg *.webp *.svg)");
    if (chemin.isEmpty())
        return;
    m_cheminLogo = chemin;
    m_labelLogo->setText(QFileInfo(chemin).fileName());
    m_regie->definirLogo(QUrl::fromLocalFile(chemin));
}

void MainWindow::appliquerTimer()
{
    m_regie->definirTimer(m_spinMin->value() * 60 + m_spinSec->value());
}

QString MainWindow::formatMs(qint64 ms)
{
    const qint64 s = ms / 1000;
    const qint64 h = s / 3600;
    const int m = int((s % 3600) / 60);
    const int sec = int(s % 60);
    return h > 0 ? QString("%1:%2:%3").arg(h).arg(m, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0'))
                 : QString("%1:%2").arg(m, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0'));
}

// sauvegarde

void MainWindow::enregistrerReglages()
{
    QSettings s;
    s.setValue("titre", m_editTitre->text());
    s.setValue("duree", m_editDuree->text());
    s.setValue("langue", m_comboLangue->currentIndex());
    s.setValue("debut", m_timeDebut->time());
    s.setValue("message", m_editMessage->text());
    s.setValue("affiche", m_cheminAffiche);
    s.setValue("film", m_cheminFilm);
    s.setValue("ecran", m_comboEcran->currentIndex());
    s.setValue("fenetre", m_checkFenetre->isChecked());
    s.setValue("volume", m_sliderVolume->value());
    s.setValue("logo", m_cheminLogo);
    s.setValue("timerMin", m_spinMin->value());
    s.setValue("timerSec", m_spinSec->value());
    s.setValue("autoFilm", m_checkAutoFilm->isChecked());
    s.setValue("afficherTimer", m_checkAfficherTimer->isChecked());

    QStringList snacks;
    for (int r = 0; r < m_tableSnacks->rowCount(); ++r) {
        const auto *n = m_tableSnacks->item(r, 0);
        const auto *pr = m_tableSnacks->item(r, 1);
        const auto *img = m_tableSnacks->item(r, 2);
        snacks << (n ? n->text() : QString()) + "\t" + (pr ? pr->text() : QString())
                      + "\t" + (img ? img->text() : QString());
    }
    s.setValue("snacks", snacks);
}

void MainWindow::chargerReglages()
{
    QSettings s;
    m_editTitre->setText(s.value("titre").toString());
    m_editDuree->setText(s.value("duree").toString());
    m_comboLangue->setCurrentIndex(s.value("langue", 0).toInt());
    m_timeDebut->setTime(s.value("debut", QTime(20, 30)).toTime());
    m_editMessage->setText(s.value("message", "Merci d'éteindre vos téléphones").toString());
    m_sliderVolume->setValue(s.value("volume", 80).toInt());

    m_spinMin->setValue(s.value("timerMin", 10).toInt());
    m_spinSec->setValue(s.value("timerSec", 0).toInt());
    m_checkAutoFilm->setChecked(s.value("autoFilm", true).toBool());
    m_checkAfficherTimer->setChecked(s.value("afficherTimer", true).toBool());
    m_regie->setAutoFilm(m_checkAutoFilm->isChecked());
    appliquerTimer();

    const QString logo = s.value("logo").toString();
    if (!logo.isEmpty() && QFileInfo::exists(logo)) {
        m_cheminLogo = logo;
        m_labelLogo->setText(QFileInfo(logo).fileName());
        m_regie->definirLogo(QUrl::fromLocalFile(logo));
    }

    const QString affiche = s.value("affiche").toString();
    if (!affiche.isEmpty() && QFileInfo::exists(affiche)) {
        m_cheminAffiche = affiche;
        m_labelAffiche->setText(QFileInfo(affiche).fileName());
        m_regie->definirAffiche(QUrl::fromLocalFile(affiche));
    }
    const QString film = s.value("film").toString();
    if (!film.isEmpty() && QFileInfo::exists(film)) {
        m_cheminFilm = film;
        m_labelFilm->setText(QFileInfo(film).fileName());
        m_regie->chargerFilm(QUrl::fromLocalFile(film));
    }

    const QStringList snacks = s.value("snacks").toStringList();
    if (snacks.isEmpty()) {
        ajouterSnack("Pop-corn", "0,10 €", "");
    } else {
        for (const QString &ligne : snacks) {
            const auto morceaux = ligne.split('\t');
            ajouterSnack(morceaux.value(0), morceaux.value(1), morceaux.value(2));
        }
    }

    if (s.contains("ecran") && s.value("ecran").toInt() < m_comboEcran->count())
        m_comboEcran->setCurrentIndex(s.value("ecran").toInt());
    m_checkFenetre->setChecked(s.value("fenetre", QGuiApplication::screens().size() < 2).toBool());
}
