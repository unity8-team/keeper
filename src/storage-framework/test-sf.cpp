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
 * Author: Xavi Garcia <xavi.garcia.mena@canonical.com>
 */
#include "test-sf.h"
#include "storage_framework_client.h"

#include <QDebug>
#include <QTimer>

TestSF::TestSF(QObject *parent)
    : QObject(parent)
    , timer_(new QTimer())
    , sf_(new StorageFrameworkClient())
    , seconds_{0}
{
    connect(timer_.data(), &QTimer::timeout, this, &TestSF::timeout_reached);
    timer_->start(2000);

    connect(sf_.data(), &StorageFrameworkClient::socketReady, this, &TestSF::sf_socket_ready);
}

TestSF::~TestSF() = default;

void TestSF::start()
{
    qDebug() << "Asking for socket";
    sf_->getNewFileForBackup(1024);
}

void TestSF::timeout_reached()
{
    if (++seconds_ == 1)
    {
        start();
    }
    if (seconds_ == 3)
    {
	QByteArray test("12345");
	const auto n = sf_socket_->write(test);
	if (n > 0) 
        {
	    qDebug() << "Wrote " << n << " bytes OK";
	}
	else 
        {
	    if (n < 0) 
            {
                qWarning() << "Write error:" << sf_socket_->errorString();
	    }
	}
    }
    qDebug() << "tick...";
}

void TestSF::sf_socket_ready(std::shared_ptr<QLocalSocket> const& sf_socket)
{
	qDebug() << "Storage framework socket ok";
        sf_socket_ = sf_socket;
	connect(sf_socket_.get(), &QLocalSocket::stateChanged, this, &TestSF::on_sf_socket_state_changed);
}

void TestSF::on_sf_socket_state_changed(QLocalSocket::LocalSocketState socketState)
{
    qDebug() << "State of storage framework socket changed to: " << socketState;
}

