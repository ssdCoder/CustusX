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

#include <QTimer>
#include <QApplication>

#include "ctkDICOMDatabase.h"
#include "ctkDICOMIndexer.h"

#include "catch.hpp"

#include "cxDicomConverter.h"
#include "cxReporter.h"
#include "cxDataReaderWriter.h"
#include "cxImage.h"
#include "cxTypeConversions.h"
#include "vtkImageMathematics.h"
#include "vtkImageAccumulate.h"
#include "vtkImageData.h"
#include "cxDataLocations.h"
#include "cxVolumeHelpers.h"
#include "org_custusx_dicom_Export.h"



typedef vtkSmartPointer<vtkImageAccumulate> vtkImageAccumulatePtr;
typedef vtkSmartPointer<vtkImageMathematics> vtkImageMathematicsPtr;
typedef vtkSmartPointer<vtkMetaImageWriter> vtkMetaImageWriterPtr;
typedef QSharedPointer<ctkDICOMDatabase> ctkDICOMDatabasePtr;

class DicomConverterTestFixture
{
public:
	DicomConverterTestFixture()
	{
		cx::DataLocations::setTestMode();
	}

	void checkImagesEqual(vtkImageDataPtr input1, vtkImageDataPtr input2)
	{
		REQUIRE(input1.Get()!=(vtkImageData*)NULL);
		REQUIRE(input2.Get()!=(vtkImageData*)NULL);
		REQUIRE(input1->GetDataDimension() == input2->GetDataDimension());
		REQUIRE(input1->GetScalarType() == input2->GetScalarType());
		REQUIRE(input1->GetNumberOfScalarComponents() == input2->GetNumberOfScalarComponents());
		REQUIRE(Eigen::Array3i(input1->GetDimensions()).isApprox(Eigen::Array3i(input2->GetDimensions())));
		CHECK(Eigen::Array3d(input1->GetSpacing()).isApprox(Eigen::Array3d(input2->GetSpacing()), 1.0E-2));
		CHECK(Eigen::Array3d(input1->GetOrigin()).isApprox(Eigen::Array3d(input2->GetOrigin())));
		// check spacing, dim, type, origin

		vtkImageMathematicsPtr diff = vtkImageMathematicsPtr::New();
		diff->SetOperationToSubtract();
		diff->SetInput1Data(input1);
		diff->SetInput2Data(input2);
		diff->Update();

		vtkImageAccumulatePtr histogram = vtkImageAccumulatePtr::New();
		histogram->SetInputData(0, diff->GetOutput());
		histogram->Update();

		Eigen::Array3d histogramRange = Eigen::Array3d(histogram->GetMax()) - Eigen::Array3d(histogram->GetMin());

		for (int i=0; i<input1->GetNumberOfScalarComponents(); ++i)
		{
			CHECK(histogramRange[i] <  0.01);
			CHECK(histogramRange[i] > -0.01);
		}
	}

	void checkImagesEqual(cx::ImagePtr input1, cx::ImagePtr input2)
	{
		REQUIRE(input1);
		REQUIRE(input2);
		CHECK(input1->getModality() == input2->getModality());
		CHECK(cx::similar(input1->getInitialWindowLevel(), input2->getInitialWindowLevel()));
		CHECK(cx::similar(input1->getInitialWindowWidth(), input2->getInitialWindowWidth()));
		CHECK(cx::similar(input1->get_rMd(), input2->get_rMd(), 1.0E-2));
		checkImagesEqual(input1->getBaseVtkImageData(), input2->getBaseVtkImageData());
	}

	cx::ImagePtr loadImageFromFile(QString filename, QString uid)
	{
		cx::ImagePtr image = cx::Image::create(uid,uid);
		cx::MetaImageReader().readInto(image, filename);
		return image;
	}

	QString getDatabaseFileName() const
	{
		return cx::DataLocations::getTestDataPath()+"/temp/testDatabase";
	}

	void eraseDatabase()
	{
		QString databaseFileName = this->getDatabaseFileName();
		QFile(databaseFileName).remove();
	}

	ctkDICOMDatabasePtr openDatabase()
	{
		QString databaseFileName = this->getDatabaseFileName();

		QString dbPath = QFileInfo(databaseFileName).absolutePath();
		QDir(dbPath).mkpath(".");

		ctkDICOMDatabasePtr DICOMDatabase;
		DICOMDatabase = ctkDICOMDatabasePtr(new ctkDICOMDatabase);
		DICOMDatabase->openDatabase( databaseFileName );

		return DICOMDatabase;
	}

	ctkDICOMDatabasePtr loadDirectory(QString folder)
	{
		this->eraseDatabase();
		ctkDICOMDatabasePtr DICOMDatabase = this->openDatabase();
		QSharedPointer<ctkDICOMIndexer> DICOMIndexer = QSharedPointer<ctkDICOMIndexer> (new ctkDICOMIndexer);
		DICOMIndexer->addDirectory(*DICOMDatabase,folder,"");

		return DICOMDatabase;
	}

	QString getOneFromList(QStringList strings)
	{
		REQUIRE(strings.size()==1);
		return strings.front();
	}
};

TEST_CASE("DicomConverter: Fixture test", "[unit][plugins][org.custusx.dicom]")
{
	DicomConverterTestFixture fixture;

	cx::ImagePtr flatImage = cx::Image::create("uid1", "name1");
	vtkImageDataPtr flatVtkImage = cx::generateVtkImageDataSignedShort(Eigen::Array3i(30,30,20), cx::Vector3D(1,1,1), 1);
	flatImage->setVtkImageData(flatVtkImage);

	cx::ImagePtr zeroImage = cx::Image::create("uid1", "name1");
	vtkImageDataPtr zeroVtkImage = cx::generateVtkImageDataSignedShort(Eigen::Array3i(30,30,20), cx::Vector3D(1,1,1), 0);
	zeroImage->setVtkImageData(zeroVtkImage);

	fixture.checkImagesEqual(flatImage, flatImage);
//	fixture.checkImagesEqual(flatImage, flatImage);
//	fixture.checkImagesEqual(referenceImage, referenceImage);
	CHECK(true);
}

TEST_CASE("DicomConverter: Open database", "[unit][plugins][org.custusx.dicom]")
{
	cx::Reporter::initialize();

	SSC_LOG("");
	DicomConverterTestFixture fixture;
	fixture.eraseDatabase();
	SSC_LOG("");
	ctkDICOMDatabasePtr db = fixture.openDatabase();
	SSC_LOG("");

	CHECK(db->isOpen());
	SSC_LOG("");

	cx::Reporter::shutdown();
}

TEST_CASE("DicomConverter: Convert Kaisa", "[integration][plugins][org.custusx.dicom]")
{
	cx::Reporter::initialize();
	SSC_LOG("");
	bool verbose = true;
	DicomConverterTestFixture fixture;
	// input  I: kaisa dicom data -> pass dicom data through converter
	// input II: kaisa mhd file   -> load
	//
	// verify converter output equals loaded mhd file output

	QString inputDicomDataDirectory = cx::DataLocations::getTestDataPath()+"/Phantoms/Kaisa/DICOM/";
	QString referenceImageFilename = cx::DataLocations::getTestDataPath()+"/Phantoms/Kaisa/MetaImage/Kaisa.mhd";

	SSC_LOG("");
	ctkDICOMDatabasePtr db = fixture.loadDirectory(inputDicomDataDirectory);
	SSC_LOG("");

	QString patient = fixture.getOneFromList(db->patients());
	QString study = fixture.getOneFromList(db->studiesForPatient(patient));
	QString series = fixture.getOneFromList(db->seriesForStudy(study));
	QStringList files = db->filesForSeries(series);
	SSC_LOG("");

	if (verbose)
	{
		std::cout << "patient " << patient << std::endl;
		std::cout << "study " << study << std::endl;
		std::cout << "series " << series << std::endl;
		std::cout << "files " << files.join("\n") << std::endl;
	}

	cx::DicomConverter converter;
	SSC_LOG("");
	converter.setDicomDatabase(db.data());
	SSC_LOG("");
	cx::ImagePtr convertedImage = converter.convertToImage(series);
	SSC_LOG("");

	cx::ImagePtr referenceImage = fixture.loadImageFromFile(referenceImageFilename, "reference");
	referenceImage->setModality("SC"); // hack: "SC" is not supported by mhd, it is instead set to "OTHER"

	if (verbose)
	{
		std::cout << "image: " << streamXml2String(*referenceImage) << std::endl;
		referenceImage->getBaseVtkImageData()->Print(std::cout);
		if (convertedImage)
		{
			std::cout << "converted: " << streamXml2String(*convertedImage) << std::endl;
			convertedImage->getBaseVtkImageData()->Print(std::cout);
			cx::MetaImageReader().saveImage(convertedImage, cx::DataLocations::getTestDataPath()+"/temp/kaisa_series5353_out.mhd");
		}
	}

	fixture.checkImagesEqual(referenceImage, referenceImage); //
	fixture.checkImagesEqual(convertedImage, referenceImage);
}

