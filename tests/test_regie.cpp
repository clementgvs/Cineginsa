#include <QSignalSpy>
#include <QtTest>

#include "../regie.h"

class RegieTests : public QObject
{
    Q_OBJECT

private slots:
    void sceneSuivantePartDeLAttente();
    void impossibleDePasserAuFilmSansMedia();
    void timerZeroNeDemarrePas();
    void timerAfficheLeTempsConfigure();
};

void RegieTests::sceneSuivantePartDeLAttente()
{
    Regie regie;

    QCOMPARE(regie.scene(), QString());
    regie.sceneSuivante();
    QCOMPARE(regie.scene(), QString("attente"));
}

void RegieTests::impossibleDePasserAuFilmSansMedia()
{
    Regie regie;
    QSignalSpy erreur(&regie, &Regie::erreur);

    regie.setScene("attente");
    regie.setScene("film");

    QCOMPARE(regie.scene(), QString("attente"));
    QCOMPARE(erreur.count(), 1);
}

void RegieTests::timerZeroNeDemarrePas()
{
    Regie regie;

    regie.definirTimer(0);
    regie.lancerTimer();

    QCOMPARE(regie.timerActif(), false);
    QCOMPARE(regie.compteARebours(), QString("00:00"));
}

void RegieTests::timerAfficheLeTempsConfigure()
{
    Regie regie;

    regie.definirTimer(61);
    QCOMPARE(regie.compteARebours(), QString("01:01"));

    regie.reinitialiserTimer();
    QCOMPARE(regie.compteARebours(), QString("01:01"));
    QCOMPARE(regie.timerProgression(), 1.0);
}

QTEST_GUILESS_MAIN(RegieTests)
#include "test_regie.moc"
