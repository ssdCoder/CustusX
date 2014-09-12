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
#include <cxCalibrationMethodsWidget.h>

#include <ctkPluginContext.h>
#include <cxToolTipSampleWidget.h>
#include "cxToolTipCalibrationWidget.h"
#include "cxToolManualCalibrationWidget.h"
#include "cxTemporalCalibrationWidget.h"
#include "cxLapFrameToolCalibrationWidget.h"
#include "cxProbeConfigWidget.h"

namespace cx
{

CalibrationMethodsWidget::CalibrationMethodsWidget(ctkPluginContext *pluginContext, AcquisitionDataPtr acquisitionData, QWidget* parent, QString objectName, QString windowTitle) :
  TabbedWidget(parent, objectName, windowTitle)
{
  this->addTab(new ToolTipCalibrateWidget(this), "Tool Tip");
  this->addTab(new LapFrameToolCalibrationWidget(this), "Lap Frame");
	this->addTab(new ToolTipSampleWidget(pluginContext, this), "Sample");
  this->addTab(new TemporalCalibrationWidget(acquisitionData, this), "Temporal");
  this->addTab(new ToolManualCalibrationWidget(this), "Tool Manual");
  this->addTab(new ProbeConfigWidget(this), "Probe");
}

QString CalibrationMethodsWidget::defaultWhatsThis() const
{
  return"<html>"
      "<h3>Calibration methods.</h3>"
      "<p>These methods creates data structures that can be use in visualization.</p>"
      "<p><i>Choose a method.</i></p>"
      "</html>";
}

}
