/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2013 Matthias Gehre <gehre.matthias@gmail.com>
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

#ifndef _TelepathyQt_base_channel_h_HEADER_GUARD_
#define _TelepathyQt_base_channel_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt/DBusService>
#include <TelepathyQt/Global>
#include <TelepathyQt/Types>
#include <TelepathyQt/Callbacks>

#include <QDBusConnection>

class QString;

namespace Tp
{

class BaseChannel : public DBusService
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseChannel)

public:
    static BaseChannelPtr create(BaseConnection* connection, const QString &channelType,
                                 uint targetHandle, uint targetHandleType) {
        return BaseChannelPtr(new BaseChannel(QDBusConnection::sessionBus(), connection,
                                              channelType, targetHandle, targetHandleType));
    }

    virtual ~BaseChannel();

    QVariantMap immutableProperties() const;
    bool registerObject(DBusError *error = NULL);
    virtual QString uniqueName() const;

    QString channelType() const;
    QList<AbstractChannelInterfacePtr> interfaces() const;
    AbstractChannelInterfacePtr interface(const QString &interfaceName) const;
    uint targetHandle() const;
    QString targetID() const;
    uint targetHandleType() const;
    bool requested() const;
    uint initiatorHandle() const;
    QString initiatorID() const;
    Tp::ChannelDetails details() const;

    void setInitiatorHandle(uint initiatorHandle);
    void setInitiatorID(const QString &initiatorID);
    void setTargetID(const QString &targetID);
    void setRequested(bool requested);

    bool plugInterface(const AbstractChannelInterfacePtr &interface);

Q_SIGNALS:
    void closed();
protected:
    BaseChannel(const QDBusConnection &dbusConnection, BaseConnection* connection,
                const QString &channelType, uint targetHandle, uint targetHandleType);
    virtual bool registerObject(const QString &busName, const QString &objectPath,
                                DBusError *error);
private:
    class Adaptee;
    friend class Adaptee;
    class Private;
    friend class Private;
    Private *mPriv;
};

class TP_QT_EXPORT AbstractChannelInterface : public AbstractDBusServiceInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(AbstractChannelInterface)

public:
    AbstractChannelInterface(const QString &interfaceName);
    virtual ~AbstractChannelInterface();

private:
    friend class BaseChannel;

    class Private;
    friend class Private;
    Private *mPriv;
};

class TP_QT_EXPORT BaseChannelTextType : public AbstractChannelInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseChannelTextType)

public:
    static BaseChannelTextTypePtr create(BaseChannel* channel) {
        return BaseChannelTextTypePtr(new BaseChannelTextType(channel));
    }
    template<typename BaseChannelTextTypeSubclass>
    static SharedPtr<BaseChannelTextTypeSubclass> create(BaseChannel* channel) {
        return SharedPtr<BaseChannelTextTypeSubclass>(
                   new BaseChannelTextTypeSubclass(channel));
    }

    typedef Callback2<QDBusObjectPath, const QVariantMap&, DBusError*> CreateChannelCallback;
    CreateChannelCallback createChannel;

    typedef Callback2<bool, const QVariantMap&, DBusError*> EnsureChannelCallback;
    EnsureChannelCallback ensureChannel;

    virtual ~BaseChannelTextType();

    QVariantMap immutableProperties() const;

    Tp::RequestableChannelClassList requestableChannelClasses;

    typedef Callback1<void, QString> MessageAcknowledgedCallback;
    void setMessageAcknowledgedCallback(const MessageAcknowledgedCallback &cb);

    Tp::MessagePartListList pendingMessages();

    /* Convenience function */
    void addReceivedMessage(const Tp::MessagePartList &message);
private Q_SLOTS:
    void sent(uint timestamp, uint type, QString text);
protected:
    BaseChannelTextType(BaseChannel* channel);
    void acknowledgePendingMessages(const Tp::UIntList &IDs, DBusError* error);

private:
    void createAdaptor();

    class Adaptee;
    friend class Adaptee;
    struct Private;
    friend struct Private;
    Private *mPriv;
};

class TP_QT_EXPORT BaseChannelMessagesInterface : public AbstractChannelInterface
{
    Q_OBJECT
    Q_DISABLE_COPY(BaseChannelMessagesInterface)

public:
    static BaseChannelMessagesInterfacePtr create(BaseChannelTextType* textTypeInterface,
            QStringList supportedContentTypes,
            UIntList messageTypes,
            uint messagePartSupportFlags,
            uint deliveryReportingSupport) {
        return BaseChannelMessagesInterfacePtr(new BaseChannelMessagesInterface(textTypeInterface,
                                               supportedContentTypes,
                                               messageTypes,
                                               messagePartSupportFlags,
                                               deliveryReportingSupport));
    }
    template<typename BaseChannelMessagesInterfaceSubclass>
    static SharedPtr<BaseChannelMessagesInterfaceSubclass> create() {
        return SharedPtr<BaseChannelMessagesInterfaceSubclass>(
                   new BaseChannelMessagesInterfaceSubclass());
    }
    virtual ~BaseChannelMessagesInterface();

    QVariantMap immutableProperties() const;

    QStringList supportedContentTypes();
    Tp::UIntList messageTypes();
    uint messagePartSupportFlags();
    uint deliveryReportingSupport();
    Tp::MessagePartListList pendingMessages();

    void messageSent(const Tp::MessagePartList &content, uint flags, const QString &messageToken);

    typedef Callback3<QString, const Tp::MessagePartList&, uint, DBusError*> SendMessageCallback;
    void setSendMessageCallback(const SendMessageCallback &cb);
protected:
    QString sendMessage(const Tp::MessagePartList &message, uint flags, DBusError* error);
private Q_SLOTS:
    void pendingMessagesRemoved(const Tp::UIntList &messageIDs);
    void messageReceived(const Tp::MessagePartList &message);
private:
    BaseChannelMessagesInterface(BaseChannelTextType* textType,
                                 QStringList supportedContentTypes,
                                 Tp::UIntList messageTypes,
                                 uint messagePartSupportFlags,
                                 uint deliveryReportingSupport);
    void createAdaptor();

    class Adaptee;
    friend class Adaptee;
    struct Private;
    friend struct Private;
    Private *mPriv;
};

}
#endif