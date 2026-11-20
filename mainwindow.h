#pragma once
#include <QMainWindow>
#include <QMap>

class Regie;
class QButtonGroup;
class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QQuickView;
class QSlider;
class QSpinBox;
class QTableWidget;
class QTimeEdit;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    // construction de l'interface
    void construireInterface();
    QWidget *ongletSeance();
    QWidget *ongletSnacks();
    QWidget *ongletPilotage();
    QWidget *ongletReglages();

    // affichage projecteur
    void creerAffichage();
    void placerAffichage();

    // logique
    void brancherLecteur();
    void appliquerSeance();
    void appliquerSnacks();
    void ajouterSnack(const QString &nom, const QString &prix, const QString &imageUrl = QString());
    QString choisirImageSnack(const QString & nom);
    void choisirAffiche();
    void choisirFilm();
    void choisirLogo();
    void appliquerTimer();
    void majPistes();
    void remplirEcrans();
    void chargerReglages();
    void enregistrerReglages();
    static QString formatMs(qint64 ms);

    Regie *m_regie = nullptr;
    QQuickView *m_affichage = nullptr;

    // onglet Séance
    QLineEdit *m_editTitre = nullptr;
    QLineEdit *m_editDuree = nullptr;
    QComboBox *m_comboLangue = nullptr;
    QTimeEdit *m_timeDebut = nullptr;
    QLineEdit *m_editMessage = nullptr;
    QLabel *m_labelAffiche = nullptr;
    QLabel *m_labelFilm = nullptr;
    QLabel *m_labelLogo = nullptr;

    // onglet Snacks
    QTableWidget *m_tableSnacks = nullptr;

    // onglet Pilotage
    QMap<QString, QPushButton *> m_boutonsScene;
    QButtonGroup *m_groupeScenes = nullptr;
    QLabel *m_labelScene = nullptr;
    QLabel *m_labelTimer = nullptr;
    QLabel *m_labelPosition = nullptr;
    QSpinBox *m_spinMin = nullptr;
    QSpinBox *m_spinSec = nullptr;
    QPushButton *m_btnTimer = nullptr;
    QCheckBox *m_checkAutoFilm = nullptr;
    QCheckBox *m_checkAfficherTimer = nullptr;
    QSlider *m_sliderPosition = nullptr;
    QSlider *m_sliderVolume = nullptr;
    QComboBox *m_comboAudio = nullptr;
    QComboBox *m_comboSous = nullptr;

    // onglet Réglages
    QComboBox *m_comboEcran = nullptr;
    QCheckBox *m_checkFenetre = nullptr;

    QString m_cheminAffiche;
    QString m_cheminFilm;
    QString m_cheminLogo;
};
