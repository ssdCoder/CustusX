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

#ifndef SSCVIEWCONTAINER_H_
#define SSCVIEWCONTAINER_H_
#include "sscConfig.h"
#include <boost/shared_ptr.hpp>
#include "vtkForwardDeclarations.h"
#include "sscIndent.h"
#include <QLayoutItem>
class QColor;

#include "sscViewQVTKWidget.h"
#include "sscView.h"
#include "sscTransform3D.h"

namespace ssc
{
class DoubleBoundingBox3D;
typedef boost::shared_ptr<class Rep> RepPtr;

class ViewItem : public QObject, public View
{
Q_OBJECT

public:
	ViewItem(QWidget *parent, vtkRenderWindowPtr renderWindow, QSize size) : View(parent, size) { mSize = size; mRenderWindow = renderWindow; }
	~ViewItem() {}

	// Implement pure virtuals in base class
	virtual vtkRenderWindowPtr getRenderWindow() const { return mRenderWindow; }
	virtual QSize size() { return mSize; }
	virtual void setZoomFactor(double factor);
	virtual void setSize(QSize size) { mSize = size; emit resized(size); }
	void setRenderer(vtkRendererPtr renderer);

signals:
	void resized(QSize size);

private:
	vtkRenderWindowPtr mRenderWindow;
};
typedef boost::shared_ptr<ViewItem> ViewItemPtr;

/// More advanced N:1 combination of SSC Views and Qt Widgets
class ViewContainer : public ViewQVTKWidget
{
Q_OBJECT
	typedef ViewQVTKWidget widget;

public:
	ViewContainer(QWidget *parent = NULL, Qt::WFlags f = 0);
	~ViewContainer();
	ViewItemPtr getView(int view);
	void setupViews(int cols, int rows);
	void clear();

signals:
	void mouseMoveSignal(QMouseEvent *event);
	void mousePressSignal(QMouseEvent *event);
	void mouseReleaseSignal(QMouseEvent *event);
	void mouseWheelSignal(QWheelEvent*);
	void showSignal(QShowEvent *event);
	void focusInSignal(QFocusEvent *event);
	void resized(QSize size);

protected:
	QList<ViewItemPtr> mViews;
	vtkRenderWindowPtr mRenderWindow;
	int mRows, mCols;

private:
	virtual void showEvent(QShowEvent* event);
	virtual void wheelEvent(QWheelEvent*);
	virtual void mouseMoveEvent(QMouseEvent *event);
	virtual void mousePressEvent(QMouseEvent *event);
	virtual void mouseReleaseEvent(QMouseEvent *event);
	virtual void focusInEvent(QFocusEvent* event);
	virtual void resizeEvent(QResizeEvent *event);
	virtual void paintEvent(QPaintEvent *event);
};
typedef boost::shared_ptr<ViewContainer> ViewContainerPtr;

} // namespace ssc

#endif /*SSCVIEW_H_*/
