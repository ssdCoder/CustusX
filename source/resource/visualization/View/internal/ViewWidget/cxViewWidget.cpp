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


#include "cxViewWidget.h"
#include <QtWidgets>

#include <QApplication>
#include <QDesktopWidget>
#include <vtkImageActor.h>
#include <vtkImageData.h>
#include "cxVector3D.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "cxLogger.h"
#ifdef check
#undef check
#endif

#include "cxRep.h"
#include "cxTypeConversions.h"
#include "cxReporter.h"
#include "cxBoundingBox3D.h"
#include "cxTransform3D.h"
#include "cxViewLinkingViewWidget.h"

namespace cx
{


///--------------------------------------------------------

ViewWidget::ViewWidget(const QString& uid, const QString& name, QWidget *parent, Qt::WindowFlags f) :
	inherited(parent, f)
{
	this->setContextMenuPolicy(Qt::CustomContextMenu);
	mZoomFactor = -1.0;
	mView = ViewLinkingViewWidget::create(this, vtkRenderWindowPtr::New());
	connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(customContextMenuRequestedSlot(const QPoint &)));
	this->SetRenderWindow(mView->getRenderWindow());
	mView->getRenderWindow()->GetInteractor()->EnableRenderOff();
	mView->clear();
}

void ViewWidget::customContextMenuRequestedSlot(const QPoint& point)
{
	QWidget* sender = dynamic_cast<QWidget*>(this->sender());
	QPoint pointGlobal = sender->mapToGlobal(point);
	emit customContextMenuRequestedInGlobalPos(pointGlobal);
}

ViewRepCollectionPtr ViewWidget::getView()
{
	return mView;
}

ViewWidget::~ViewWidget()
{
	this->getView()->clear();
}

vtkRendererPtr ViewWidget::getRenderer()
{
	return this->getView()->getRenderer();
}

void ViewWidget::resizeEvent(QResizeEvent * event)
{
	inherited::resizeEvent(event);
	QSize size = event->size();
	vtkRenderWindowInteractor* iren = mView->getRenderWindow()->GetInteractor();
	if (iren != NULL)
		iren->UpdateSize(size.width(), size.height());
	emit resized(size);
}

void ViewWidget::mouseMoveEvent(QMouseEvent* event)
{
	inherited::mouseMoveEvent(event);
	emit mouseMove(event->x(), event->y(), event->buttons());
}

void ViewWidget::mousePressEvent(QMouseEvent* event)
{
	// special case for CustusX: when context menu is opened, mousereleaseevent is never called.
	// this sets the render interactor in a zoom state after each menu call. This hack prevents
	// the mouse press event in this case.
	if ((this->contextMenuPolicy() == Qt::CustomContextMenu) && event->buttons().testFlag(Qt::RightButton))
		return;

	inherited::mousePressEvent(event);
	emit mousePress(event->x(), event->y(), event->buttons());
}

void ViewWidget::mouseReleaseEvent(QMouseEvent* event)
{
	inherited::mouseReleaseEvent(event);
	emit mouseRelease(event->x(), event->y(), event->buttons());
}

void ViewWidget::focusInEvent(QFocusEvent* event)
{
	inherited::focusInEvent(event);
	emit focusChange(event->gotFocus(), event->reason());
}

void ViewWidget::wheelEvent(QWheelEvent* event)
{
	inherited::wheelEvent(event);
	emit mouseWheel(event->x(), event->y(), event->delta(), event->orientation(), event->buttons());
}

void ViewWidget::showEvent(QShowEvent* event)
{
	inherited::showEvent(event);
	emit shown();
}

void ViewWidget::paintEvent(QPaintEvent* event)
{
	mView->setModified();
	inherited::paintEvent(event);
}

void ViewWidget::setZoomFactor(double factor)
{
	if (similar(factor, mZoomFactor))
	{
		return;
	}
	mZoomFactor = factor;
	emit resized(this->size());
}

double ViewWidget::getZoomFactor() const
{
	return mZoomFactor;
}

DoubleBoundingBox3D ViewWidget::getViewport_s() const
{
	return transform(this->get_vpMs().inv(), this->getViewport());
}

Transform3D ViewWidget::get_vpMs() const
{
	Vector3D center_vp = this->getViewport().center();
	double scale = mZoomFactor / this->mmPerPix(); // double zoomFactor = 0.3; // real magnification
	Transform3D S = createTransformScale(Vector3D(scale, scale, scale));
	Transform3D T = createTransformTranslate(center_vp);// center of viewport in viewport coordinates
	Transform3D M_vp_w = T * S; // first scale , then translate to center.
	return M_vp_w;
}

/**return the pixel viewport.
 */
DoubleBoundingBox3D ViewWidget::getViewport() const
{
	return DoubleBoundingBox3D(0, size().width(), 0, size().height(), 0, 0);
}

double ViewWidget::mmPerPix() const
{
	// use mean mm/pix over entire screen. DONT use the height of the widget in mm,
	// this is truncated to the nearest integer.
	QDesktopWidget* desktop = dynamic_cast<QApplication*>(QApplication::instance())->desktop();
	QWidget* screen = desktop->screen(desktop->screenNumber(this));
	double r_h = (double) screen->heightMM() / (double) screen->geometry().height();
	double r_w = (double) screen->widthMM() / (double) screen->geometry().width();
	double retval = (r_h + r_w) / 2.0;
	return retval;
}

} // namespace cx
