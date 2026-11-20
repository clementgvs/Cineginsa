#include "regie.h"

#include <QAudio>
#include <QAudioOutput>
#include <QFileInfo>
#include <QLocale>
#include <QMediaMetaData>
#include <QMediaPlayer>
#include <QtMath>

namespace {
    const QStringList kOrdre = {"attente", "timer", "film", "fin"};

    QString nomPiste(const QMediaMetaData &md, int i, const QString &defaut)
    {
        QStringList parts;
        const QVariant lang = md.value(QMediaMetaData::Language);
        if (lang.isValid()) {
            const auto l = lang.value<QLocale::Language>();
            if (l != QLocale::AnyLanguage && l != QLocale::C) {
                QString nom = QLocale(l).nativeLanguageName();
                if (!nom.isEmpty()) {
                    nom[0] = nom.at(0).toUpper();
                    parts << nom;
                }
            }
        }
        const QString titre = md.stringValue(QMediaMetaData::Title);
        if (!titre.isEmpty() && !parts.contains(titre, Qt::CaseInsensitive))
            parts << titre;

        return parts.isEmpty() ? QString("%1 %2").arg(defaut).arg(i + 1)
                               : QString("%1. %2").arg(i + 1).arg(parts.join(" – "));
    }

    QStringList nomsPistes(const QList<QMediaMetaData> &pistes, const QString &defaut)
    {
        QStringList noms;
        for (int i = 0; i < pistes.size(); ++i)
            noms << nomPiste(pistes.at(i), i, defaut);
        return noms;
    }
}

Regie::Regie(QObject *parent) : QObject(parent)
{
    m_player = new QMediaPlayer(this);
    m_audio = new QAudioOutput(this);
    m_player->setAudioOutput(m_audio);
    setVolume(80);

    m_tick.setInterval(200);
    connect(&m_tick, &QTimer::timeout, this, &Regie::tick);
    majTexte();

    connect(m_player, &QMediaPlayer::positionChanged, this, &Regie::positionChanged);
    connect(m_player, &QMediaPlayer::durationChanged, this, &Regie::dureeFilmChanged);
    connect(m_player, &QMediaPlayer::tracksChanged, this, &Regie::pistesChanged);
    connect(m_player, &QMediaPlayer::playbackStateChanged, this,
            [this](QMediaPlayer::PlaybackState e) { emit lectureChanged(e == QMediaPlayer::PlayingState); });
    connect(m_player, &QMediaPlayer::errorOccurred, this,
            [this](QMediaPlayer::Error, const QString &msg) { emit erreur("Erreur lecteur : " + msg); });
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus s) {
        if (s == QMediaPlayer::InvalidMedia) {
            m_filmCharge = false;
            emit erreur("Le film ne peut pas être lu (format ou fichier invalide).");
        }
    });
}

// scènes

void Regie::setScene(const QString &scene)
{
    if (scene == m_scene)
        return;

    if (scene == "film" && !m_filmCharge) {
        emit erreur("Impossible de lancer le film : aucun fichier valide n'est chargé (onglet Séance).");
        return;
    }

    const bool etaitFilm = (m_scene == "film");
    m_scene = scene;

    if (scene == "film") {
        m_player->play();
    } else if (etaitFilm) {
        m_player->pause();
    }
    emit sceneChanged();
}

void Regie::sceneSuivante()
{
    const int i = kOrdre.indexOf(m_scene);
    if (i >= 0 && i < kOrdre.size() - 1)
        setScene(kOrdre.at(i + 1));
}

void Regie::scenePrecedente()
{
    const int i = kOrdre.indexOf(m_scene);
    if (i > 0)
        setScene(kOrdre.at(i - 1));
}

// séance

void Regie::definirSeance(const QString &titre, const QString &duree, const QString &langue,
                          const QTime &debut, const QString &message)
{
    m_titre = titre;
    m_duree = duree;
    m_langue = langue;
    m_debut = debut;
    m_message = message;
    emit seanceChanged();
}

void Regie::definirSnacks(const QVariantList &snacks)
{
    m_snacks = snacks;
    emit seanceChanged();
}

void Regie::definirAffiche(const QUrl &url)
{
    m_affiche = url;
    emit seanceChanged();
}

void Regie::definirLogo(const QUrl &url)
{
    m_logo = url;
    emit seanceChanged();
}

// film / lecteur

void Regie::chargerFilm(const QUrl &url)
{
    m_filmCharge = !url.isEmpty() && (!url.isLocalFile() || QFileInfo::exists(url.toLocalFile()));
    m_player->setSource(m_filmCharge ? url : QUrl());
    emit pistesChanged();
}

void Regie::basculerLecture()
{
    if (!m_filmCharge) {
        emit erreur("Aucun film chargé.");
        return;
    }
    if (m_player->playbackState() == QMediaPlayer::PlayingState)
        m_player->pause();
    else
        m_player->play();
}

void Regie::sauter(int secondes)
{
    setPosition(m_player->position() + qint64(secondes) * 1000);
}

void Regie::setPosition(qint64 ms)
{
    m_player->setPosition(qBound<qint64>(0, ms, qMax<qint64>(0, m_player->duration())));
}

void Regie::setVolume(int pourcent)
{
    const qreal curseur = qBound(0, pourcent, 100) / 100.0;
    m_audio->setVolume(float(QAudio::convertVolume(curseur, QAudio::LogarithmicVolumeScale,
                                                   QAudio::LinearVolumeScale)));
}

qint64 Regie::position() const { return m_player->position(); }
qint64 Regie::dureeFilm() const { return m_player->duration(); }
bool Regie::enLecture() const { return m_player->playbackState() == QMediaPlayer::PlayingState; }

QStringList Regie::pistesAudio() const { return nomsPistes(m_player->audioTracks(), "Piste"); }
int Regie::pisteAudioActive() const { return m_player->activeAudioTrack(); }
void Regie::setPisteAudio(int index) { m_player->setActiveAudioTrack(index); }

QStringList Regie::pistesSousTitres() const { return nomsPistes(m_player->subtitleTracks(), "Sous-titres"); }
int Regie::pisteSousTitresActive() const { return m_player->activeSubtitleTrack(); }
void Regie::setPisteSousTitres(int index) { m_player->setActiveSubtitleTrack(index); }

void Regie::attacherSortieVideo(QObject *sortie)
{
    m_player->setVideoOutput(sortie);
}

// timer

void Regie::definirTimer(int secondes)
{
    m_timerDuree = qMax(0, secondes);
    if (!m_actif) {
        m_restantMs = qint64(m_timerDuree) * 1000;
        majTexte();
    }
}

void Regie::lancerTimer()
{
    if (m_actif)
        return;
    if (m_restantMs <= 0)
        m_restantMs = qint64(m_timerDuree) * 1000;
    if (m_restantMs <= 0)
        return;

    m_baseMs = m_restantMs;
    m_chrono.restart();
    m_actif = true;
    m_tick.start();
    emit timerChanged();
}

void Regie::pauseTimer()
{
    if (!m_actif)
        return;
    m_restantMs = qMax<qint64>(0, m_baseMs - m_chrono.elapsed());
    m_actif = false;
    m_tick.stop();
    majTexte();
    emit timerChanged();
}

void Regie::basculerTimer()
{
    if (m_actif)
        pauseTimer();
    else
        lancerTimer();
}

void Regie::reinitialiserTimer()
{
    const bool etait = m_actif;
    m_actif = false;
    m_tick.stop();
    m_restantMs = qint64(m_timerDuree) * 1000;
    majTexte();
    if (etait)
        emit timerChanged();
}

void Regie::tick()
{
    if (!m_actif)
        return;

    const qint64 ms = m_baseMs - m_chrono.elapsed();
    if (ms <= 0) {
        m_restantMs = 0;
        m_actif = false;
        m_tick.stop();
        majTexte();
        emit timerChanged();
        emit timerTermine();
        if (m_autoFilm) {
            if (m_filmCharge)
                setScene("film");
            else
                emit erreur("Timer terminé, mais aucun film valide n'est chargé : la scène film n'est pas lancée.");
        }
        return;
    }
    m_restantMs = ms;
    majTexte();
}

double Regie::timerProgression() const
{
    if (m_timerDuree <= 0)
        return 0.0;
    return qBound(0.0, double(m_restantMs) / (double(m_timerDuree) * 1000.0), 1.0);
}

void Regie::majTexte()
{
    const qint64 s = (m_restantMs + 999) / 1000;     // arrondi vers le haut : 00:01 jusqu'au bout
    const int h = int(s / 3600);
    const int m = int((s % 3600) / 60);
    const int sec = int(s % 60);

    m_compte = h > 0
        ? QString("%1:%2:%3").arg(h).arg(m, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0'))
        : QString("%1:%2").arg(m, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0'));
    emit compteChanged();
}
