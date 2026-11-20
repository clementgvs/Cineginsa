#pragma once
#include <QElapsedTimer>
#include <QObject>
#include <QStringList>
#include <QTime>
#include <QTimer>
#include <QUrl>
#include <QVariantList>

class QMediaPlayer;
class QAudioOutput;

// Cœur de la régie : état de la séance, scène en cours, timer, lecteur vidéo.
// Les Widgets l'appellent, le QML le lit (context property "regie").
class Regie : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString scene READ scene NOTIFY sceneChanged)
    Q_PROPERTY(QString titre READ titre NOTIFY seanceChanged)
    Q_PROPERTY(QString duree READ duree NOTIFY seanceChanged)
    Q_PROPERTY(QString langue READ langue NOTIFY seanceChanged)
    Q_PROPERTY(QString heureDebut READ heureDebut NOTIFY seanceChanged)
    Q_PROPERTY(QString messageInfos READ messageInfos NOTIFY seanceChanged)
    Q_PROPERTY(QUrl affiche READ affiche NOTIFY seanceChanged)
    Q_PROPERTY(QUrl logo READ logo NOTIFY seanceChanged)
    Q_PROPERTY(QVariantList snacks READ snacks NOTIFY seanceChanged)
    Q_PROPERTY(QString compteARebours READ compteARebours NOTIFY compteChanged)
    Q_PROPERTY(double timerProgression READ timerProgression NOTIFY compteChanged)
    Q_PROPERTY(bool timerActif READ timerActif NOTIFY timerChanged)

public:
    explicit Regie(QObject *parent = nullptr);

    QString scene() const { return m_scene; }
    QString titre() const { return m_titre; }
    QString duree() const { return m_duree; }
    QString langue() const { return m_langue; }
    QString heureDebut() const { return m_debut.toString("HH:mm"); }
    QString messageInfos() const { return m_message; }
    QUrl affiche() const { return m_affiche; }
    QUrl logo() const { return m_logo; }
    QVariantList snacks() const { return m_snacks; }
    QString compteARebours() const { return m_compte; }
    double timerProgression() const;
    bool timerActif() const { return m_actif; }
    int timerDuree() const { return m_timerDuree; }
    QTime debut() const { return m_debut; }

    QMediaPlayer *player() const { return m_player; }
    QAudioOutput *audio() const { return m_audio; }

    void definirSeance(const QString &titre, const QString &duree, const QString &langue,
                       const QTime &debut, const QString &message);
    void definirSnacks(const QVariantList &snacks);
    void definirAffiche(const QUrl &url);
    void definirLogo(const QUrl &url);
    void chargerFilm(const QUrl &url);

    // timer
    void definirTimer(int secondes);          // durée configurée (remet le timer à cette valeur s'il est arrêté)
    void setAutoFilm(bool oui) { m_autoFilm = oui; }
    bool autoFilm() const { return m_autoFilm; }

public slots:
    void setScene(const QString &scene);      // attente, timer, film, fin
    Q_INVOKABLE void sceneSuivante();         // aussi appelable depuis le QML
    Q_INVOKABLE void scenePrecedente();

    void lancerTimer();
    void pauseTimer();
    void basculerTimer();                     // lancer <-> pause
    void reinitialiserTimer();

signals:
    void sceneChanged();
    void seanceChanged();
    void compteChanged();
    void timerChanged();                      // actif / arrêté
    void timerTermine();

private:
    void tick();
    void majTexte();

    QString m_scene;   // vide au départ : la 1re scène émet bien le signal
    QString m_titre, m_duree, m_langue, m_message;
    QTime m_debut = QTime(20, 0);
    QUrl m_affiche, m_logo;
    QVariantList m_snacks;

    // timer
    int m_timerDuree = 600;           // secondes
    qint64 m_restantMs = 600000;
    qint64 m_baseMs = 0;
    bool m_actif = false;
    bool m_autoFilm = true;
    QString m_compte = "10:00";
    QElapsedTimer m_chrono;
    QTimer m_tick;

    QMediaPlayer *m_player = nullptr;
    QAudioOutput *m_audio = nullptr;
};
