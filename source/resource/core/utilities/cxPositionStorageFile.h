/*=========================================================================
This file is part of CustusX, an Image Guided Therapy Application.
                 
Copyright (c) SINTEF Department of Medical Technology.
All rights reserved.
                 
CustusX is released under a BSD 3-Clause license.
                 
See Lisence.txt (https://github.com/SINTEFMedtek/CustusX/blob/master/License.txt) for details.
=========================================================================*/

#ifndef CXPOSITIONSTORAGEFILE_H_
#define CXPOSITIONSTORAGEFILE_H_

#include "cxResourceExport.h"

#include <QString>
#include <QFile>
#include <QDataStream>
#include <boost/cstdint.hpp>

#include "cxTransform3D.h"

namespace cx {

/**\brief Reader class for the position file.
 * 
 * Each call to read() gives the next position entry from the file.
 * When atEnd() returns true, all positions have been read. 
 *
 *
 *
 *
 * Binary file format description
   \verbatim
  Header:
    "SNWPOS"<version>

  Entries, can be one of:

   * Change tool. All position entries following this one belongs to the given tool
       <type=2><size><toolUid>

   * Position. integer-format tool id are appended.
      <type=1><size><timestamp><position><int toolid>

   * Position. Requires change tool to have been called.
      <type=3><size><timestamp><position>

   The position field is <position> = <thetaXY><thetaZ><phi><x><y><z>
   Where the parameters are found from a matrix using the class CGFrame.
   \endverbatim
 *
 * \sa PositionStorageWriter
 * \ingroup cx_resource_core_utilities
 */
class cxResource_EXPORT PositionStorageReader
{
public:
	PositionStorageReader(QString filename);
	~PositionStorageReader();
	bool read(Transform3D* matrix, double* timestamp, int* toolIndex); // reads only tool data in integer format.
	bool read(Transform3D* matrix, double* timestamp, QString* toolUid);
	bool atEnd() const;
	static QString timestampToString(double timestamp);
	int version();
private:
	QString mCurrentToolUid; ///< the tool currently being written.
	QFile positions;
	QDataStream stream;
	quint8 mVersion;
	bool mError;
	class Frame3D frameFromStream();
};

typedef boost::shared_ptr<PositionStorageReader> PositionStorageReaderPtr;

/**\brief Writer class for the position file.
 * 
 * The generated file contains a compact representation
 * of tool position data along with timestamp and tool id.
 * Extract the info with class PositionStorageReader.
 *
 * For a description of the file format, see PositionStorageReader.
 *
 * \sa PositionStorageReader
 * \ingroup sscUtility
 */
class cxResource_EXPORT PositionStorageWriter
{
public:
	PositionStorageWriter(QString filename);
	~PositionStorageWriter();
	void write(Transform3D matrix, uint64_t timestamp, int toolIndex);
	void write(Transform3D matrix, uint64_t timestamp, QString toolUid);
private:
	QString mCurrentToolUid; ///< the tool currently being written.
	QFile positions;
	QDataStream stream;
};

} // namespace cx 


#endif /*CXPOSITIONSTORAGEFILE_H_*/
