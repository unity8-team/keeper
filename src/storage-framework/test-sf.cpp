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

TestSF::TestSF(bool commit, QObject *parent)
    : QObject(parent)
    , timer_(new QTimer())
    , sf_(new StorageFrameworkClient())
    , commit_(commit)
    , state_(TestSF::State::INACTIVE)
{
    connect(timer_.data(), &QTimer::timeout, this, &TestSF::timeout_reached);
    timer_->start(2000);

    connect(sf_.data(), &StorageFrameworkClient::socketReady, this, &TestSF::sf_socket_ready);
    connect(sf_.data(), &StorageFrameworkClient::finished, this, &TestSF::on_sf_socket_finished);
}

TestSF::~TestSF() = default;

void TestSF::start()
{
    state_ = TestSF::State::WAITING_SOCKET;
    sf_->getNewFileForBackup(1024);
}

void TestSF::write()
{
    QByteArray test("12345");
    const auto n = sf_socket_->write(test);
    if (n > 0) 
    {
	qDebug() << "Wrote " << n << " bytes OK";
        state_ = TestSF::State::WROTE;
    }
    else 
    {
        if (n < 0) 
        {
            qWarning() << "Write error:" << sf_socket_->errorString();
	}
    }
}

void TestSF::finish()
{
    sf_->finish(commit_);
    if (commit_)
    {
        state_ = TestSF::State::WAITING_FINISH;
    }
    else
    {
        state_ = TestSF::State::INACTIVE;
    }
}

void TestSF::timeout_reached()
{
    switch (state_)
    {
	case TestSF::State::INACTIVE:
		qDebug() << "********************************* Asking for socket";
		start();
		break;
	case TestSF::State::WAITING_SOCKET:
		qDebug() << "********************************* Waiting for socket";
		break;
	case TestSF::State::SOCKET_READY:
                qDebug() << "********************************* Writing to the socket";
		write();
		break;
	case TestSF::State::WROTE:
		qDebug() << "********************************* Finishing socket: commiting changes = " << commit_;
		finish();
		break;
	case TestSF::State::WAITING_FINISH:
		qDebug() << "********************************* Waiting for the socket to finish";
		break;
    };
    qDebug() << "tick...";
}

void TestSF::sf_socket_ready(std::shared_ptr<QLocalSocket> const& sf_socket)
{
	qDebug() << "Storage framework socket ok";
        sf_socket_ = sf_socket;
	connect(sf_socket_.get(), &QLocalSocket::stateChanged, this, &TestSF::on_sf_socket_state_changed);
        state_ = TestSF::State::SOCKET_READY;
}

void TestSF::on_sf_socket_state_changed(QLocalSocket::LocalSocketState socketState)
{
    qDebug() << "State of storage framework socket changed to: " << socketState;
}

void TestSF::on_sf_socket_finished()
{
    qDebug() << "Socket finished";
    state_ = TestSF::State::INACTIVE;
}
