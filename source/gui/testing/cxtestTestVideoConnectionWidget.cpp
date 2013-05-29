#include "cxtestTestVideoConnectionWidget.h"

#include <QtTest/QtTest>
#include <QComboBox>
#include <QPushButton>
#include "sscVideoSource.h"
#include "sscStringDataAdapterXml.h"
#include "cxVideoService.h"
#include "cxVideoConnectionManager.h"
#include "cxtestSignalListener.h"

namespace cxtest
{

TestVideoConnectionWidget::TestVideoConnectionWidget() :
		VideoConnectionWidget(NULL)
{
}

bool TestVideoConnectionWidget::canStream(QString filename)
{
	this->show();
	QTest::qWaitForWindowShown(this);

	this->setupWidgetToRunDummyMhdStreamer(filename);

	QTest::mouseClick(mConnectButton, Qt::LeftButton); //connect

	bool videoConnectionConnected = waitForSignal(this->getConnection().get(), SIGNAL(connected(bool)));
	bool activeVideoSourceChanged = waitForSignal(cx::videoService(), SIGNAL(activeVideoSourceChanged()));
	ssc::VideoSourcePtr stream = cx::videoService()->getActiveVideoSource();
	bool videoSourceReceivedNewFrame = waitForSignal(stream.get(), SIGNAL(newFrame()));
	bool canStream = stream->isStreaming();

	QTest::mouseClick(mConnectButton, Qt::LeftButton); //disconnect

	this->close();

	return canStream;
}

void TestVideoConnectionWidget::setupWidgetToRunDummyMhdStreamer(QString filename)
{
	QString connectionMethod("Direct Link");
	mConnectionSelector->setValue(connectionMethod);
	QString connectionArguments("--type MHDFile --filename " + filename);
	mDirectLinkArguments->addItem(connectionArguments);
	mDirectLinkArguments->setCurrentIndex(mDirectLinkArguments->findText(connectionArguments));
}

} /* namespace cxtest */
