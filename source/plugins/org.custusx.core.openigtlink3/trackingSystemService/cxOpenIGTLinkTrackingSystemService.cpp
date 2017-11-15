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

#include "cxOpenIGTLinkTrackingSystemService.h"

#include "cxLogger.h"
#include "cxOpenIGTLinkTool.h"
#include "cxProfile.h"

#include "cxToolConfigurationParser.h"

namespace cx
{

std::vector<ToolPtr> toVector(std::map<QString, OpenIGTLinkToolPtr> map)
{
	std::vector<ToolPtr> retval;
	std::map<QString, OpenIGTLinkToolPtr>::iterator it = map.begin();
	for(; it!= map.end(); ++it)
	{
		retval.push_back(it->second);
	}
	return retval;
}

OpenIGTLinkTrackingSystemService::OpenIGTLinkTrackingSystemService(NetworkHandlerPtr networkHandler) :
	mState(Tool::tsNONE)
	, mNetworkHandler(networkHandler)
	, mConfigurationFilePath("")
	, mLoggingFolder("")

{
	if(mNetworkHandler == NULL)
		return;

	connect(mNetworkHandler.get(), &NetworkHandler::connected,this, &OpenIGTLinkTrackingSystemService::serverIsConnected);
	connect(mNetworkHandler.get(), &NetworkHandler::disconnected, this, &OpenIGTLinkTrackingSystemService::serverIsDisconnected);
	connect(mNetworkHandler.get(), &NetworkHandler::transform, this, &OpenIGTLinkTrackingSystemService::receiveTransform);
	//connect(mNetworkHandler.get(), &NetworkHandler::calibration, this, &OpenIGTLinkTrackingSystemService::receiveCalibration);
	connect(mNetworkHandler.get(), &NetworkHandler::probedefinition, this, &OpenIGTLinkTrackingSystemService::receiveProbedefinition);

	this->setConfigurationFile(profile()->getToolConfigFilePath());
}

OpenIGTLinkTrackingSystemService::~OpenIGTLinkTrackingSystemService()
{
	this->deconfigure();
}

void OpenIGTLinkTrackingSystemService::setConfigurationFile(QString configurationFile)
{
	if (configurationFile == mConfigurationFilePath)
		return;

/*	if (this->isConfigured())
	{
		connect(this, SIGNAL(deconfigured()), this, SLOT(configureAfterDeconfigureSlot()));
		this->deconfigure();
	}*/

	mConfigurationFilePath = configurationFile;
}

void OpenIGTLinkTrackingSystemService::setLoggingFolder(QString loggingFolder)
{
	if (mLoggingFolder == loggingFolder)
		return;

/*	if (this->isConfigured())
	{
		connect(this, SIGNAL(deconfigured()), this, SLOT(configureAfterDeconfigureSlot()));
		this->deconfigure();
	}*/

	mLoggingFolder = loggingFolder;
}

QString OpenIGTLinkTrackingSystemService::getUid() const
{
	return "org.custusx.core.openigtlink3";
}

Tool::State OpenIGTLinkTrackingSystemService::getState() const
{
	return mState;
}

void OpenIGTLinkTrackingSystemService::setState(const Tool::State val)
{
	CX_LOG_DEBUG() << "OpenIGTLinkTrackingSystemService::setState: val: " << val;
	if (mState==val)
		return;

	if (val > mState) // up
	{
		if (val == Tool::tsTRACKING)
			this->startTracking();
		else if (val == Tool::tsINITIALIZED)
			this->initialize();
		else if (val == Tool::tsCONFIGURED)
			this->configure();
	}
	else // down
	{
		if (val == Tool::tsINITIALIZED)
			this->stopTracking();
		else if (val == Tool::tsCONFIGURED)
			this->uninitialize();
		else if (val == Tool::tsNONE)
			this->deconfigure();
	}
}

std::vector<ToolPtr> OpenIGTLinkTrackingSystemService::getTools()
{
	return toVector(mTools);
}

TrackerConfigurationPtr OpenIGTLinkTrackingSystemService::getConfiguration()
{
	TrackerConfigurationPtr retval;
	return retval;
}

ToolPtr OpenIGTLinkTrackingSystemService::getReference()
{
	return mReference;
}

//void OpenIGTLinkTrackingSystemService::setLoggingFolder(QString loggingFolder)
//{}

//TODO
void OpenIGTLinkTrackingSystemService::configure()
{
	CX_LOG_DEBUG() << "OpenIGTLinkTrackingSystemService::configure()";
	//parse
	ConfigurationFileParser configParser(mConfigurationFilePath, mLoggingFolder);

	if(!configParser.getTrackingSystem().contains("openigtlink", Qt::CaseInsensitive))
	{
		CX_LOG_DEBUG() << "OpenIGTLinkTrackingSystemService::configure(): Not using OpenIGTLink tracking.";
		return;
	}

	CX_LOG_DEBUG() << "OpenIGTLinkTrackingSystemService::configure(): Using OpenIGTLink tracking";

	//TODO
	//Copied from TrackingSystemIGSTKService::configure()
//	ToolFileParser::ToolInternalStructure referenceToolStructure;
//	std::vector<ToolFileParser::ToolInternalStructure> toolStructures;
//	QString referenceToolFile = configParser.getAbsoluteReferenceFilePath();
//	std::vector<QString> toolfiles = configParser.getAbsoluteToolFilePaths();
	std::vector<ConfigurationFileParser::ToolStructure> toolList = configParser.getToolListWithMetaInformation();

	//Create tools
	for(std::vector<ConfigurationFileParser::ToolStructure>::iterator it = toolList.begin(); it != toolList.end(); ++it)
	{
		ToolFileParser toolParser((*it).mAbsoluteToolFilePath, mLoggingFolder);
		ToolFileParser::ToolInternalStructure internalTool = toolParser.getTool();

		QString devicename = internalTool.mUid;
		OpenIGTLinkToolPtr newTool = OpenIGTLinkToolPtr(new OpenIGTLinkTool((*it), internalTool));
		mTools[devicename] = newTool;
	}

//	for (std::vector<QString>::iterator it = toolfiles.begin(); it != toolfiles.end(); ++it)
//	{
//		//TODO add OpenIGTLink uid names to tools
//		ToolFileParser toolParser(*it, mLoggingFolder);
//				ToolFileParser::ToolInternalStructure internalTool = toolParser.getTool();
//		if ((*it) == referenceToolFile)
//			referenceToolStructure = internalTool;
//		else
//			toolStructures.push_back(internalTool);
//	}


	this->serverIsConfigured();
}

void OpenIGTLinkTrackingSystemService::deconfigure()
{
	mTools.clear();
	mReference.reset();
	this->serverIsDeconfigured();
}

void OpenIGTLinkTrackingSystemService::initialize()
{
	//emit connectToServer();

	//TODO is(!configured());
	this->configure();
}

void OpenIGTLinkTrackingSystemService::uninitialize()
{
	emit disconnectFromServer();
}

void OpenIGTLinkTrackingSystemService::startTracking()
{
	//emit startListenToServer();
	//emit connectToServer();

	//TODO: is(!Initialized())
	this->initialize();
}

void OpenIGTLinkTrackingSystemService::stopTracking()
{
	//emit stopListenToServer();
	emit disconnectFromServer();
}

void OpenIGTLinkTrackingSystemService::serverIsConfigured()
{
	this->internalSetState(Tool::tsCONFIGURED);
}

void OpenIGTLinkTrackingSystemService::serverIsDeconfigured()
{
	this->internalSetState(Tool::tsNONE);
}

void OpenIGTLinkTrackingSystemService::serverIsConnected()
{
	this->internalSetState(Tool::tsINITIALIZED);
	this->internalSetState(Tool::tsTRACKING);
}

void OpenIGTLinkTrackingSystemService::serverIsDisconnected()
{
	this->internalSetState(Tool::tsCONFIGURED);
	this->internalSetState(Tool::tsINITIALIZED);
}


//TODO: Require/trigger configure?
void OpenIGTLinkTrackingSystemService::receiveTransform(QString devicename, Transform3D transform, double timestamp)
{
//	CX_LOG_DEBUG() << "receiveTransform for: " << devicename;
	OpenIGTLinkToolPtr tool = this->getTool(devicename);
	if(tool)
		tool->toolTransformAndTimestampSlot(transform, timestamp);
}

void OpenIGTLinkTrackingSystemService::receiveCalibration(QString devicename, Transform3D calibration)
{
	CX_LOG_DEBUG() << "receiveCalibration for: " << devicename;
	OpenIGTLinkToolPtr tool = this->getTool(devicename);
	if(tool)
		tool->setCalibration_sMt(calibration);
}

void OpenIGTLinkTrackingSystemService::receiveProbedefinition(QString devicename, ProbeDefinitionPtr definition)
{
//	CX_LOG_DEBUG() << "receiveProbedefinition for: " << devicename << " equipmentType: " << equipmentType;
	CX_LOG_DEBUG() << "receiveProbedefinition for: " << devicename;
	OpenIGTLinkToolPtr tool = this->getTool(devicename);
	if(tool)
	{
		ProbePtr probe = tool->getProbe();
		if(probe)
		{
			CX_LOG_DEBUG() << "receiveProbedefinition. Tool is probe: " << devicename;
			ProbeDefinition old_def = probe->getProbeDefinition();
			definition->setUid(old_def.getUid());
			definition->applySoundSpeedCompensationFactor(old_def.getSoundSpeedCompensationFactor());

			probe->setProbeDefinition(*(definition.get()));
			emit stateChanged();
		}
		else
		{
			CX_LOG_DEBUG() << "receiveProbedefinition. Tool is not probe: " << devicename;
		}
	}
}

void OpenIGTLinkTrackingSystemService::internalSetState(Tool::State state)
{
	mState = state;
	emit stateChanged();
}

OpenIGTLinkToolPtr OpenIGTLinkTrackingSystemService::getTool(QString devicename)
{
//	CX_LOG_DEBUG() << "OpenIGTLinkTrackingSystemService::getTool: " << devicename;

	std::map<QString, OpenIGTLinkToolPtr>::iterator it;
	for (it = mTools.begin(); it != mTools.end(); ++it)
	{
		OpenIGTLinkToolPtr tool = it->second;
		if (tool->isThisTool(devicename))
		{
//			emit stateChanged();
			return tool;
		}
	}
	return OpenIGTLinkToolPtr();
}


} /* namespace cx */