#ifndef CXVLCPROCESS_H_
#define CXVLCPROCESS_H_

#include <QString>
#include "cxProcessWrapper.h"

namespace cx
{
typedef boost::shared_ptr<class VLCRecorder> VLCRecorderPtr;

/**
 * \brief Lets you use the third party application VLC
 * to record a video of the screen.
 *
 * \date Oct 10, 2013
 * \author Janne Beate Bakeng, SINTEF
 */

class VLCRecorder: public QObject
{
	Q_OBJECT

public:
	static VLCRecorder* getInstance(); ///< Returns a reference to the only MessageManager that exists.

	bool hasVLCApplication();
	void findVLCApplication(QStringList searchPaths = QStringList());

	bool isRecording();

	bool waitForStarted(int msecs = 30000);
	bool waitForFinished(int msecs = 30000);

	QString getVLCPath();

public slots:
	void startRecording(QString saveFile);
	void stopRecording();

	void play(QString moviePath);

private:
	VLCRecorder();
	virtual ~VLCRecorder();
	VLCRecorder(const VLCRecorder&);
	VLCRecorder& operator=(const VLCRecorder&);

	void setVLCPath(QString path);
	bool isValidVLC(QString vlcPath);
	QString getVLCDefaultLocation();
	QString getVLCDefaultRecorderArguments(QString saveFile);

	ProcessWrapperPtr mCommandLine;
	QString mVLCPath;

	static VLCRecorder* mTheInstance; ///< The unique VLCRecorder
};

/**Shortcut for accessing the vlc recorder
 */
VLCRecorder* vlc();


} /* namespace cx */

#endif /* CXVLCPROCESS_H_ */