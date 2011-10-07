/*
 *  sscReconstructer.h
 *  Created by Ole Vegard Solberg on 5/4/10.
 *
 */

#ifndef SSCRECONSTRUCTER_H_
#define SSCRECONSTRUCTER_H_

#include <QObject>
#include <QThread>
#include <math.h>
#include "sscReconstructAlgorithm.h"
#include "sscBoundingBox3D.h"
#include "sscReconstructedOutputVolumeParams.h"
#include "sscStringDataAdapterXml.h"
#include "sscDoubleDataAdapterXml.h"
#include "sscBoolDataAdapterXml.h"
#include "sscXmlOptionItem.h"
#include "sscProbeSector.h"

namespace ssc
{

class ReconstructParams : public QObject
{
	Q_OBJECT

public:
	ReconstructParams(XmlOptionFile settings);
	virtual ~ReconstructParams();

	StringDataAdapterXmlPtr mOrientationAdapter;
	StringDataAdapterXmlPtr mPresetTFAdapter;
	StringDataAdapterXmlPtr mAlgorithmAdapter;
	std::vector<DataAdapterPtr> mAlgoOptions;
	StringDataAdapterXmlPtr mMaskReduce;//Reduce mask size in % in each direction
	BoolDataAdapterXmlPtr mAlignTimestamps; ///align track and frame timestamps to each other automatically
	DoubleDataAdapterXmlPtr mTimeCalibration; ///set a offset in the frame timestamps
	BoolDataAdapterXmlPtr mAngioAdapter; ///US angio data is used as input

	XmlOptionFile mSettings;

signals:
	void changedInputSettings();
};
typedef boost::shared_ptr<class ReconstructParams> ReconstructParamsPtr;

typedef boost::shared_ptr<class Reconstructer> ReconstructerPtr;

/**
 * Used coordinate systems:
 * u  = raw input Ultrasound frames (in x, y. Origin lower left.)
 * t  = Tool space for probe as defined in ssc:Tool (z in ray direction, y to the left)
 * s  = probe localizer Sensor.
 * pr = Patient Reference localizer sensor.
 * d  = Output Data space
 *
 */
class Reconstructer: public QObject
{
Q_OBJECT
	friend class ThreadedReconstructer;

public:
	Reconstructer(XmlOptionFile settings, QString shaderPath);
	virtual ~Reconstructer();

	void setInputData(USReconstructInputData fileData);
	void reconstruct(); // assumes readFiles has already been called

	ImagePtr getOutput();
	ReconstructParamsPtr mParams;
	std::vector<DataAdapterPtr> mAlgoOptions;

	ReconstructAlgorithmPtr mAlgorithm;///< The used reconstruction algorithm

	OutputVolumeParams getOutputVolumeParams() const;
	void setOutputVolumeParams(const OutputVolumeParams& par);
	void setOutputRelativePath(QString path);
	void setOutputBasePath(QString path);
	void clearAll();
	Transform3D interpolate(const Transform3D& a, const Transform3D& b, double t);
	Transform3D slerpInterpolate(const Transform3D& a, const Transform3D& b, double t);

public slots:
	void setSettings();

signals:
	void paramsChanged();
	void algorithmChanged();
	void inputDataSelected(QString mhdFileName);
	void reconstructFinished();

private:
	USReconstructInputData mFileData;
	USReconstructInputData mOriginalFileData; ///< original version of loaded data. Use as basis when recalculating due to changed params.

	OutputVolumeParams mOutputVolumeParams;
	XmlOptionFile mSettings;
	ImagePtr mOutput;///< Output image from reconstruction
	QString mOutputRelativePath;///< Relative path to the output image
	QString mOutputBasePath;///< Global path where the relative path starts, for the output image
	QString mShaderPath; ///< name of shader folder
	double mMaxTimeDiff; ///< The largest allowed time deviation for the positions used in the frame interpolations

	ssc::Transform3D applyOutputOrientation();
	void findExtentAndOutputTransform();
	void transformPositionsTo_prMu();

	void alignTimeSeries();
	void applyTimeCalibration();
	void calibrateTimeStamps(double timeOffset, double scale);
	void interpolatePositions();
	std::vector<ssc::Vector3D> generateInputRectangle();
	ImagePtr generateOutputVolume();
	void clearOutput();
	void createAlgorithm();
	void updateFromOriginalFileData();

	QString generateOutputUid();
	QString generateImageName(QString uid) const;

	void threadedPreReconstruct();
	void threadedReconstruct();
	void threadedPostReconstruct();

	bool validInputData() const;///< checks if internal states is valid (that it actually has frames to reconstruct)
};


}//namespace
#endif //SSCRECONSTRUCTER_H_
