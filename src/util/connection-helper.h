/*
 * Copyright (C) 2016 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Charles Kerr <charles.kerr@canonical.com>
 */

#pragma once

#include <QDebug>
#include <QObject>
#include <QFuture>
#include <QFutureWatcher>

#include <functional>
#include <map>
#include <memory>

/**
 * A class to manage connections to QFutures and one-shot signals.
 * It will disconnect & release resources when the QFuture is finished
 * or when the single-shot signal is called.
 *
 * Destroying the ConnectionHelper also disconnects & releases resources,
 * so an object that needs to safely manage those kinds of connections
 * can aggregate a ConnectionHelper so that connections are destroyed when
 * the object is destroyed.
 */
class ConnectionHelper final: public QObject
{
    Q_OBJECT

public:

    Q_DISABLE_COPY(ConnectionHelper)
    ConnectionHelper(QObject *parent = nullptr): QObject(parent) {}
    virtual ~ConnectionHelper() =default;

    void
    remember(QMetaObject::Connection const& c,
             std::function<void(void)> const& closure = [](){})
    {
        remember(c, next_tag++, closure);
    }

    template<typename PointerToMemberFunction,
             typename... FunctionArgs>
    void
    connect_oneshot(QObject* sender,
                    PointerToMemberFunction signal,
                    std::function<void(FunctionArgs... args)> const& receiver,
                    std::function<void(void)> const& closure = [](){})
    {
        auto const tag = next_tag++;
        remember(
            QObject::connect(
                qobject_cast<typename QtPrivate::FunctionPointer<PointerToMemberFunction>::Object *>(sender),
                signal,
                [this, receiver, tag](FunctionArgs... args){
                    receiver(args...);
                    qDebug() << "erasing tag" << tag;
                    connections_.erase(tag);
                }
            ),
            tag,
            closure
        );
    }

    template<typename ResultType>
    void
    connect_future(QFuture<ResultType> future,
                   std::function<void(ResultType const&)> const& callme)
    {
        auto watcher = new QFutureWatcher<ResultType>{};

        std::function<void()> on_finished = [watcher, callme](){
            try {
                callme(watcher->result());
            } catch(std::exception& e) {
                qWarning() << "future threw error:" << e.what();
                callme(ResultType{});
            }
        };
        std::function<void()> closure = [watcher](){qDebug() << "calling watcher->deleteLater"; watcher->deleteLater();};

        connect_oneshot(watcher,
                        &std::remove_reference<decltype(*watcher)>::type::finished,
                        on_finished,
                        closure);

        watcher->setFuture(future);
    }

private:

    unsigned int next_tag = 1;
    std::map<decltype(next_tag),std::shared_ptr<QMetaObject::Connection>> connections_;

    void
    remember(QMetaObject::Connection const& connection,
             decltype(next_tag) tag,
             std::function<void(void)> const& closure = [](){})
    {
        connections_[tag] = std::shared_ptr<QMetaObject::Connection>(
            new QMetaObject::Connection(connection),
            [closure, tag](QMetaObject::Connection* c){
                qDebug() << "deleting connection" << c << "for tag" << tag;
                QObject::disconnect(*c);
                delete c;
                closure();
            }
        );
    }
};
