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

#include "cxViewContainer.h"

//#include "sscViewContainer.h"
#include <QtGui>
#include <vtkImageActor.h>
#include <vtkImageData.h>
#include "cxVector3D.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
//#ifdef check
//#undef check
//#endif

#include "cxRep.h"
#include "cxTypeConversions.h"
#include "cxReporter.h"
#include "cxBoundingBox3D.h"
#include "cxTransform3D.h"
#include <QGridLayout>
#include "cxLogger.h"

namespace cx
{

ViewContainer::ViewContainer(QWidget *parent, Qt::WindowFlags f) :
	QVTKWidget(parent, f),
	mMouseEventTarget(NULL)
{
	this->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(customContextMenuRequestedSlot(const QPoint &)));
	mMTimeHash = 0;
	mMouseEventTarget = NULL;
	this->setLayout(new QGridLayout);
}

ViewContainer::~ViewContainer()
{
}

/**
  * Clears view container, deleting all layout objects
  */
void ViewContainer::clear()
{
	QLayoutItem *item;
	while ((item = getGridLayout()->takeAt(0)) != 0)
	{
		ViewItem* viewItem = dynamic_cast<ViewItem*>(item);
		delete viewItem;
	}
	this->setStretchFactors(LayoutRegion(0, 0, 10, 10), 0);
	this->setModified();
	mMouseEventTarget = NULL;
}

/**
  * Return this widget's grid layout object
  */
QGridLayout* ViewContainer::getGridLayout()
{
	return (QGridLayout*) layout();
}

void ViewContainer::paintEvent(QPaintEvent* event)
{
	inherited_widget::paintEvent(event);
	this->setModified();
}

void ViewContainer::setModified()
{
	if (this->getGridLayout())
	{
		for (int i = 0; i < this->getGridLayout()->count(); ++i)
		{
			this->getViewItem(i)->getView()->setModified();
		}
	}
	mMTimeHash = 0;
}

ViewItem* ViewContainer::getViewItem(int index)
{
	return dynamic_cast<ViewItem*>(this->getGridLayout()->itemAt(index));
}

/**
  * Creates and adds a view to this container.
  * Returns a pointer to the created view item that the container owns.
  */
ViewItem *ViewContainer::addView(QString uid, LayoutRegion region, QString name)
{
	this->initializeRenderWindow();

	// Create a viewItem for this view
	ViewItem *item = new ViewItem(uid, name, this, mRenderWindow, QRect());
	if (getGridLayout())
		getGridLayout()->addItem(item,
								 region.pos.row, region.pos.col,
								 region.span.row, region.span.col);
	this->setStretchFactors(region, 1);

	return item;
}

void ViewContainer::setStretchFactors(LayoutRegion region, int stretchFactor)
{
	// set stretch factors for the affected cols to 1 in order to get even distribution
	for (int i = region.pos.col; i < region.pos.col + region.span.col; ++i)
	{
		getGridLayout()->setColumnStretch(i, stretchFactor);
	}
	// set stretch factors for the affected rows to 1 in order to get even distribution
	for (int i = region.pos.row; i < region.pos.row + region.span.row; ++i)
	{
		getGridLayout()->setRowStretch(i, stretchFactor);
	}
}

void ViewContainer::initializeRenderWindow()
{
	if (mRenderWindow)
		return;

	mRenderWindow = vtkRenderWindowPtr::New();
	this->SetRenderWindow(mRenderWindow);

	this->addBackgroundRenderer();
}

void ViewContainer::addBackgroundRenderer()
{
	vtkRendererPtr renderer = vtkRendererPtr::New();
	mRenderWindow->AddRenderer(renderer);
	renderer->SetViewport(0,0,1,1);
	QColor background = palette().color(QPalette::Background);
	renderer->SetBackground(background.redF(), background.greenF(), background.blueF());
}

void ViewContainer::customContextMenuRequestedSlot(const QPoint& point)
{
	SSC_LOG("");

	ViewItem* item = this->findViewItem(point);
	if (!item)
		return;

	QWidget* sender = dynamic_cast<QWidget*>(this->sender());
	QPoint pointGlobal = sender->mapToGlobal(point);

	item->customContextMenuRequestedGlobalSlot(pointGlobal);
}

void ViewContainer::mouseMoveEvent(QMouseEvent* event)
{
	inherited_widget::mouseMoveEvent(event);
	this->handleMouseMove(event->pos(), event->buttons());
}

void ViewContainer::handleMouseMove(const QPoint &pos, const Qt::MouseButtons &buttons)
{
	if (mMouseEventTarget)
	{
		QRect r = mMouseEventTarget->geometry();
		QPoint p = pos;
		mMouseEventTarget->mouseMoveSlot(p.x() - r.left(), p.y() - r.top(), buttons);
	}
}

void ViewContainer::mousePressEvent(QMouseEvent* event)
{
//	SSC_LOG("");
	// special case for CustusX: when context menu is opened, mousereleaseevent is never called.
	// this sets the render interactor in a zoom state after each menu call. This hack prevents
	// the mouse press event in this case.
	// NOTE: this doesnt seem to be the case in this class - investigate
	if ((this->contextMenuPolicy() == Qt::CustomContextMenu) && event->buttons().testFlag(Qt::RightButton))
		return;
	SSC_LOG("");

	inherited_widget::mousePressEvent(event);
//	this->handleMousePress(event->pos(), event->buttons());

	mMouseEventTarget = this->findViewItem(event->pos());
	if (!mMouseEventTarget)
		return;
	QPoint pos_t = this->convertToItemSpace(event->pos(), mMouseEventTarget);
	mMouseEventTarget->mousePressSlot(pos_t.x(), pos_t.y(), event->buttons());
}

//void ViewContainer::handleMousePress(const QPoint &pos, const Qt::MouseButtons & buttons)
//{
//	mMouseEventTarget = this->findViewItem(pos);
//	if (!mMouseEventTarget)
//		return;
//	QPoint pos_t = this->convertToItemSpace(pos, mMouseEventTarget);
//	mMouseEventTarget->mousePressSlot(pos_t.x(), pos_t.y(), buttons);

////	for (int i = 0; getGridLayout() && i < getGridLayout()->count(); ++i)
////	{
////		ViewItem *item = this->getViewItem(i);
////		QRect r = item->geometry();
////		if (r.contains(pos))
////		{
////			mMouseEventTarget = item;
////			item->mousePressSlot(pos.x() - r.left(), pos.y() - r.top(), buttons);
////		}
////	}
//}

QPoint ViewContainer::convertToItemSpace(const QPoint &pos, ViewItem* item) const
{
	QRect r = item->geometry();
	QPoint retval(pos.x() - r.left(), pos.y() - r.top());
	return retval;
}

ViewItem* ViewContainer::findViewItem(const QPoint &pos)
{
	for (int i = 0; getGridLayout() && i < getGridLayout()->count(); ++i)
	{
		ViewItem *item = this->getViewItem(i);
		QRect r = item->geometry();
		if (r.contains(pos))
			return item;
	}
	return NULL;
}

void ViewContainer::mouseReleaseEvent(QMouseEvent* event)
{
	SSC_LOG("");

	inherited_widget::mouseReleaseEvent(event);
	this->handleMouseRelease(event->pos(), event->buttons());
}

void ViewContainer::handleMouseRelease(const QPoint &pos, const Qt::MouseButtons &buttons)
{
	if (mMouseEventTarget)
	{
		QRect r = mMouseEventTarget->geometry();
		QPoint p = pos;
		mMouseEventTarget->mouseReleaseSlot(p.x() - r.left(), p.y() - r.top(), buttons);
		mMouseEventTarget = NULL;
	}
}

void ViewContainer::focusInEvent(QFocusEvent* event)
{
	inherited_widget::focusInEvent(event);
}

void ViewContainer::wheelEvent(QWheelEvent* event)
{
	inherited_widget::wheelEvent(event);
	for (int i = 0; layout() && i < layout()->count(); ++i)
	{
		ViewItem *item = dynamic_cast<ViewItem*>(layout()->itemAt(i));
		QRect r = item->geometry();
		QPoint p = event->pos();
		if (r.contains(p))
		{
			item->mouseWheelSlot(p.x() - r.left(), p.y() - r.top(), event->delta(), event->orientation(), event->buttons());
		}
	}
}

void ViewContainer::showEvent(QShowEvent* event)
{
	inherited_widget::showEvent(event);
}

void ViewContainer::renderAll()
{
	// First, calculate if anything has changed
	long hash = 0;
	for (int i = 0; getGridLayout() && i < getGridLayout()->count(); ++i)
	{
		ViewItem *item = this->getViewItem(i);
		hash += item->getView()->computeTotalMTime();
	}
	// Then, if anything has changed, render everything anew
	if (hash != mMTimeHash)
	{
		this->doRender();
		mMTimeHash = hash;
	}
}

void ViewContainer::doRender()
{
	this->getRenderWindow()->Render();
}

void ViewContainer::resizeEvent( QResizeEvent *event)
{
	inherited_widget::resizeEvent(event);
	this->setModified();
	this->getGridLayout()->update();
}


} /* namespace cx */
