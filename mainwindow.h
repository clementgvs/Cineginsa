#pragma once
#include <QJsonObject>
#include <QMainWindow>
#include <QMap>

class QButtonGroup;
class QPushButton;
class QQuickView;
class Regie;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    // construction
    void initialiserInterface();
    void creerAffichage();
    void creerRaccourcis();
    void connecterSeance();
    void connecterSnacks();
    void connecterPilotage();
    void connecterLecteur();
    void connecterReglages();

    // affichage projecteur
    void remplirEcrans();
    void placerAffichage();

    // widgets -> Regie
    void appliquerSeance();
    void appliquerSnacks();
    void appliquerTimer();
    void ajouterSnack(const QString &nom, const QString &prix, const QString &image = QString());
    QString choisirImageSnack(const QString &nom);
    void choisirAffiche();
    void choisirFilm();
    void choisirLogo();
    void definirAffiche(const QString &chemin);
    void definirLogo(const QString &chemin);
    void definirFilm(const QString &chemin);

    // Regie -> widgets
    void majScene();
    void majPistes();
    void majPosition(qint64 ms);

    // sauvegarde
    QJsonObject seanceEnJson() const;
    void appliquerJson(const QJsonObject &o);
    QString cheminDerniereSeance() const;
    void chargerDerniereSeance();
    void sauverSeance();
    void chargerSeance();
    void chargerReglages();
    void enregistrerReglages() const;

    static QString formatMs(qint64 ms);

    Ui::MainWindow *ui = nullptr;
    Regie *m_regie = nullptr;
    QQuickView *m_affichage = nullptr;
    QButtonGroup *m_groupeScenes = nullptr;
    QMap<QString, QPushButton *> m_boutonsScene;

    QString m_cheminAffiche;
    QString m_cheminFilm;
    QString m_cheminLogo;
    bool m_chargement = false;     // vrai pendant le chargement d'un fichier de séance
};
