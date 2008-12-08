#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include <QtDBus/QtDBus>

#include <TelepathyQt4/Client/ConnectionManager>

#include <tests/pinocchio/lib.h>

using namespace Telepathy::Client;

class TestCmBasics : public PinocchioTest
{
    Q_OBJECT

private:
    Telepathy::Client::ConnectionManager* mCM;

protected Q_SLOTS:
    void onCmReady(ConnectionManager*);

private Q_SLOTS:
    void initTestCase();
    void init();

    void testBasics();

    void cleanup();
    void cleanupTestCase();
};


void TestCmBasics::initTestCase()
{
    initTestCaseImpl();

    // Wait for the CM to start up
    QVERIFY(waitForPinocchio(5000));
}


void TestCmBasics::init()
{
    initImpl();
}


void TestCmBasics::onCmReady(ConnectionManager* it)
{
    if (mCM != it) {
        qWarning() << "Got the wrong CM pointer";
        mLoop->exit(1);
        return;
    }

    mLoop->exit(0);
}


void TestCmBasics::testBasics()
{
    mCM = new ConnectionManager("pinocchio");
    QCOMPARE(mCM->isReady(), false);

    connect(mCM, SIGNAL(ready(ConnectionManager*)),
            SLOT(onCmReady(ConnectionManager*)));
    QCOMPARE(mLoop->exec(), 0);
    QCOMPARE(mCM->isReady(), true);
    disconnect(mCM, SIGNAL(ready(ConnectionManager*)),
            this, SLOT(onCmReady(ConnectionManager*)));

    QCOMPARE(mCM->interfaces(), QStringList());
    QCOMPARE(mCM->supportedProtocols(), QStringList() << "dummy");
}


void TestCmBasics::cleanup()
{
    if (mCM != NULL) {
        delete mCM;
        mCM = NULL;
    }
    cleanupImpl();
}


void TestCmBasics::cleanupTestCase()
{
    cleanupTestCaseImpl();
}


QTEST_MAIN(TestCmBasics)
#include "_gen/cm-basics.cpp.moc.hpp"
