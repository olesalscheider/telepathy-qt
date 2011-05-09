#include "fake-handler-manager.h"

#include "TelepathyQt4/_gen/fake-handler-manager.moc.hpp"

#include <TelepathyQt4/Channel>

using namespace Tp;

FakeHandler::FakeHandler()
: QObject(0)
, mNumChannels(0)
{
}

void FakeHandler::addChannel(const ChannelPtr &channel,
                             const ClientRegistrarPtr &registrar)
{
    if (mNumChannels <= 0) {
        mRegistrar = registrar;
    }
    mNumChannels++;
    connect(channel.data(), SIGNAL(invalidated(Tp::DBusProxy*, QString, QString)),
            this, SLOT(onChannelInvalidated()));
    connect(channel.data(), SIGNAL(destroyed()),
            this, SLOT(onChannelDestroyed()));
}

void FakeHandler::onChannelInvalidated()
{
    disconnect(sender(), SIGNAL(destroyed()),
            this, SLOT(onChannelDestroyed()));
    onChannelDestroyed();
}

void FakeHandler::onChannelDestroyed()
{
    mNumChannels--;
    if (mNumChannels <= 0) {
        deleteLater();
    }
}

FakeHandlerManager::FakeHandlerManager()
: QObject(0)
{
}

void FakeHandlerManager::registerHandler(const QPair<QString, QString> &dbusConnection,
                                         const ChannelPtr &channel,
                                         const ClientRegistrarPtr &registrar)
{
    FakeHandler *handler;

    QWeakPointer<FakeHandler> weakFakeHandlerPointer = mFakeHandlers[dbusConnection];
    if (weakFakeHandlerPointer.isNull()) {
        handler = new FakeHandler;
        mFakeHandlers.insert(dbusConnection, QWeakPointer<FakeHandler>(handler));
    } else {
        handler = weakFakeHandlerPointer.data();
    }
    handler->addChannel(channel, registrar);
}

FakeHandlerManager *FakeHandlerManager::instance()
{
    static FakeHandlerManager sInstance;
    return &sInstance;
}
