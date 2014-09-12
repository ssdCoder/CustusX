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

#include "cxContourFilter.h"

#include <vtkImageShrink3D.h>
#include <vtkMarchingCubes.h>
#include <vtkWindowedSincPolyDataFilter.h>
#include <vtkTriangleFilter.h>
#include <vtkDecimatePro.h>
#include <vtkPolyDataNormals.h>
#include <vtkImageData.h>

#include "cxReporter.h"
#include "cxRegistrationTransform.h"
#include "cxTypeConversions.h"
#include "cxImage.h"
#include "cxMesh.h"
#include "cxDataManager.h"
#include "cxDoubleDataAdapterXml.h"
#include "cxBoolDataAdapterXml.h"
#include "cxColorDataAdapterXml.h"
#include "cxPatientData.h"
#include "cxPatientService.h"
#include "cxRepManager.h"
#include "cxThresholdPreview.h"
#include "cxSelectDataStringDataAdapter.h"
#include "cxLegacySingletons.h"

namespace cx
{

ContourFilter::ContourFilter(ctkPluginContext *pluginContext) :
	FilterImpl(pluginContext)
{
}

QString ContourFilter::getName() const
{
	return "Contour";
}

QString ContourFilter::getType() const
{
	return "ContourFilter";
}

QString ContourFilter::getHelp() const
{
	return "<html>"
	        "<h3>Surfacing.</h3>"
	        "<p><i>Find the surface of a binary volume using marching cubes.</i></p>"
	        "<p>- Optional factor 2 reduction</p>"
	        "<p>- Marching Cubes contouring</p>"
	        "<p>- Optional Windowed Sinc smoothing</p>"
	        "<p>- Decimation of triangles</p>"
	        "</html>";
}

BoolDataAdapterXmlPtr ContourFilter::getReduceResolutionOption(QDomElement root)
{
	return BoolDataAdapterXml::initialize("Reduce input", "",
	                                           "Reduce input volumes resolution by a factor of 2 in all directions.", false, root);
}

BoolDataAdapterXmlPtr ContourFilter::getSmoothingOption(QDomElement root)
{
	return BoolDataAdapterXml::initialize("Smoothing", "",
	                                           "Smooth the output contour", true, root);
}

BoolDataAdapterXmlPtr ContourFilter::getPreserveTopologyOption(QDomElement root)
{
	return BoolDataAdapterXml::initialize("Preserve mesh topology", "",
	                                           "Preserve mesh topology during reduction", true, root);
}

DoubleDataAdapterXmlPtr ContourFilter::getSurfaceThresholdOption(QDomElement root)
{
	DoubleDataAdapterXmlPtr retval = DoubleDataAdapterXml::initialize("Threshold", "",
	                                                                            "Values from this threshold and above will be included",
	                                                                            100.0, DoubleRange(-1000, 1000, 1), 0, root);
	retval->setGuiRepresentation(DoubleDataAdapter::grSLIDER);
	return retval;
}

DoubleDataAdapterXmlPtr ContourFilter::getDecimationOption(QDomElement root)
{
	DoubleDataAdapterXmlPtr retval = DoubleDataAdapterXml::initialize("Decimation %", "",
	                                                                            "Reduce number of triangles in output surface",
	                                                                            0.2, DoubleRange(0, 1, 0.01), 0, root);
	retval->setInternal2Display(100);
	return retval;
}

ColorDataAdapterXmlPtr ContourFilter::getColorOption(QDomElement root)
{
	return ColorDataAdapterXml::initialize("Color", "",
	                                            "Color of output model.",
	                                            QColor("green"), root);
}

void ContourFilter::createOptions()
{
	mReduceResolutionOption = this->getReduceResolutionOption(mOptions);
	mOptionsAdapters.push_back(mReduceResolutionOption);

	mSurfaceThresholdOption = this->getSurfaceThresholdOption(mOptions);
	connect(mSurfaceThresholdOption.get(), SIGNAL(changed()), this, SLOT(thresholdSlot()));
	mOptionsAdapters.push_back(mSurfaceThresholdOption);

	mOptionsAdapters.push_back(this->getSmoothingOption(mOptions));
	mOptionsAdapters.push_back(this->getDecimationOption(mOptions));
	mOptionsAdapters.push_back(this->getPreserveTopologyOption(mOptions));

	mOptionsAdapters.push_back(this->getColorOption(mOptions));
}

void ContourFilter::createInputTypes()
{
	SelectDataStringDataAdapterBasePtr temp;

	temp = SelectImageStringDataAdapter::New(mPluginContext);
	temp->setValueName("Input");
	temp->setHelp("Select image input for contouring");
	connect(temp.get(), SIGNAL(dataChanged(QString)), this, SLOT(imageChangedSlot(QString)));
	mInputTypes.push_back(temp);
}

void ContourFilter::createOutputTypes()
{
	SelectDataStringDataAdapterBasePtr temp;

	temp = SelectMeshStringDataAdapter::New(mPluginContext);
	temp->setValueName("Output");
	temp->setHelp("Output contour");
	mOutputTypes.push_back(temp);
}

void ContourFilter::setActive(bool on)
{
	FilterImpl::setActive(on);

	if (!mActive)
		RepManager::getInstance()->getThresholdPreview()->removePreview();
}

void ContourFilter::imageChangedSlot(QString uid)
{
	ImagePtr image = dataManager()->getImage(uid);
	if(!image)
		return;

	this->updateThresholdFromImageChange(uid, mSurfaceThresholdOption);
	RepManager::getInstance()->getThresholdPreview()->removePreview();

	int extent[6];
	image->getBaseVtkImageData()->GetExtent(extent);
	mReduceResolutionOption->setHelp("Current input resolution: " + qstring_cast(extent[1])
	                                 + " " + qstring_cast(extent[3]) + " " + qstring_cast(extent[5])
	                                 + " (If checked: " + qstring_cast(extent[1]/2)+ " " + qstring_cast(extent[3]/2) + " "
	                                 + qstring_cast(extent[5]/2) + ")");
}

void ContourFilter::thresholdSlot()
{
	//    std::cout << "ContourFilter::thresholdSlot() " << mActive << std::endl;
	if (mActive)
	{
		ImagePtr image = boost::dynamic_pointer_cast<Image>(mInputTypes[0]->getData());
		RepManager::getInstance()->getThresholdPreview()->setPreview(image, mSurfaceThresholdOption->getValue());
	}
}

bool ContourFilter::preProcess()
{
	RepManager::getInstance()->getThresholdPreview()->removePreview();
	return FilterImpl::preProcess();
}

bool ContourFilter::execute()
{
	ImagePtr input = this->getCopiedInputImage();
	if (!input)
		return false;

	//    std::cout << "ContourFilter::execute : " << mCopiedOptions.ownerDocument().toString() << std::endl;

	BoolDataAdapterXmlPtr reduceResolutionOption = this->getReduceResolutionOption(mCopiedOptions);
	BoolDataAdapterXmlPtr smoothingOption = this->getSmoothingOption(mCopiedOptions);
	BoolDataAdapterXmlPtr preserveTopologyOption = this->getPreserveTopologyOption(mCopiedOptions);
	DoubleDataAdapterXmlPtr surfaceThresholdOption = this->getSurfaceThresholdOption(mCopiedOptions);
	DoubleDataAdapterXmlPtr decimationOption = this->getDecimationOption(mCopiedOptions);

	//    report(QString("Creating contour from \"%1\"...").arg(input->getName()));

	mRawResult = this->execute(input->getBaseVtkImageData(),
	                           surfaceThresholdOption->getValue(),
	                           reduceResolutionOption->getValue(),
	                           smoothingOption->getValue(),
	                           preserveTopologyOption->getValue(),
	                           decimationOption->getValue());
	return true;
}

vtkPolyDataPtr ContourFilter::execute(vtkImageDataPtr input,
                                      double threshold,
                                      bool reduceResolution,
                                      bool smoothing,
                                      bool preserveTopology,
                                      double decimation)
{
	if (!input)
		return vtkPolyDataPtr();

	//Shrink input volume
	vtkImageShrink3DPtr shrinker = vtkImageShrink3DPtr::New();
	if(reduceResolution)
	{
//		std::cout << "smooth" << std::endl;
		shrinker->SetInputData(input);
		shrinker->SetShrinkFactors(2,2,2);
		shrinker->Update();
	}

	// Find countour
	vtkMarchingCubesPtr convert = vtkMarchingCubesPtr::New();
	if(reduceResolution)
		convert->SetInputConnection(shrinker->GetOutputPort());
	else
		convert->SetInputData(input);

	convert->SetValue(0, threshold);
	convert->Update();

	vtkPolyDataPtr cubesPolyData = convert->GetOutput();
//	vtkPolyDataPtr cubesPolyData = vtkPolyDataPtr::New();
//	cubesPolyData = convert->GetOutput();
//	std::cout << "convert->GetOutput(); " << cubesPolyData.GetPointer() << std::endl;

	// Smooth surface model
	vtkWindowedSincPolyDataFilterPtr smoother = vtkWindowedSincPolyDataFilterPtr::New();
	if(smoothing)
	{
		smoother->SetInputData(cubesPolyData);
		smoother->SetNumberOfIterations(15);// Higher number = more smoothing
		smoother->SetBoundarySmoothing(false);
		smoother->SetFeatureEdgeSmoothing(false);
		smoother->SetNormalizeCoordinates(true);
		smoother->SetFeatureAngle(120);
		smoother->SetPassBand(0.3);//Lower number = more smoothing
		smoother->Update();
		cubesPolyData = smoother->GetOutput();
	}

	//Create a surface of triangles

	//Decimate surface model (remove a percentage of the polygons)
	vtkTriangleFilterPtr trifilt = vtkTriangleFilterPtr::New();
	vtkDecimateProPtr deci = vtkDecimateProPtr::New();
	vtkPolyDataNormalsPtr normals = vtkPolyDataNormalsPtr::New();
	if (decimation > 0.000001)
	{
		trifilt->SetInputData(cubesPolyData);
		trifilt->Update();
		deci->SetInputConnection(trifilt->GetOutputPort());
		deci->SetTargetReduction(decimation);
		deci->SetPreserveTopology(preserveTopology);
		//    deci->PreserveTopologyOn();
		deci->Update();
		cubesPolyData = deci->GetOutput();
	}

	normals->SetInputData(cubesPolyData);
	normals->Update();

	cubesPolyData->DeepCopy(normals->GetOutput());

	return cubesPolyData;
}

bool ContourFilter::postProcess()
{
	if (!mRawResult)
		return false;

	ImagePtr input = this->getCopiedInputImage();

	if (!input)
		return false;

	ColorDataAdapterXmlPtr colorOption = this->getColorOption(mOptions);
	MeshPtr output = this->postProcess(mRawResult, input, colorOption->getValue());
	mRawResult = NULL;

	if (output)
		mOutputTypes.front()->setValue(output->getUid());

	return true;
}

MeshPtr ContourFilter::postProcess(vtkPolyDataPtr contour, ImagePtr base, QColor color)
{
	if (!contour || !base)
		return MeshPtr();

	QString uid = base->getUid() + "_ge%1";
	QString name = base->getName()+" ge%1";
	MeshPtr output = dataManager()->createMesh(contour, uid, name, "");
	if (!output)
		return MeshPtr();

	output->get_rMd_History()->setRegistration(base->get_rMd());
	output->get_rMd_History()->setParentSpace(base->getUid());

	output->setColor(color);

	dataManager()->loadData(output);
	dataManager()->saveData(output, patientService()->getPatientData()->getActivePatientFolder());

	return output;
}

} // namespace cx
