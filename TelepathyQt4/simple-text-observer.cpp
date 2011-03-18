/**
 * This file is part of TelepathyQt4
 *
 * @copyright Copyright (C) 2011 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2011 Nokia Corporation
 * @license LGPL 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <TelepathyQt4/SimpleTextObserver>
#include "TelepathyQt4/simple-text-observer-internal.h"

#include "TelepathyQt4/_gen/simple-text-observer.moc.hpp"
#include "TelepathyQt4/_gen/simple-text-observer-internal.moc.hpp"

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/ChannelClassSpec>
#include <TelepathyQt4/ChannelClassSpecList>
#include <TelepathyQt4/Contact>
#include <TelepathyQt4/Message>
#include <TelepathyQt4/PendingReady>

namespace Tp
{

uint SimpleTextObserver::Private::numObservers = 0;

SimpleTextObserver::Private::Private(const ClientRegistrarPtr &cr, const AccountPtr &account,
        const QString &contactIdentifier)
    : cr(cr),
      account(account),
      contactIdentifier(contactIdentifier)
{
}

SimpleTextObserver::Private::~Private()
{
    for (QHash<TextChannelPtr, TextChannelWrapper*>::iterator i = channels.begin();
            i != channels.end();) {
        delete i.value();
    }
}

SimpleTextObserver::Private::TextChannelWrapper::TextChannelWrapper(const TextChannelPtr &channel)
    : mChannel(channel)
{
    connect(channel.data(),
            SIGNAL(invalidated(Tp::PendingOperation*)),
            SLOT(onChannelInvalidated()));

    Features features = TextChannel::FeatureMessageQueue | TextChannel::FeatureMessageSentSignal;
    if (!channel->isReady(features)) {
        // The channel factory passed to the Account used by SimpleTextObserver does not contain the
        // neeeded features, request them
        connect(channel->becomeReady(features),
                SIGNAL(finished(Tp::PendingOperation*)),
                SLOT(onChannelReady()));
    } else {
        onChannelReady();
    }
}

void SimpleTextObserver::Private::TextChannelWrapper::onChannelInvalidated()
{
    emit channelInvalidated(mChannel);
}

void SimpleTextObserver::Private::TextChannelWrapper::onChannelReady()
{
    connect(mChannel.data(),
            SIGNAL(messageSent(Tp::Message,Tp::MessageSendingFlags,QString)),
            SLOT(onChannelMessageSent(Tp::Message,Tp::MessageSendingFlags,QString)));
    connect(mChannel.data(),
            SIGNAL(messageReceived(Tp::ReceivedMessage)),
            SLOT(onChannelMessageReceived(Tp::ReceivedMessage)));

    foreach (const ReceivedMessage &message, mChannel->messageQueue()) {
        onChannelMessageReceived(message);
    }
}

void SimpleTextObserver::Private::TextChannelWrapper::onChannelMessageSent(
        const Tp::Message &message, Tp::MessageSendingFlags flags,
        const QString &sentMessageToken)
{
    emit channelMessageSent(message, flags, sentMessageToken, mChannel);
}

void SimpleTextObserver::Private::TextChannelWrapper::onChannelMessageReceived(
        const Tp::ReceivedMessage &message)
{
    emit channelMessageReceived(message, mChannel);
}

SimpleTextObserverPtr SimpleTextObserver::create(const AccountPtr &account)
{
    return create(account, QString());
}

SimpleTextObserverPtr SimpleTextObserver::create(const AccountPtr &account,
        const ContactPtr &contact)
{
    if (contact) {
        return create(account, contact->id());
    }
    return create(account, QString());
}

SimpleTextObserverPtr SimpleTextObserver::create(const AccountPtr &account,
        const QString &contactIdentifier)
{
    QVariantMap additionalProperties;
    if (!contactIdentifier.isEmpty()) {
        QVariantMap additionalProperties;
        additionalProperties.insert(TP_QT4_IFACE_CHANNEL + QLatin1String(".TargetID"),
                contactIdentifier);
    }
    ChannelClassSpec channelFilter = ChannelClassSpec::textChat(additionalProperties);

    ClientRegistrarPtr cr = ClientRegistrar::create(
            Private::FakeAccountFactory::create(account),
            account->connectionFactory(),
            account->channelFactory(),
            account->contactFactory());

    SimpleTextObserverPtr observer = SimpleTextObserverPtr(
            new SimpleTextObserver(cr, ChannelClassSpecList() << channelFilter,
                account, contactIdentifier));

    QString observerName = QString(QLatin1String("TpQt4STO_%1_%2"))
        .arg(account->dbusConnection().baseService()
            .replace(QLatin1String(":"), QLatin1String("_"))
            .replace(QLatin1String("."), QLatin1String("_")))
        .arg(Private::numObservers++);
    if (!cr->registerClient(observer, observerName, false)) {
        warning() << "Unable to register observer" << observerName;
        return SimpleTextObserverPtr();
    }

    return observer;
}

SimpleTextObserver::SimpleTextObserver(const ClientRegistrarPtr &cr,
        const ChannelClassSpecList &channelFilter,
        const AccountPtr &account, const QString &contactIdentifier)
    : QObject(),
      AbstractClientObserver(channelFilter, true),
      mPriv(new Private(cr, account, contactIdentifier))
{
}

SimpleTextObserver::~SimpleTextObserver()
{
    delete mPriv;
}

void SimpleTextObserver::observeChannels(
        const MethodInvocationContextPtr<> &context,
        const AccountPtr &account,
        const ConnectionPtr &connection,
        const QList<ChannelPtr> &channels,
        const ChannelDispatchOperationPtr &dispatchOperation,
        const QList<ChannelRequestPtr> &requestsSatisfied,
        const ObserverInfo &observerInfo)
{
    if (account != mPriv->account) {
        context->setFinished();
        return;
    }

    foreach (const ChannelPtr &channel, channels) {
        TextChannelPtr textChannel = TextChannelPtr::qObjectCast(channel);
        if (mPriv->channels.contains(textChannel)) {
            // we are already observing this channel
            continue;
        }

        // this shouldn't happen, but in any case
        if (!textChannel || !textChannel->isValid()) {
            continue;
        }

        Private::TextChannelWrapper *wrapper = new Private::TextChannelWrapper(textChannel);
        mPriv->channels.insert(textChannel, wrapper);
        connect(wrapper,
                SIGNAL(channelInvalidated(Tp::TextChannelPtr)),
                SLOT(onChannelInvalidated(Tp::TextChannelPtr)));
        connect(wrapper,
                SIGNAL(channelMessageSent(Tp::Message,Tp::MessageSendingFlags,QString,Tp::TextChannelPtr)),
                SLOT(onChannelMessageSent(Tp::Message,Tp::MessageSendingFlags,QString,Tp::TextChannelPtr)));
        connect(wrapper,
                SIGNAL(channelMessageReceived(Tp::ReceivedMessage,Tp::TextChannelPtr)),
                SLOT(onChannelMessageReceived(Tp::ReceivedMessage,Tp::TextChannelPtr)));
    }

    context->setFinished();
}

void SimpleTextObserver::onChannelInvalidated(const TextChannelPtr &textChannel)
{
    delete mPriv->channels.take(textChannel);
}

void SimpleTextObserver::onChannelMessageSent(const Message &message, MessageSendingFlags flags,
        const QString &sentMessageToken, const TextChannelPtr &textChannel)
{
    emit messageSent(message, flags, sentMessageToken, textChannel);
}

void SimpleTextObserver::onChannelMessageReceived(const ReceivedMessage &message,
        const TextChannelPtr &textChannel)
{
    emit messageReceived(message, textChannel);
}

} // Tp
