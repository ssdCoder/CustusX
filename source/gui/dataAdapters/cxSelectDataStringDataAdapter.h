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
#ifndef CXSELECTDATASTRINGDATAADAPTER_H
#define CXSELECTDATASTRINGDATAADAPTER_H

#include "cxSelectDataStringDataAdapterBase.h"

namespace cx
{

typedef boost::shared_ptr<class ActiveImageStringDataAdapter> ActiveImageStringDataAdapterPtr;
/** Adapter that connects to the current active image.
 * Example: Active image: [DataName]
 * where active image is the value
 * and DataName is taken from the valuerange
 */
class ActiveImageStringDataAdapter : public SelectDataStringDataAdapterBase
{
  Q_OBJECT
public:
	static ActiveImageStringDataAdapterPtr New(ctkPluginContext *pluginContext) { return ActiveImageStringDataAdapterPtr(new ActiveImageStringDataAdapter(pluginContext)); }
	ActiveImageStringDataAdapter(ctkPluginContext *pluginContext);
  virtual ~ActiveImageStringDataAdapter() {}

public: // basic methods
  virtual bool setValue(const QString& value);
  virtual QString getValue() const;
};


typedef boost::shared_ptr<class SelectImageStringDataAdapter> SelectImageStringDataAdapterPtr;
/** Adapter that selects and stores an image.
 * The image is stored internally in the adapter.
 * Use setValue/getValue plus changed() to access it.
 */
class SelectImageStringDataAdapter : public SelectDataStringDataAdapterBase
{
  Q_OBJECT
public:
	static SelectImageStringDataAdapterPtr New(ctkPluginContext *pluginContext) { return SelectImageStringDataAdapterPtr(new SelectImageStringDataAdapter(pluginContext)); }
  virtual ~SelectImageStringDataAdapter() {}

public: // basic methods
  virtual bool setValue(const QString& value);
  virtual QString getValue() const;

public: // interface extension
  ImagePtr getImage();

protected:
	SelectImageStringDataAdapter(ctkPluginContext *pluginContext);
private:
  QString mImageUid;
};

typedef boost::shared_ptr<class SelectDataStringDataAdapter> SelectDataStringDataAdapterPtr;
/** Adapter that selects and stores a data.
 * The data is stored internally in the adapter.
 * Use setValue/getValue plus changed() to access it.
 */
class SelectDataStringDataAdapter : public SelectDataStringDataAdapterBase
{
  Q_OBJECT
public:
	static SelectDataStringDataAdapterPtr New(ctkPluginContext *pluginContext) { return SelectDataStringDataAdapterPtr(new SelectDataStringDataAdapter(pluginContext)); }
  virtual ~SelectDataStringDataAdapter() {}

public: // basic methods
  virtual bool setValue(const QString& value);
  virtual QString getValue() const;

public: // interface extension
  virtual DataPtr getData() const;

protected:
	SelectDataStringDataAdapter(ctkPluginContext *pluginContext);
private:
//  DataPtr mData;
  QString mUid;

};

typedef boost::shared_ptr<class SelectMeshStringDataAdapter> SelectMeshStringDataAdapterPtr;
/** Adapter that selects and stores an mesh.
 * The image is stored internally in the adapter.
 * Use setValue/getValue plus changed() to access it.
 */
class SelectMeshStringDataAdapter : public SelectDataStringDataAdapterBase
{
  Q_OBJECT
public:
	static SelectMeshStringDataAdapterPtr New(ctkPluginContext *pluginContext) { return SelectMeshStringDataAdapterPtr(new SelectMeshStringDataAdapter(pluginContext)); }
  virtual ~SelectMeshStringDataAdapter() {}

public: // basic methods
  virtual bool setValue(const QString& value);
  virtual QString getValue() const;

public: // interface extension
  MeshPtr getMesh();

protected:
	SelectMeshStringDataAdapter(ctkPluginContext *pluginContext);
private:
  QString mMeshUid;
};


} // namespace cx

#endif // CXSELECTDATASTRINGDATAADAPTER_H
