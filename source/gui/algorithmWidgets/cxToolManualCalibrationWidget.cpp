/*
 * cxToolManualCalibrationWidget.cpp
 *
 *  Created on: May 4, 2011
 *      Author: christiana
 */

#include <cxToolManualCalibrationWidget.h>
#include "cxActiveToolWidget.h"
#include "sscToolManager.h"

namespace cx
{

ToolManualCalibrationWidget::ToolManualCalibrationWidget(QWidget* parent) :
    BaseWidget(parent, "TemporalCalibrationWidget", "Temporal Calibrate")
{
  //layout
  QVBoxLayout* mToptopLayout = new QVBoxLayout(this);
  //toptopLayout->setMargin(0);

  mToptopLayout->addWidget(new ActiveToolWidget(this));

  mGroup = new QGroupBox(this);
  mGroup->setTitle("Calibration matrix sMt");
  mToptopLayout->addWidget(mGroup);
  QVBoxLayout* groupLayout = new QVBoxLayout;
  mGroup->setLayout(groupLayout);
  groupLayout->setMargin(0);
  mMatrixWidget = new Transform3DWidget(mGroup);
  groupLayout->addWidget(mMatrixWidget);
  connect(mMatrixWidget, SIGNAL(changed()), this, SLOT(matrixWidgetChanged()));
  connect(ssc::toolManager(), SIGNAL(dominantToolChanged(const QString&)), this, SLOT(toolCalibrationChanged()));

  this->toolCalibrationChanged();
  mMatrixWidget->setEditable(true);

  mToptopLayout->addStretch();

  connect(ssc::toolManager(), SIGNAL(configured()), this, SLOT(toolCalibrationChanged()));
  connect(ssc::toolManager(), SIGNAL(initialized()), this, SLOT(toolCalibrationChanged()));
  connect(ssc::toolManager(), SIGNAL(trackingStarted()), this, SLOT(toolCalibrationChanged()));
  connect(ssc::toolManager(), SIGNAL(trackingStopped()), this, SLOT(toolCalibrationChanged()));
}


QString ToolManualCalibrationWidget::defaultWhatsThis() const
{
  return "<html>"
      "<h3>Tool Manual Calibration.</h3>"
      "<p><i>Manipulate the tool calibration matrix sMt directly, using the matrix manipulation interface.</i></br>"
      "</html>";
}


void ToolManualCalibrationWidget::toolCalibrationChanged()
{
  ssc::ToolPtr tool = ssc::toolManager()->getDominantTool();
  if (!tool)
    return;

//  mManualGroup->setVisible(tool->getVisible());
  mMatrixWidget->blockSignals(true);
  mMatrixWidget->setMatrix(tool->getCalibration_sMt());
  mMatrixWidget->blockSignals(false);
}

void ToolManualCalibrationWidget::matrixWidgetChanged()
{
  ssc::ToolPtr tool = ssc::toolManager()->getDominantTool();
    if (!tool)
      return;

  ssc::Transform3D M = mMatrixWidget->getMatrix();
  tool->setCalibration_sMt(M);
}



}
