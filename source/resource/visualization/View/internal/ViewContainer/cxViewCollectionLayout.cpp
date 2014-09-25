/*
 * cxViewCollectionLayout.cpp
 *
 *  Created on: Sep 16, 2014
 *      Author: christiana
 */

#include "cxViewCollectionLayout.h"

#include <QGridLayout>
#include "cxLogger.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "cxReporter.h"

#include "cxViewContainer.h"

namespace cx
{

LayoutWidgetUsingViewCollection::LayoutWidgetUsingViewCollection()
{
	QVBoxLayout* layout = new QVBoxLayout(this);
	this->setLayout(layout);
	layout->setSpacing(0);
	layout->setMargin(0);
	mViewContainer = new ViewContainer;
	mViewContainer->getGridLayout()->setSpacing(2);
	mViewContainer->getGridLayout()->setMargin(4);
	layout->addWidget(mViewContainer);
}

LayoutWidgetUsingViewCollection::~LayoutWidgetUsingViewCollection()
{
}

ViewPtr LayoutWidgetUsingViewCollection::addView(View::Type type, LayoutRegion region)
{
	static int nameGenerator = 0;
	QString uid = QString("view-%1-%2")
			.arg(nameGenerator++)
			.arg(reinterpret_cast<long>(this));

	ViewItem* viewItem = mViewContainer->addView(uid, region, uid);
	ViewPtr view = viewItem->getView();

	viewItem->getView()->setType(type);
	mViews.push_back(view);
	return view;
}

void LayoutWidgetUsingViewCollection::clearViews()
{
	mViews.clear();
	mViewContainer->clear();
}

void LayoutWidgetUsingViewCollection::setModified()
{
	mViewContainer->setModified();
}

void LayoutWidgetUsingViewCollection::render()
{
	mViewContainer->renderAll();
}


} /* namespace cx */
