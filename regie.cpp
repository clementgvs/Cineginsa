#include "regie.h"

#include <QAudioOutput>
#include <QMediaPlayer>
#include <QtMath>

namespace {
    const QStringList kOrdre = {"attente", "timer", "film", "fin"};
}

Regie::Regie(QObject *parent) : QObject(parent)
{
    m_player = new QMediaPlayer(this);
    m_audio = new QAudioOutput(this);
    m_audio->setVolume(0.8f);
    m_player->setAudioOutput(m_audio);

    m_tick.setInterval(200);
    connect(&m_tick, &QTimer::timeout, this, &Regie::tick);
    majTexte();
}

// scènes

void Regie::setScene(const QString &scene)
{
    if (scene == m_scene)
        return;

    const bool etaitFilm = (m_scene == "film");
    m_scene = scene;

    if (scene == "film") {
        if (!m_player->source().isEmpty())
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

void Regie::chargerFilm(const QUrl &url)
{
    m_player->setSource(url);
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
        if (m_autoFilm)
            setScene("film");     // lance aussi la lecture
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
