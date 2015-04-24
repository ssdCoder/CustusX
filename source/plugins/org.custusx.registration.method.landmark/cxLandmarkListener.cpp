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

#include "cxLandmarkListener.h"
#include "cxLandmarkRep.h"
#include "cxRegistrationService.h"
#include "cxViewService.h"
#include "cxRepContainer.h"

namespace cx
{

LandmarkListener::LandmarkListener(RegServices services) :
	mServices(services),
	mImage2Image(false),
	mUseOnlyOneSourceUpdatedFromOutside(false)
{
	mFixedLandmarkSource = ImageLandmarksSource::New();
	mMovingLandmarkSource = ImageLandmarksSource::New();

	connect(mServices.registrationService.get(), &RegistrationService::fixedDataChanged, this, &LandmarkListener::updateFixed);
	connect(mServices.registrationService.get(), &RegistrationService::movingDataChanged, this, &LandmarkListener::updateMoving);
}

LandmarkListener::~LandmarkListener()
{
	disconnect(mServices.registrationService.get(), &RegistrationService::fixedDataChanged, this, &LandmarkListener::updateFixed);
	disconnect(mServices.registrationService.get(), &RegistrationService::movingDataChanged, this, &LandmarkListener::updateMoving);
}

void LandmarkListener::useI2IRegistration(bool useI2I)
{
	mImage2Image = useI2I;
}

void LandmarkListener::useOnlyOneSourceUpdatedFromOutside(bool useOnlyOneSourceUpdatedFromOutside)
{
	mUseOnlyOneSourceUpdatedFromOutside = useOnlyOneSourceUpdatedFromOutside;
	disconnect(mServices.registrationService.get(), &RegistrationService::fixedDataChanged, this, &LandmarkListener::updateFixed);
	disconnect(mServices.registrationService.get(), &RegistrationService::movingDataChanged, this, &LandmarkListener::updateMoving);
}


void LandmarkListener::updateFixed()
{
	mFixedLandmarkSource->setData(mServices.registrationService->getFixedData());
}

void LandmarkListener::updateMoving()
{
	mMovingLandmarkSource->setData(mServices.registrationService->getMovingData());
}

void LandmarkListener::setLandmarkSource(DataPtr data)
{
	if(!mUseOnlyOneSourceUpdatedFromOutside)
		return;
	mFixedLandmarkSource->setData(data);
}

DataPtr LandmarkListener::getLandmarkSource()
{
	return mFixedLandmarkSource->getData();
}

void LandmarkListener::showRep()
{
	if(!mServices.visualizationService->get3DView(0, 0))
		return;

	LandmarkRepPtr rep = mServices.visualizationService->get3DReps(0, 0)->findFirst<LandmarkRep>();

	if (rep)
	{
		rep->setPrimarySource(mFixedLandmarkSource);
		rep->setSecondaryColor(QColor::fromRgbF(0, 0.6, 0.8));
//		rep->setSecondaryColor(QColor::fromRgbF(0, 0.9, 0.5));

		if (mUseOnlyOneSourceUpdatedFromOutside)
			rep->setSecondarySource(LandmarksSourcePtr());//Only show one source
		else if(mImage2Image)
			rep->setSecondarySource(mMovingLandmarkSource);//I2I reg
		else
			rep->setSecondarySource(PatientLandmarksSource::New(mServices.patientModelService));//I2P reg
	}
}

void LandmarkListener::hideRep()
{
	if(!mServices.visualizationService->get3DView(0, 0))
		return;

	LandmarkRepPtr rep = mServices.visualizationService->get3DReps(0, 0)->findFirst<LandmarkRep>();
	if (rep)
	{
		rep->setPrimarySource(LandmarksSourcePtr());
		rep->setSecondarySource(LandmarksSourcePtr());
	}
}

} //cx