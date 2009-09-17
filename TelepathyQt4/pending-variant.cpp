/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2009 Nokia Corporation
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

#include <TelepathyQt4/PendingVariant>

#include "TelepathyQt4/_gen/pending-variant.moc.hpp"
#include "TelepathyQt4/debug-internal.h"

#include <QDBusPendingReply>

namespace Tp
{

struct PendingVariant::Private
{
    QVariant result;
};

/**
 * \class PendingVariant
 * \headerfile <TelepathyQt4/pending-variant.h> <TelepathyQt4/PendingVariant>
 */

PendingVariant::PendingVariant(QDBusPendingCall call, QObject *parent)
    : PendingOperation(parent),
      mPriv(new Private)
{
    connect(new QDBusPendingCallWatcher(call),
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            this,
            SLOT(watcherFinished(QDBusPendingCallWatcher*)));
}

/**
 * Class destructor.
 */
PendingVariant::~PendingVariant()
{
    delete mPriv;
}

QVariant PendingVariant::result() const
{
    return mPriv->result;
}

void PendingVariant::watcherFinished(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<QDBusVariant> reply = *watcher;

    if (!reply.isError()) {
        debug() << "Got reply to PendingVariant call";
        mPriv->result = reply.value().variant();
        setFinished();
    } else {
        debug().nospace() << "PendingVariant call failed: " <<
            reply.error().name() << ": " << reply.error().message();
        setFinishedWithError(reply.error());
    }

    watcher->deleteLater();
}

} // Tp
