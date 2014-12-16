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

#include "cxDicomImageReader.h"

#include "cxVolumeHelpers.h"
#include "dcvrpn.h"
#include "cxLogger.h"

namespace cx
{

DicomImageReaderPtr DicomImageReader::createFromFile(QString filename)
{
	DicomImageReaderPtr retval(new DicomImageReader);
	if (retval->loadFile(filename))
		return retval;
	else
		return DicomImageReaderPtr();
}

DicomImageReader::DicomImageReader() :
	mDataset(NULL)
{
}

bool DicomImageReader::loadFile(QString filename)
{
	mFilename = filename;
	OFCondition status = mFileFormat.loadFile(filename.toLatin1().data());
	if( !status.good() )
	{
		return false;
	}

	mDataset = mFileFormat.getDataset();
	return true;
}

ctkDICOMItemPtr DicomImageReader::item() const
{
	return this->wrapInCTK(mDataset);
}

double DicomImageReader::getDouble(const DcmTagKey& tag, const unsigned long pos, const OFBool searchIntoSub) const
{
	double retval = 0;
	OFCondition condition;
	condition = mDataset->findAndGetFloat64(tag, retval, pos, searchIntoSub);
	if (!condition.good())
	{
		QString tagName = this->item()->TagDescription(tag);
		this->error(QString("Failed to get tag %1/%2").arg(tagName).arg(pos));
	}
	return retval;
}

DicomImageReader::WindowLevel DicomImageReader::getWindowLevel() const
{
	WindowLevel retval;
	retval.center = this->getDouble(DCM_WindowCenter, 0, OFTrue);
	retval.width = this->getDouble(DCM_WindowWidth, 0, OFTrue);
	return retval;
}

int DicomImageReader::getNumberOfFrames() const
{
	int numberOfFrames = this->item()->GetElementAsInteger(DCM_NumberOfFrames);
	if (numberOfFrames==0)
	{
		unsigned short rows = 0;
		unsigned short columns = 0;
		mDataset->findAndGetUint16(DCM_Rows, rows, 0, OFTrue);
		mDataset->findAndGetUint16(DCM_Columns, columns, 0, OFTrue);
		if (rows*columns > 0)
			numberOfFrames = 1; // seems like we have a 2D image
	}
	return numberOfFrames;
}

Transform3D DicomImageReader::getImageTransformPatient() const
{
	Vector3D pos;
	Vector3D e_x;
	Vector3D e_y;

	for (int i=0; i<3; ++i)
	{
		OFCondition condition;
		e_x[i] = this->getDouble(DCM_ImageOrientationPatient, i, OFTrue);
		e_y[i] = this->getDouble(DCM_ImageOrientationPatient, i+3, OFTrue);
		pos[i] = this->getDouble(DCM_ImagePositionPatient, i, OFTrue);
	}

	Transform3D retval = cx::createTransformIJC(e_x, e_y, pos);
	return retval;
}

ctkDICOMItemPtr DicomImageReader::wrapInCTK(DcmItem* item) const
{
	if (!item)
		return ctkDICOMItemPtr();
	ctkDICOMItemPtr retval(new ctkDICOMItem);
	retval->InitializeFromItem(item);
	return retval;
}

void DicomImageReader::error(QString message) const
{
	reportError(QString("Dicom convert: [%1] in %2").arg(message).arg(mFilename));
}

vtkImageDataPtr DicomImageReader::createVtkImageData()
{
	DicomImage dicomImage(mFilename.toLatin1().data()); //, CIF_MayDetachPixelData );
	const DiPixel *pixels = dicomImage.getInterData();
	if (!pixels)
	{
		this->error("Found no pixel data");
		return vtkImageDataPtr();
	}

	vtkImageDataPtr data = vtkImageDataPtr::New();

	data->SetSpacing(this->getSpacing().data());

	Eigen::Array3i dim = this->getDim(dicomImage);
	data->SetExtent(0, dim[0]-1, 0, dim[1]-1, 0, dim[2]-1);

	int samplesPerPixel = pixels->getPlanes();
	int scalarSize = dim.prod() * samplesPerPixel;
	int pixelDepth = dicomImage.getDepth();

	switch (pixels->getRepresentation())
	{
	case EPR_Uint8:
//		std::cout << "  VTK_UNSIGNED_CHAR" << std::endl;
		data->AllocateScalars(VTK_UNSIGNED_CHAR, samplesPerPixel);
		break;
	case EPR_Uint16:
//		std::cout << "  VTK_UNSIGNED_SHORT" << std::endl;
		data->AllocateScalars(VTK_UNSIGNED_SHORT, samplesPerPixel);
		break;
	case EPR_Uint32:
		this->error("DICOM EPR_Uint32 not supported");
		return vtkImageDataPtr();
		break;
	case EPR_Sint8:
//		std::cout << "  VTK_CHAR" << std::endl;
		data->AllocateScalars(VTK_CHAR, samplesPerPixel);
		break;
	case EPR_Sint16:
//		std::cout << "  VTK_SHORT" << std::endl;
		data->AllocateScalars(VTK_SHORT, samplesPerPixel);
		break;
	case EPR_Sint32:
		this->error("DICOM EPR_Sint32 not supported");
		return vtkImageDataPtr();
		break;
	}

	int bytesPerPixel = data->GetScalarSize() * samplesPerPixel;

	memcpy(data->GetScalarPointer(), pixels->getData(), pixels->getCount()*bytesPerPixel);
	if (pixels->getCount()!=scalarSize)
		this->error("Mismatch in pixel counts");
	setDeepModified(data);
	return data;
}

Eigen::Array3d DicomImageReader::getSpacing() const
{
	Eigen::Array3d spacing;
	spacing[0] = this->getDouble(DCM_PixelSpacing, 0, OFTrue);
	spacing[1] = this->getDouble(DCM_PixelSpacing, 1, OFTrue);
	spacing[2] = this->getDouble(DCM_SliceThickness, 0, OFTrue);
//	std::cout << "  spacing: " << spacing << std::endl;
	return spacing;
}

Eigen::Array3i DicomImageReader::getDim(const DicomImage& dicomImage) const
{
	Eigen::Array3i dim;
	dim[0] = dicomImage.getWidth();
	dim[1] = dicomImage.getHeight();
	dim[2] = dicomImage.getFrameCount();
	return dim;
}

QString DicomImageReader::getPatientName() const
{
	QString rawName = this->item()->GetElementAsString(DCM_PatientName);
	return this->formatPatientName(rawName);
}

QString DicomImageReader::formatPatientName(QString rawName) const
{
	// ripped from ctkDICOMModel

	OFString dicomName = rawName.toStdString().c_str();
	OFString formattedName;
	OFString lastName, firstName, middleName, namePrefix, nameSuffix;
	OFCondition l_error = DcmPersonName::getNameComponentsFromString(dicomName,
																	 lastName, firstName, middleName, namePrefix, nameSuffix);
	if (l_error.good())
	{
		formattedName.clear();
		/* concatenate name components per this convention
   * Last, First Middle, Suffix (Prefix)
   * */
		if (!lastName.empty())
		{
			formattedName += lastName;
			if ( !(firstName.empty() && middleName.empty()) )
			{
				formattedName += ",";
			}
		}
		if (!firstName.empty())
		{
			formattedName += " ";
			formattedName += firstName;
		}
		if (!middleName.empty())
		{
			formattedName += " ";
			formattedName += middleName;
		}
		if (!nameSuffix.empty())
		{
			formattedName += ", ";
			formattedName += nameSuffix;
		}
		if (!namePrefix.empty())
		{
			formattedName += " (";
			formattedName += namePrefix;
			formattedName += ")";
		}
	}
	return QString(formattedName.c_str());
}


} // namespace cx

