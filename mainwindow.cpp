#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "regie.h"

#include <QButtonGroup>
#include <QCloseEvent>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QPushButton>
#include <QQmlContext>
#include <QQuickView>
#include <QScreen>
#include <QSettings>
#include <QShortcut>
#include <QSignalBlocker>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTableWidgetItem>
#include <QUrl>
#include <QWindow>

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
        QMenuBar::item:selected, QMenu::item:selected { background: #0e3361; }
        QMenu { background: #051c38; border: 1px solid #0d2b4d; }
        QLabel#labelScene { color: #ff7a1a; font-size: 21px; font-weight: bold; }
        QLabel#labelTimer { color: #ffb36b; font-size: 30px; font-weight: bold; }
        QFrame#boiteTimer { border: 1px solid #2a3c68; border-radius: 6px; }
    )";
}

// construction

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_regie(new Regie(this))
{
    ui->setupUi(this);

    initialiserInterface();
    creerAffichage();
    connecterSeance();
    connecterSnacks();
    connecterPilotage();
    connecterLecteur();
    connecterReglages();
    creerRaccourcis();

    chargerReglages();          // volume, mode fenêtré (QSettings)
    chargerDerniereSeance();    // tout le reste (JSON)

    appliquerSeance();
    appliquerSnacks();
    appliquerTimer();
    m_regie->setAutoFilm(ui->checkAutoFilm->isChecked());
    m_regie->setVolume(ui->sliderVolume->value());

    placerAffichage();
    m_regie->setScene("attente");
    majScene();
}

MainWindow::~MainWindow()
{
    delete m_affichage;   // QQuickView sans parent
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    enregistrerReglages();

    QDir().mkpath(QFileInfo(cheminDerniereSeance()).absolutePath());
    QFile f(cheminDerniereSeance());
    if (f.open(QIODevice::WriteOnly))
        f.write(QJsonDocument(seanceEnJson()).toJson());

    if (m_affichage)
        m_affichage->close();
    event->accept();
}

void MainWindow::initialiserInterface()
{
    setStyleSheet(kStyle);

    ui->tableSnacks->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->tableSnacks->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    ui->tableSnacks->verticalHeader()->setDefaultSectionSize(36);

    // boutons de scène : un seul allumé à la fois, retrouvés par le nom de la scène
    m_boutonsScene.insert("attente", ui->btnSceneAttente);
    m_boutonsScene.insert("timer", ui->btnSceneTimer);
    m_boutonsScene.insert("film", ui->btnSceneFilm);
    m_boutonsScene.insert("fin", ui->btnSceneFin);
    m_groupeScenes = new QButtonGroup(this);
    m_groupeScenes->setExclusive(true);
    for (QPushButton *b : std::as_const(m_boutonsScene))
        m_groupeScenes->addButton(b);

    remplirEcrans();
}

void MainWindow::creerAffichage()
{
    m_affichage = new QQuickView;
    m_affichage->setTitle("Cineginsa – Projecteur");
    m_affichage->setColor(Qt::black);
    m_affichage->setResizeMode(QQuickView::SizeRootObjectToView);
    m_affichage->rootContext()->setContextProperty("regie", m_regie);
    m_affichage->setSource(QUrl("qrc:/display.qml"));

    if (m_affichage->status() == QQuickView::Error)
        statusBar()->showMessage("Erreur de chargement du QML (voir la sortie de l'application)");
}

// raccourcis

void MainWindow::creerRaccourcis()
{
    // ApplicationShortcut : marche quel que soit l'onglet affiché et même si c'est la
    // fenêtre projecteur qui a le focus
    auto lier = [this](const QKeySequence &touche, auto action) {
        auto *s = new QShortcut(touche, this);
        s->setContext(Qt::ApplicationShortcut);
        connect(s, &QShortcut::activated, this, action);
    };
    auto allerA = [this](const QString &scene) {
        m_regie->setScene(scene);
        majScene();      // resynchronise les boutons si la scène a été refusée (pas de film)
    };

    lier(QKeySequence(Qt::Key_F1), [=] { allerA("attente"); });
    lier(QKeySequence(Qt::Key_F2), [=] { allerA("timer"); });
    lier(QKeySequence(Qt::Key_F3), [=] { allerA("film"); });
    lier(QKeySequence(Qt::Key_F4), [=] { allerA("fin"); });

    // les flèches ne changent PAS de scène : un → pendant le film ne doit pas couper la séance
    lier(QKeySequence(Qt::Key_PageDown), [this] { m_regie->sceneSuivante(); majScene(); });
    lier(QKeySequence(Qt::Key_PageUp), [this] { m_regie->scenePrecedente(); majScene(); });

    lier(QKeySequence(Qt::Key_T), [this] { ui->btnTimer->click(); });
    lier(QKeySequence(Qt::Key_P), [this] { m_regie->basculerLecture(); });
    lier(QKeySequence(Qt::CTRL | Qt::Key_Left), [this] { m_regie->sauter(-10); });
    lier(QKeySequence(Qt::CTRL | Qt::Key_Right), [this] { m_regie->sauter(10); });
    lier(QKeySequence(Qt::CTRL | Qt::Key_Up),
         [this] { ui->sliderVolume->setValue(ui->sliderVolume->value() + 5); });
    lier(QKeySequence(Qt::CTRL | Qt::Key_Down),
         [this] { ui->sliderVolume->setValue(ui->sliderVolume->value() - 5); });
    lier(QKeySequence(Qt::Key_F11), [this] { ui->checkFenetre->toggle(); });
}

// connexions

void MainWindow::connecterSeance()
{
    connect(ui->editTitre, &QLineEdit::textChanged, this, &MainWindow::appliquerSeance);
    connect(ui->editDuree, &QLineEdit::textChanged, this, &MainWindow::appliquerSeance);
    connect(ui->editMessage, &QLineEdit::textChanged, this, &MainWindow::appliquerSeance);
    connect(ui->comboLangue, &QComboBox::currentIndexChanged, this, &MainWindow::appliquerSeance);
    connect(ui->timeDebut, &QTimeEdit::timeChanged, this, &MainWindow::appliquerSeance);
    connect(ui->btnAffiche, &QPushButton::clicked, this, &MainWindow::choisirAffiche);
    connect(ui->btnFilm, &QPushButton::clicked, this, &MainWindow::choisirFilm);
    connect(ui->btnLogo, &QPushButton::clicked, this, &MainWindow::choisirLogo);

    connect(ui->actionCharger, &QAction::triggered, this, &MainWindow::chargerSeance);
    connect(ui->actionEnregistrer, &QAction::triggered, this, &MainWindow::sauverSeance);
    connect(ui->actionQuitter, &QAction::triggered, this, &QWidget::close);
}

void MainWindow::connecterSnacks()
{
    connect(ui->btnAjout, &QPushButton::clicked, this, [this] {
        ajouterSnack("Nouveau", "0,00 €", "");
        appliquerSnacks();
    });
    connect(ui->btnSuppr, &QPushButton::clicked, this, [this] {
        const int ligne = ui->tableSnacks->currentRow();
        if (ligne >= 0) {
            ui->tableSnacks->removeRow(ligne);
            appliquerSnacks();
        }
    });
    connect(ui->tableSnacks, &QTableWidget::itemChanged, this, [this] { appliquerSnacks(); });
}

void MainWindow::connecterPilotage()
{
    // scènes
    for (auto it = m_boutonsScene.cbegin(); it != m_boutonsScene.cend(); ++it) {
        const QString id = it.key();
        connect(it.value(), &QPushButton::clicked, this, [this, id] {
            m_regie->setScene(id);
            majScene();
        });
    }
    connect(ui->btnPrec, &QPushButton::clicked, this, [this] { m_regie->scenePrecedente(); majScene(); });
    connect(ui->btnSuiv, &QPushButton::clicked, this, [this] { m_regie->sceneSuivante(); majScene(); });
    connect(m_regie, &Regie::sceneChanged, this, &MainWindow::majScene);

    // timer
    connect(ui->spinMin, &QSpinBox::valueChanged, this, &MainWindow::appliquerTimer);
    connect(ui->spinSec, &QSpinBox::valueChanged, this, &MainWindow::appliquerTimer);
    connect(ui->btnCaler, &QPushButton::clicked, this, [this] {
        int s = QTime::currentTime().secsTo(m_regie->debut());
        if (s < 0) s = 0;
        const QSignalBlocker b1(ui->spinMin);
        const QSignalBlocker b2(ui->spinSec);
        ui->spinMin->setValue(s / 60);
        ui->spinSec->setValue(s % 60);
        appliquerTimer();
    });
    connect(ui->btnTimer, &QPushButton::clicked, this, [this] {
        const bool allaitLancer = !m_regie->timerActif();
        m_regie->basculerTimer();
        if (allaitLancer && ui->checkAfficherTimer->isChecked())
            m_regie->setScene("timer");
    });
    connect(ui->btnReset, &QPushButton::clicked, m_regie, &Regie::reinitialiserTimer);
    connect(ui->checkAutoFilm, &QCheckBox::toggled, this, [this](bool v) { m_regie->setAutoFilm(v); });
    connect(m_regie, &Regie::compteChanged, this, [this] {
        ui->labelTimer->setText(m_regie->compteARebours());
    });
    connect(m_regie, &Regie::timerChanged, this, [this] {
        ui->btnTimer->setText(m_regie->timerActif() ? "Pause du timer  (T)" : "Lancer le timer  (T)");
    });
    connect(m_regie, &Regie::timerTermine, this, [this] {
        statusBar()->showMessage("Timer terminé", 8000);
    });
    ui->labelTimer->setText(m_regie->compteARebours());
}

void MainWindow::connecterLecteur()
{
    // la fenêtre ne parle qu'à Regie : aucun QMediaPlayer ici
    connect(ui->btnPlay, &QPushButton::clicked, m_regie, &Regie::basculerLecture);
    connect(ui->btnReculer, &QPushButton::clicked, this, [this] { m_regie->sauter(-10); });
    connect(ui->btnAvancer, &QPushButton::clicked, this, [this] { m_regie->sauter(10); });
    connect(ui->sliderVolume, &QSlider::valueChanged, m_regie, &Regie::setVolume);

    // valueChanged est bloqué quand c'est le programme qui déplace le curseur (voir majPosition),
    // donc on ne reçoit ici que les actions de l'utilisateur (glisser ou cliquer sur la barre)
    connect(ui->sliderPosition, &QSlider::valueChanged, this, [this](int ms) { m_regie->setPosition(ms); });

    connect(ui->comboAudio, &QComboBox::currentIndexChanged, this, [this](int i) {
        if (i >= 0) m_regie->setPisteAudio(i);
    });
    connect(ui->comboSous, &QComboBox::currentIndexChanged, this, [this](int i) {
        if (i >= 0) m_regie->setPisteSousTitres(i - 1);   // 0 = « Aucun » -> -1
    });

    connect(m_regie, &Regie::pistesChanged, this, &MainWindow::majPistes);
    connect(m_regie, &Regie::positionChanged, this, &MainWindow::majPosition);
    connect(m_regie, &Regie::dureeFilmChanged, this, [this](qint64 d) {
        {
            const QSignalBlocker bloc(ui->sliderPosition);
            ui->sliderPosition->setRange(0, int(d));
        }
        majPosition(m_regie->position());
    });
    connect(m_regie, &Regie::erreur, this, [this](const QString &msg) {
        statusBar()->showMessage("⚠ " + msg, 10000);
    });
}

void MainWindow::connecterReglages()
{
    connect(ui->comboEcran, &QComboBox::currentIndexChanged, this, [this] {
        // on mémorise le NOM de l'écran (pas son numéro, qui change quand on débranche un écran)
        QSettings().setValue("ecranNom", ui->comboEcran->currentData().toString());
        placerAffichage();
    });
    connect(ui->checkFenetre, &QCheckBox::toggled, this, [this] { placerAffichage(); });
    connect(ui->btnActualiser, &QPushButton::clicked, this, [this] {
        remplirEcrans();
        placerAffichage();
    });
    connect(qGuiApp, &QGuiApplication::screenAdded, this, [this] { remplirEcrans(); placerAffichage(); });
    connect(qGuiApp, &QGuiApplication::screenRemoved, this, [this] { remplirEcrans(); placerAffichage(); });
}

// écran projecteur

void MainWindow::remplirEcrans()
{
    const QSignalBlocker blocker(ui->comboEcran);
    const QString voulu = QSettings().value("ecranNom").toString();

    ui->comboEcran->clear();
    const auto ecrans = QGuiApplication::screens();
    int choix = ecrans.size() > 1 ? 1 : 0;      // par défaut : le 2e écran s'il existe
    for (int i = 0; i < ecrans.size(); ++i) {
        const QScreen *e = ecrans.at(i);
        const QRect g = e->geometry();
        ui->comboEcran->addItem(QString("%1 : %2 (%3×%4)").arg(i + 1).arg(e->name()).arg(g.width()).arg(g.height()),
                                e->name());
        if (!voulu.isEmpty() && e->name() == voulu)
            choix = i;                          // l'écran mémorisé est de retour : on le reprend
    }
    ui->comboEcran->setCurrentIndex(choix);
}

void MainWindow::placerAffichage()
{
    if (!m_affichage)
        return;
    const auto ecrans = QGuiApplication::screens();
    if (ecrans.isEmpty())
        return;
    int i = ui->comboEcran->currentIndex();
    if (i < 0 || i >= ecrans.size())
        i = 0;
    QScreen *ecran = ecrans.at(i);
    const QRect g = ecran->geometry();

    const bool fenetre = ui->checkFenetre->isChecked() || ecrans.size() < 2;

    m_affichage->setScreen(ecran);
    if (fenetre) {
        m_affichage->showNormal();
        m_affichage->setCursor(Qt::ArrowCursor);
        m_affichage->setGeometry(g.x() + 60, g.y() + 60, 960, 540);
        m_affichage->show();
    } else {
        m_affichage->setGeometry(g);
        m_affichage->setCursor(Qt::BlankCursor);
        m_affichage->showFullScreen();
    }
}

// widgets -> Regie

void MainWindow::appliquerSeance()
{
    if (m_chargement || !m_regie)
        return;
    m_regie->definirSeance(ui->editTitre->text(), ui->editDuree->text(),
                           ui->comboLangue->currentText(), ui->timeDebut->time(),
                           ui->editMessage->text());
}

void MainWindow::appliquerSnacks()
{
    if (m_chargement)
        return;
    QVariantList liste;
    for (int r = 0; r < ui->tableSnacks->rowCount(); ++r) {
        const auto *n = ui->tableSnacks->item(r, 0);
        const auto *pr = ui->tableSnacks->item(r, 1);
        const auto *img = ui->tableSnacks->item(r, 2);
        if (!n || n->text().trimmed().isEmpty())
            continue;
        QVariantMap m;
        m["nom"] = n->text();
        m["prix"] = pr ? pr->text() : QString();
        m["img"] = (img && !img->text().isEmpty())
                       ? QUrl::fromLocalFile(img->text()).toString()
                       : QString();
        liste << m;
    }
    m_regie->definirSnacks(liste);
}

void MainWindow::appliquerTimer()
{
    m_regie->definirTimer(ui->spinMin->value() * 60 + ui->spinSec->value());
}

void MainWindow::ajouterSnack(const QString &nom, const QString &prix, const QString &image)
{
    const QSignalBlocker blocker(ui->tableSnacks);
    const int r = ui->tableSnacks->rowCount();
    ui->tableSnacks->insertRow(r);
    ui->tableSnacks->setItem(r, 0, new QTableWidgetItem(nom));
    ui->tableSnacks->setItem(r, 1, new QTableWidgetItem(prix));

    // Item invisible qui porte la valeur (chemin) : c'est lui que lit appliquerSnacks()
    auto *imgItem = new QTableWidgetItem(image);
    imgItem->setFlags(imgItem->flags() & ~Qt::ItemIsEditable);
    ui->tableSnacks->setItem(r, 2, imgItem);

    // Widget affiché par-dessus la cellule : nom du fichier + bouton
    auto *cellule = new QWidget;
    auto *ligne = new QHBoxLayout(cellule);
    ligne->setContentsMargins(4, 0, 4, 0);

    auto *label = new QLabel(image.isEmpty() ? "(aucune)" : QFileInfo(image).fileName());
    auto *bouton = new QPushButton("Trouver une image…");
    ligne->addWidget(label, 1);
    ligne->addWidget(bouton);

    connect(bouton, &QPushButton::clicked, this, [this, imgItem, label] {
        const QString chemin = choisirImageSnack(imgItem->tableWidget()
                                                 ? ui->tableSnacks->item(imgItem->row(), 0)->text() : QString());
        if (chemin.isEmpty())
            return;
        imgItem->setText(chemin);              // déclenche itemChanged -> appliquerSnacks()
        label->setText(QFileInfo(chemin).fileName());
    });

    ui->tableSnacks->setCellWidget(r, 2, cellule);
}

QString MainWindow::choisirImageSnack(const QString &nom)
{
    return QFileDialog::getOpenFileName(this, "Choisir l'image pour " + nom, QString(),
                                        "Images (*.png *.jpg *.jpeg *.webp)");
}

void MainWindow::choisirAffiche()
{
    const QString chemin = QFileDialog::getOpenFileName(
        this, "Choisir l'affiche", m_cheminAffiche, "Images (*.png *.jpg *.jpeg *.webp)");
    if (!chemin.isEmpty())
        definirAffiche(chemin);
}

void MainWindow::choisirFilm()
{
    const QString chemin = QFileDialog::getOpenFileName(
        this, "Choisir le film", m_cheminFilm,
        "Vidéos (*.mkv *.mp4 *.avi *.mov *.m4v);;Tous les fichiers (*)");
    if (!chemin.isEmpty())
        definirFilm(chemin);
}

void MainWindow::choisirLogo()
{
    const QString chemin = QFileDialog::getOpenFileName(
        this, "Choisir le logo", m_cheminLogo, "Images (*.png *.jpg *.jpeg *.webp *.svg)");
    if (!chemin.isEmpty())
        definirLogo(chemin);
}

void MainWindow::definirAffiche(const QString &chemin)
{
    m_cheminAffiche = chemin;
    const bool ok = !chemin.isEmpty() && QFileInfo::exists(chemin);
    ui->labelAffiche->setText(chemin.isEmpty() ? "(aucune)"
                              : ok ? QFileInfo(chemin).fileName()
                                   : "⚠ introuvable : " + QFileInfo(chemin).fileName());
    m_regie->definirAffiche(ok ? QUrl::fromLocalFile(chemin) : QUrl());
}

void MainWindow::definirLogo(const QString &chemin)
{
    m_cheminLogo = chemin;
    const bool ok = !chemin.isEmpty() && QFileInfo::exists(chemin);
    ui->labelLogo->setText(chemin.isEmpty() ? "(aucun)"
                           : ok ? QFileInfo(chemin).fileName()
                                : "⚠ introuvable : " + QFileInfo(chemin).fileName());
    m_regie->definirLogo(ok ? QUrl::fromLocalFile(chemin) : QUrl());
}

void MainWindow::definirFilm(const QString &chemin)
{
    m_cheminFilm = chemin;
    const bool ok = !chemin.isEmpty() && QFileInfo::exists(chemin);
    ui->labelFilm->setText(chemin.isEmpty() ? "(aucun)"
                           : ok ? QFileInfo(chemin).fileName()
                                : "⚠ introuvable : " + QFileInfo(chemin).fileName());
    m_regie->chargerFilm(ok ? QUrl::fromLocalFile(chemin) : QUrl());
}

// Regie -> widgets

void MainWindow::majScene()
{
    const QString s = m_regie->scene();
    ui->labelScene->setText("Scène : " + s.toUpper());
    if (auto *b = m_boutonsScene.value(s))
        b->setChecked(true);
}

void MainWindow::majPistes()
{
    const QSignalBlocker b1(ui->comboAudio);
    const QSignalBlocker b2(ui->comboSous);

    const QStringList audio = m_regie->pistesAudio();
    ui->comboAudio->clear();
    ui->comboAudio->addItems(audio);
    ui->comboAudio->setCurrentIndex(m_regie->pisteAudioActive());

    const QStringList sous = m_regie->pistesSousTitres();
    ui->comboSous->clear();
    ui->comboSous->addItem("Aucun");
    ui->comboSous->addItems(sous);
    ui->comboSous->setCurrentIndex(m_regie->pisteSousTitresActive() + 1);

    if (m_regie->filmCharge())
        statusBar()->showMessage(QString("Pistes audio : %1 | sous-titres : %2")
                                     .arg(audio.size()).arg(sous.size()), 8000);
}

void MainWindow::majPosition(qint64 ms)
{
    if (!ui->sliderPosition->isSliderDown()) {
        const QSignalBlocker bloc(ui->sliderPosition);    // sinon on "re-seek" nous-mêmes
        ui->sliderPosition->setValue(int(ms));
    }
    ui->labelPosition->setText(formatMs(ms) + " / " + formatMs(m_regie->dureeFilm()));
}

QString MainWindow::formatMs(qint64 ms)
{
    const qint64 s = qMax<qint64>(0, ms) / 1000;
    const qint64 h = s / 3600;
    const int m = int((s % 3600) / 60);
    const int sec = int(s % 60);
    return h > 0 ? QString("%1:%2:%3").arg(h).arg(m, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0'))
                 : QString("%1:%2").arg(m, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0'));
}

// sauvegarde

QJsonObject MainWindow::seanceEnJson() const
{
    QJsonArray snacks;
    for (int r = 0; r < ui->tableSnacks->rowCount(); ++r) {
        const auto texte = [this, r](int col) {
            const QTableWidgetItem *it = ui->tableSnacks->item(r, col);
            return it ? it->text() : QString();
        };
        snacks.append(QJsonObject{{"nom", texte(0)}, {"prix", texte(1)}, {"image", texte(2)}});
    }
    return QJsonObject{
        {"titre", ui->editTitre->text()},
        {"duree", ui->editDuree->text()},
        {"langue", ui->comboLangue->currentText()},
        {"debut", ui->timeDebut->time().toString("HH:mm")},
        {"message", ui->editMessage->text()},
        {"affiche", m_cheminAffiche},
        {"logo", m_cheminLogo},
        {"film", m_cheminFilm},
        {"timerMin", ui->spinMin->value()},
        {"timerSec", ui->spinSec->value()},
        {"autoFilm", ui->checkAutoFilm->isChecked()},
        {"afficherTimer", ui->checkAfficherTimer->isChecked()},
        {"snacks", snacks},
    };
}

void MainWindow::appliquerJson(const QJsonObject &o)
{
    m_chargement = true;
    ui->editTitre->setText(o.value("titre").toString());
    ui->editDuree->setText(o.value("duree").toString());
    ui->editMessage->setText(o.value("message").toString("Merci d'éteindre vos téléphones"));

    const QString langue = o.value("langue").toString();
    int i = ui->comboLangue->findText(langue);
    if (i < 0 && !langue.isEmpty()) {
        ui->comboLangue->addItem(langue);
        i = ui->comboLangue->count() - 1;
    }
    if (i >= 0)
        ui->comboLangue->setCurrentIndex(i);

    const QTime debut = QTime::fromString(o.value("debut").toString(), "HH:mm");
    if (debut.isValid())
        ui->timeDebut->setTime(debut);

    ui->spinMin->setValue(o.value("timerMin").toInt(10));
    ui->spinSec->setValue(o.value("timerSec").toInt(0));
    ui->checkAutoFilm->setChecked(o.value("autoFilm").toBool(true));
    ui->checkAfficherTimer->setChecked(o.value("afficherTimer").toBool(true));

    ui->tableSnacks->setRowCount(0);
    const QJsonArray snacks = o.value("snacks").toArray();
    for (const QJsonValue &v : snacks) {
        const QJsonObject s = v.toObject();
        ajouterSnack(s.value("nom").toString(), s.value("prix").toString(), s.value("image").toString());
    }
    m_chargement = false;

    definirAffiche(o.value("affiche").toString());
    definirLogo(o.value("logo").toString());
    definirFilm(o.value("film").toString());

    appliquerSeance();
    appliquerSnacks();
    appliquerTimer();
    m_regie->setAutoFilm(ui->checkAutoFilm->isChecked());
}

QString MainWindow::cheminDerniereSeance() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/derniere-seance.json";
}

void MainWindow::chargerDerniereSeance()
{
    QFile f(cheminDerniereSeance());
    if (f.open(QIODevice::ReadOnly)) {
        const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
        if (doc.isObject()) {
            appliquerJson(doc.object());
            return;
        }
    }
    ajouterSnack("Pop-corn", "0,10 €", "");     // tout premier lancement
}

void MainWindow::sauverSeance()
{
    const QString chemin = QFileDialog::getSaveFileName(
        this, "Enregistrer la séance", QDir::homePath() + "/seance.json", "Séances (*.json)");
    if (chemin.isEmpty())
        return;
    QFile f(chemin);
    if (!f.open(QIODevice::WriteOnly)) {
        statusBar()->showMessage("⚠ Impossible d'écrire " + chemin, 8000);
        return;
    }
    f.write(QJsonDocument(seanceEnJson()).toJson());
    statusBar()->showMessage("Séance enregistrée.", 5000);
}

void MainWindow::chargerSeance()
{
    const QString chemin = QFileDialog::getOpenFileName(
        this, "Charger une séance", QDir::homePath(), "Séances (*.json)");
    if (chemin.isEmpty())
        return;
    QFile f(chemin);
    if (!f.open(QIODevice::ReadOnly)) {
        statusBar()->showMessage("⚠ Impossible de lire " + chemin, 8000);
        return;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject()) {
        statusBar()->showMessage("⚠ Fichier de séance invalide.", 8000);
        return;
    }
    appliquerJson(doc.object());
    statusBar()->showMessage("Séance chargée.", 5000);
}

// réglages appli
void MainWindow::chargerReglages()
{
    QSettings s;
    ui->sliderVolume->setValue(s.value("volume", 80).toInt());
    ui->checkFenetre->setChecked(s.value("fenetre", QGuiApplication::screens().size() < 2).toBool());
}

void MainWindow::enregistrerReglages() const
{
    QSettings s;
    s.setValue("volume", ui->sliderVolume->value());
    s.setValue("fenetre", ui->checkFenetre->isChecked());
}
