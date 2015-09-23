/*=========================================================================
This file is part of CustusX, an Image Guided Therapy Application.

Copyright (c) 2008-2014, SINTEF Department of Medical Technology
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=========================================================================*/

#include "cxNetworkConnection.h"

#include "cxSender.h"
#include "cxTime.h"
#include <QThread>

#include "cxPlusDialect.h"
#include "cxCustusDialect.h"
#include "cxRASDialect.h"
#include "igtl_header.h"
#include "cxIGTLinkConversionImage.h"
#include "cxIGTLinkConversionPolyData.h"
#include "cxXmlOptionItem.h"
#include "cxProfile.h"
#include "cxStringProperty.h"
#include "cxDoubleProperty.h"

namespace cx
{

NetworkConnection::NetworkConnection(QString uid, QObject *parent) :
    SocketConnection(parent),
    mHeader(igtl::MessageHeader::New()),
	mHeaderReceived(false),
	mUid(uid)
{
    qRegisterMetaType<Transform3D>("Transform3D");
    qRegisterMetaType<ImagePtr>("ImagePtr");
	qRegisterMetaType<ImagePtr>("MeshPtr");
	qRegisterMetaType<ProbeDefinitionPtr>("ProbeDefinitionPtr");


	ConnectionInfo info = this->getConnectionInfo();

	info.protocol = this->initDialect(DialectPtr(new CustusDialect()))->getName();
	this->initDialect(DialectPtr(new PlusDialect()));
	this->initDialect(DialectPtr(new Dialect()));
	this->initDialect(DialectPtr(new RASDialect()));

	SocketConnection::setConnectionInfo(info);
	this->setDialect(info.protocol);
}

NetworkConnection::~NetworkConnection()
{
}

DialectPtr NetworkConnection::initDialect(DialectPtr value)
{
	mAvailableDialects[value->getName()] = value;
	return value;
}

void NetworkConnection::setConnectionInfo(ConnectionInfo info)
{
	SocketConnection::setConnectionInfo(info);
	this->setDialect(info.protocol);
}

void NetworkConnection::invoke(boost::function<void()> func)
{
	QMetaObject::invokeMethod(this,
							  "onInvoke",
							  Qt::QueuedConnection,
							  Q_ARG(boost::function<void()>, func));
}

void NetworkConnection::onInvoke(boost::function<void()> func)
{
	func();
}

QStringList NetworkConnection::getAvailableDialects() const
{
    QStringList retval;
    DialectMap::const_iterator it = mAvailableDialects.begin();
    for( ; it!=mAvailableDialects.end() ; ++it)
    {
        retval << it->first;
    }
    return retval;
}

void NetworkConnection::setDialect(QString dialectname)
{
    QMutexLocker locker(&mMutex);
    if(mDialect && (dialectname == mDialect->getName()))
        return;

    DialectPtr dialect = mAvailableDialects[dialectname];
    if(!dialect)
    {
        CX_LOG_ERROR() << "\"" << dialectname << "\" is an unknown opentigtlink dialect.";
        return;
    }

    if(mDialect)
    {
		disconnect(mDialect.get(), &Dialect::image, this, &NetworkConnection::image);
		disconnect(mDialect.get(), &Dialect::mesh, this, &NetworkConnection::mesh);
		disconnect(mDialect.get(), &Dialect::transform, this, &NetworkConnection::transform);
		disconnect(mDialect.get(), &Dialect::calibration, this, &NetworkConnection::calibration);
		disconnect(mDialect.get(), &Dialect::probedefinition, this, &NetworkConnection::probedefinition);
		disconnect(mDialect.get(), &Dialect::usstatusmessage, this, &NetworkConnection::usstatusmessage);
		disconnect(mDialect.get(), &Dialect::igtlimage, this, &NetworkConnection::igtlimage);
    }

    mDialect = dialect;
	connect(dialect.get(), &Dialect::image, this, &NetworkConnection::image);
	connect(dialect.get(), &Dialect::mesh, this, &NetworkConnection::mesh);
	connect(dialect.get(), &Dialect::transform, this, &NetworkConnection::transform);
	connect(dialect.get(), &Dialect::calibration, this, &NetworkConnection::calibration);
	connect(dialect.get(), &Dialect::probedefinition, this, &NetworkConnection::probedefinition);
	connect(dialect.get(), &Dialect::usstatusmessage, this, &NetworkConnection::usstatusmessage);
	connect(dialect.get(), &Dialect::igtlimage, this, &NetworkConnection::igtlimage);

	CX_LOG_CHANNEL_SUCCESS(CX_OPENIGTLINK_CHANNEL_NAME) << "IGTL Dialect set to " << dialectname;

}

void NetworkConnection::sendMessage(ImagePtr image)
{
	QMutexLocker locker(&mMutex);

	IGTLinkConversionImage imageConverter;
	igtl::ImageMessage::Pointer msg = imageConverter.encode(image, mDialect->coordinateSystem());
	CX_LOG_CHANNEL_DEBUG(CX_OPENIGTLINK_CHANNEL_NAME) << "Sending image: " << image->getName();
	msg->Pack();

	mSocket->write(reinterpret_cast<char*>(msg->GetPackPointer()), msg->GetPackSize());
}

void NetworkConnection::sendMessage(MeshPtr data)
{
	QMutexLocker locker(&mMutex);

	IGTLinkConversionPolyData polyConverter;
	igtl::PolyDataMessage::Pointer msg = polyConverter.encode(data, mDialect->coordinateSystem());
	CX_LOG_CHANNEL_DEBUG(CX_OPENIGTLINK_CHANNEL_NAME) << "Sending mesh: " << data->getName();
	msg->Pack();

	mSocket->write(reinterpret_cast<char*>(msg->GetPackPointer()), msg->GetPackSize());
}

/*
void NetworkConnection::sendStringMessage(QString command)
{
    QMutexLocker locker(&mMutex);

    OpenIGTLinkConversion converter;
    igtl::StringMessage::Pointer stringMsg = converter.encode(command);
    CX_LOG_CHANNEL_DEBUG(CX_OPENIGTLINK_CHANNEL_NAME) << "Sending string: " << command;
    stringMsg->Pack();

    mSocket->write(reinterpret_cast<char*>(stringMsg->GetPackPointer()), stringMsg->GetPackSize());
}
*/

void NetworkConnection::internalDataAvailable()
{
    if(!this->socketIsConnected())
        return;

    bool done = false;
    while(!done)
    {
        if(!mHeaderReceived)
        {
            if(!this->receiveHeader(mHeader))
                done = true;
            else
                mHeaderReceived = true;
		}

        if(mHeaderReceived)
        {
            if(!this->receiveBody(mHeader))
                done = true;
            else
                mHeaderReceived = false;
		}
    }
}

/*
void NetworkConnection::queryServer()
{
    QString command = "<Command Name=\"RequestChannelIds\" />";
    this->sendStringMessage(command);

    command = "<Command Name=\"RequestDeviceIds\" />";
    this->sendStringMessage(command);

    command = "<Command Name=\"SaveConfig\" />";
    this->sendStringMessage(command);
}
*/

bool NetworkConnection::receiveHeader(const igtl::MessageHeader::Pointer header) const
{
    header->InitPack();

    if(!this->socketReceive(header->GetPackPointer(), header->GetPackSize()))
        return false;

    int c = header->Unpack(1);
    if (c & igtl::MessageHeader::UNPACK_HEADER)
    {
        std::string deviceType = std::string(header->GetDeviceType());
        return true;
    }
    else
        return false;
}

bool NetworkConnection::receiveBody(const igtl::MessageBase::Pointer header)
{
	QString type = QString(header->GetDeviceType()).toUpper();
	if (type=="TRANSFORM")
    {
        if(!this->receive<igtl::TransformMessage>(header))
            return false;
    }
	else if (type=="POLYDATA")
	{
		if(!this->receive<igtl::PolyDataMessage>(header))
			return false;
	}
	else if (type=="IMAGE")
    {
        //----- CustusX openigtlink server -----
        //there is a special kind of image package coming from custusx
        //server where crc is set to 0.
        QString name(header->GetDeviceName());
        if(name.contains("Sonix", Qt::CaseInsensitive))
        {
            if(!this->receive<IGTLinkImageMessage>(header))
                return false;
        }
        //----------
        else
        {
            if(!this->receive<igtl::ImageMessage>(header))
                return false;
        }
    }
	else if (type=="STATUS")
    {
        if(!this->receive<igtl::StatusMessage>(header))
            return false;
    }
	else if (type=="STRING")
    {
        if(!this->receive<igtl::StringMessage>(header))
            return false;
    }
	else if (type=="CX_US_ST")
    {
        if(!this->receive<IGTLinkUSStatusMessage>(header))
            return false;
    }
    else
    {
        CX_LOG_CHANNEL_WARNING(CX_OPENIGTLINK_CHANNEL_NAME) << "Skipping message of type " << type;
        igtl::MessageBase::Pointer body = igtl::MessageBase::New();
        body->SetMessageHeader(header);
		this->skip(body->GetBodySizeToRead());
    }
    return true;
}

qint64 NetworkConnection::skip(qint64 maxSizeBytes) const
{
	char *voidData = new char[maxSizeBytes];
	int retval = mSocket->read(voidData, maxSizeBytes);
	delete[] voidData;
	return retval;
}


template <typename T>
bool NetworkConnection::receive(const igtl::MessageBase::Pointer header)
{
    QMutexLocker locker(&mMutex);

    typename T::Pointer body = T::New();
    body->SetMessageHeader(header);
    body->AllocatePack();

    if(!this->socketReceive(body->GetPackBodyPointer(), body->GetPackBodySize()))
        return false;

    int c = body->Unpack(mDialect->doCRC());
    if (c & igtl::MessageHeader::UNPACK_BODY)
    {
        mDialect->translate(body);
    }
    else
    {
        CX_LOG_CHANNEL_ERROR(CX_OPENIGTLINK_CHANNEL_NAME) << "Could not unpack the body of type: " << body->GetDeviceType();
        return false;
    }
    return true;
}


}//namespace cx