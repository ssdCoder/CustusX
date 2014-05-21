// This file is part of SSC,
// a C++ Library supporting Image Guided Therapy Applications.
//
// Copyright (C) 2008- SINTEF Medical Technology
// Copyright (C) 2008- Sonowand AS
//
// SSC is owned by SINTEF Medical Technology and Sonowand AS,
// hereafter named the owners. Each particular piece of code
// is owned by the part that added it to the library.
// SSC source code and binaries can only be used by the owners
// and those with explicit permission from the owners.
// SSC shall not be distributed to anyone else.
//
// SSC is distributed WITHOUT ANY WARRANTY; without even
// the implied warranty of MERCHANTABILITY or FITNESS FOR
// A PARTICULAR PURPOSE.
//
// See sscLicense.txt for more information.

#ifndef CXIMAGETF3D_H_
#define CXIMAGETF3D_H_

#include <boost/shared_ptr.hpp>
#include <QObject>
#include "vtkForwardDeclarations.h"

class QColor;
class QDomDocument;
class QDomNode;

#include <map>
#include <boost/shared_ptr.hpp>
#include "cxImageTFData.h"

namespace cx
{

typedef boost::shared_ptr<class ImageTF3D> ImageTF3DPtr;


/**\brief Handler for the transfer functions used in 3d image volumes.
 *
 * Used by Image.
 *
 * Set the basic lut using either setLut() or setColorPoint(), then modify it with window and level.
 * Set the alpha channel using setAlphaPoint(), or override it by creating a opacity step function
 * with LLR and Alpha.
 *
 * The volume rendering classes can use the data by getting OpacityTF and ColorTF.
 *
 * \ingroup cx_resource_core_data
 *  Created on: Jan 9, 2009
 *      Author: christiana
 */
class ImageTF3D: public ImageTFData
{
Q_OBJECT
public:
	ImageTF3D();

//	void setInitialTFFromImage(vtkImageDataPtr base);
	ImageTF3DPtr createCopy();

	vtkPiecewiseFunctionPtr getOpacityTF();
	vtkColorTransferFunctionPtr getColorTF();

protected:
	virtual void internalsHaveChanged();
private:
	void buildOpacityMapFromLLRAlpha();
	void refreshColorTF();
	void refreshOpacityTF();

	//vtkPiecewiseFunctionPtr mGradientOpacityTF; // implement when needed.
	vtkPiecewiseFunctionPtr mOpacityTF;
	vtkColorTransferFunctionPtr mColorTF;
};

} // end namespace cx

#endif /* CXIMAGETF3D_H_ */