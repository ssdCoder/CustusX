// This file is part of SSC,
// a C++ Library supporting Image Guided Therapy Applications.
//
// Copyright (C) 2012- SINTEF Medical Technology
// Copyright (C) 2012- Sonowand AS
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

/*
 * vmTextureVolumePainter.cpp
 *
 */

#include "sscTextureVolumePainter.h"

#ifndef WIN32
#include <QtCore>
#include <vtkOpenGLExtensionManager.h>
#include <vtkRenderer.h>
#include <vtkUniformVariables.h>
#include <vtkObjectFactory.h>
#include <vtkOpenGLRenderWindow.h>
#include <vtkOpenGLTexture.h>
#include <vtkgl.h>
#include <vtkPlane.h>
#include <vtkProperty.h>
#include <vtkMatrix4x4.h>
#include <vtkShaderProgram2.h>
#include <vtkShader2.h>
#include <vtkShader2Collection.h>

#ifdef __APPLE__
#include <OpenGL/glu.h>
#include "/usr/include/X11/Xlib.h"
#else
#include <GL/glu.h>
#endif

#include <Cg/cg.h>
#include <Cg/cgGL.h>


#ifdef WIN32
#include <windows.h>
#include <GL/glext.h>
#endif

#include "sscGPUImageBuffer.h"
#include "sscTypeConversions.h"
#include "sscGLHelpers.h"
#include "sscSlicePlaneClipper.h"


//---------------------------------------------------------
namespace ssc
{
//---------------------------------------------------------

vtkStandardNewMacro(TextureVolumePainter);
vtkCxxRevisionMacro(TextureVolumePainter, "$Revision: 647 $");

#define GL_TRACE(string) if (vtkgl::StringMarkerGREMEDY) {vtkgl::StringMarkerGREMEDY(0, QString("%1:%2 - %3").arg(__func__).arg(__LINE__).arg(string).toUtf8().constData());}
class HackGLTexture: public vtkOpenGLTexture
{
public:
	HackGLTexture() {}
	virtual ~HackGLTexture() {}
	virtual long GetIndex() {return mIndex;}
	virtual void SetIndex(long index) {mIndex = index;}
private:
	long mIndex;
};
	
class SingleVolumePainterHelper
{
	ssc::GPUImageDataBufferPtr mVolumeBuffer;
	ssc::GPUImageLutBufferPtr mLutBuffer;
	int mIndex;
	float mWindow;
	float mLevel;
	float mLLR;
	float mAlpha;
	HackGLTexture *mTexture;
	bool mTextureLoaded;
	Transform3D m_nMr;
	bool mClip;

public:
	explicit SingleVolumePainterHelper(int index)
	{
		mIndex = index;
		mTexture = new HackGLTexture();
		int texture = 2*mIndex;
		mTexture->SetIndex(texture);
		mTextureLoaded = false;
		mClip = false;
	}
	SingleVolumePainterHelper()
	{
		mIndex = -1;
		mTexture = NULL;
	}
	~SingleVolumePainterHelper()
	{
	}
	void SetBuffer(ssc::GPUImageDataBufferPtr buffer)
	{
		mVolumeBuffer = buffer;
	}
	void SetBuffer(ssc::GPUImageLutBufferPtr buffer)
	{
		mLutBuffer = buffer;
	}
	void SetColorAttribute(float window, float level, float llr,float alpha)
	{
		mWindow = window;
		mLevel = level;
		mLLR = llr;
		mAlpha = alpha;
	}
	void set_nMr( Transform3D nMr)
	{
		m_nMr = nMr;
	}
	void setClip( bool clip)
	{
		mClip = clip;
	}
	void initializeRendering()
	{
		if (mVolumeBuffer)
		{
			mVolumeBuffer->allocate();
		}
	}
	void eachPrepareRendering()
	{
		if (mLutBuffer)
		{
			mLutBuffer->allocate();
		}
		if (mVolumeBuffer)
		{
			mVolumeBuffer->updateTexture();
		}
	}
	void eachRenderInternal(vtkSmartPointer<vtkShaderProgram2> shader)
	{
		if (!mVolumeBuffer)
		{
			return;
		}

		mVolumeBuffer->bind(mIndex);

		int texture = 2*mIndex; //texture unit 1
		int lut = 2*mIndex+1; //texture unit 1
		int lutSize = 0;

		if (mLutBuffer)
		{
			mLutBuffer->bind(mIndex);
			lutSize = mLutBuffer->getLutSize();
		}
		shader->GetUniformVariables()->SetUniformi(cstring_cast(QString("lutSize[%1]").arg(mIndex)), 1, &lutSize);
		shader->GetUniformVariables()->SetUniformi(cstring_cast(QString("lut[%1]").arg(mIndex)), 1, &lut);
		shader->GetUniformVariables()->SetUniformi(cstring_cast(QString("volumeTexture[%1]").arg(mIndex)), 1, &texture);
		shader->GetUniformVariables()->SetUniformf(cstring_cast(QString("window[%1]").arg(mIndex)), 1, &mWindow);
		shader->GetUniformVariables()->SetUniformf(cstring_cast(QString("level[%1]").arg(mIndex)), 1, &mLevel);
		shader->GetUniformVariables()->SetUniformf(cstring_cast(QString("threshold[%1]").arg(mIndex)), 1, &mLLR);
		shader->GetUniformVariables()->SetUniformf(cstring_cast(QString("transparency[%1]").arg(mIndex)), 1, &mAlpha);
		int clip = mClip;
		shader->GetUniformVariables()->SetUniformi(cstring_cast(QString("useCutPlane[%1]").arg(mIndex)), 1, &clip);
		vtkMatrix4x4Ptr M = m_nMr.getVtkMatrix();
		float matrix[16];
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				matrix[i*4+j] = M->GetElement(j, i);
			}
		}
		shader->GetUniformVariables()->SetUniformMatrix(cstring_cast(QString("M[%1]").arg(mIndex)), 4, 4, matrix);
		report_gl_error();
	}
};

class TextureVolumePainter::vtkInternals
{
public:
	Display* mCurrentContext;

	vtkWeakPointer<vtkRenderWindow> LastContext;

	vtkSmartPointer<vtkShaderProgram2> Shader;

	unsigned int mVolumes;

	std::vector<SingleVolumePainterHelper> mElement;

	SingleVolumePainterHelper& safeIndex(int index)
	{
		if ((int)mElement.size() <= index)
		{
			mElement.resize(index+1);
			mElement[index] = SingleVolumePainterHelper(index);
		}
		return mElement[index];
	}

	vtkInternals()
	{

	}
	~vtkInternals()
	{
		mElement.clear();
	}
	void ClearGraphicsResources()
	{
		if (this->Shader != 0)
		{
			this->Shader->ReleaseGraphicsResources();
			this->Shader = 0;
		}
	}
};

//---------------------------------------------------------
TextureVolumePainter::TextureVolumePainter() :
	mDepthBuffer(0),
	mBackgroundBuffer(0),
	mBuffersValid(false),
	mRenderMode(0)
{
	mInternals = new vtkInternals();
}

void TextureVolumePainter::setShaderFiles(QString vertexShaderFile, QString fragmentShaderFile)
{
	mVertexShaderFile = vertexShaderFile;
	mFragmentShaderFile = fragmentShaderFile;
}

QString TextureVolumePainter::loadShaderFile(QString shaderFile)
{
	QFile fp(shaderFile);
	if (fp.exists())
	{
		fp.open(QFile::ReadOnly);
		QTextStream shaderfile(&fp);
		return shaderfile.readAll();
	}
	else
	{
		std::cout << "TextureSlicer can't read shaderfile [" << fp.fileName() << "]" << std::endl;
	}
	return "";
}

TextureVolumePainter::~TextureVolumePainter()
{
	if (mInternals->LastContext)
	{
	    this->ReleaseGraphicsResources(this->LastWindow);
	}

	delete mInternals;
	mInternals = 0;
}

void TextureVolumePainter::ReleaseGraphicsResources(vtkWindow* win)
{
	mInternals->ClearGraphicsResources(); //the shader
	mInternals->LastContext = 0;
	if (mDepthBuffer)
	{
		glDeleteTextures(1, &mDepthBuffer);
		mDepthBuffer = 0;
	}
	if (mBackgroundBuffer)
	{
		glDeleteTextures(1, &mBackgroundBuffer);
		mBackgroundBuffer = 0;
	}
	this->Superclass::ReleaseGraphicsResources(win);
}

void TextureVolumePainter::PrepareForRendering(vtkRenderer* renderer, vtkActor* actor)
{
	if (!CanRender(renderer, actor))
	{
		mInternals->ClearGraphicsResources();
		mInternals->LastContext = 0;
		this->Superclass::PrepareForRendering(renderer, actor);
		return;
	}

	vtkRenderWindow* renWin = renderer->GetRenderWindow();
	if (mInternals->LastContext != renWin || mInternals->mElement.size() != mInternals->mVolumes)
	{
		mInternals->ClearGraphicsResources();
	}
	mInternals->LastContext = renWin;

	report_gl_error();

	vtkOpenGLRenderWindow *context = vtkOpenGLRenderWindow::SafeDownCast(renWin); 
	if (!LoadRequiredExtensions(context->GetExtensionManager()))
	{
		std::cout << "Missing required EXTENSION!!!!!!!!!!!." << endl;
		return;
	}

	if (!mInternals->Shader)
	{
		mInternals->mVolumes = mInternals->mElement.size();
		QString vertexShaderSource = this->loadShaderFile(mVertexShaderFile);
		QString fragmentShaderSource = this->loadShaderFile(mFragmentShaderFile);
		fragmentShaderSource = fragmentShaderSource.replace("${NUMBER_OF_VOLUMES}", QString("%1").arg(mInternals->mElement.size()));

		vtkShaderProgram2Ptr pgm = vtkShaderProgram2Ptr::New();
		pgm->SetContext(static_cast<vtkOpenGLRenderWindow *> (renWin));

		vtkShader2Ptr s1 = vtkShader2Ptr::New();
		s1->SetType(VTK_SHADER_TYPE_VERTEX);
		s1->SetSourceCode(cstring_cast(vertexShaderSource));
		s1->SetContext(pgm->GetContext());
		pgm->GetShaders()->AddItem(s1);

		vtkShader2Ptr s2 = vtkShader2Ptr::New();
		s2->SetType(VTK_SHADER_TYPE_FRAGMENT);
		s2->SetSourceCode(cstring_cast(fragmentShaderSource));
		s2->SetContext(pgm->GetContext());
		pgm->GetShaders()->AddItem(s2);
		mInternals->Shader = pgm;
		report_gl_error();
		for (unsigned i = 0; i < mInternals->mElement.size(); ++i)
		{
			mInternals->mElement[i].initializeRendering();
		}
	}

	mInternals->Shader->GetUniformVariables()->SetUniformi("renderMode", 1, &mRenderMode);
	mInternals->Shader->GetUniformVariables()->SetUniformf("stepsize", 1, &mStepSize);
	float viewport[2];
	viewport[0] = mWidth;
	viewport[1] = mHeight;
	mInternals->Shader->GetUniformVariables()->SetUniformf("viewport", 2, viewport);
	
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ROW_LENGTH, 0);
	glPixelStorei(GL_PACK_SKIP_ROWS, 0);
	glPixelStorei(GL_PACK_SKIP_PIXELS, 0);

	report_gl_error();
	for (unsigned i = 0; i < mInternals->mElement.size(); ++i)
	{
		mInternals->mElement[i].eachPrepareRendering();
	}
	mInternals->Shader->Use();
	report_gl_error();

	if (!mInternals->Shader->IsValid())
	{
		vtkErrorMacro(<<" validation of the program failed: "<< mInternals->Shader->GetLastValidateLog());
	}

	this->Superclass::PrepareForRendering(renderer, actor);
	report_gl_error();

	mInternals->Shader->Restore();
	
	glDisable(vtkgl::TEXTURE_3D);
	glBindTexture(GL_TEXTURE_3D, 0);
	report_gl_error();
}

void TextureVolumePainter::RenderInternal(vtkRenderer* renderer, vtkActor* actor, unsigned long typeflags,
		bool forceCompileOnly)
{
	report_gl_error();

	if (!CanRender(renderer, actor))
	{
		return;
	}
	GL_TRACE("Entering");
	// Save context state to be able to restore.
	mInternals->Shader->Build();
	if (mInternals->Shader->GetLastBuildStatus() != VTK_SHADER_PROGRAM2_LINK_SUCCEEDED)
	{
		vtkErrorMacro("Pass Two failed.");
		abort();
	}

	for (unsigned i = 0; i < mInternals->mElement.size(); ++i)
	{
		mInternals->mElement[i].eachRenderInternal(mInternals->Shader);
	}

	if (mClipper)
	{
		float vector[3];
		double dvector[3];
		mClipper->getClipPlaneCopy()->GetNormal(dvector);
		vector[0] = dvector[0];
		vector[1] = dvector[1];
		vector[2] = dvector[2];
		mInternals->Shader->GetUniformVariables()->SetUniformfv("cutPlaneNormal", 3, 1, vector);
		mClipper->getClipPlaneCopy()->GetOrigin(dvector);
		vector[0] = dvector[0];
		vector[1] = dvector[1];
		vector[2] = dvector[2];
		mInternals->Shader->GetUniformVariables()->SetUniformfv("cutPlaneOffset", 3, 1, vector);
	}
	
	glDepthMask(1);
	if (!mBuffersValid)
	{
		if (glIsTexture(mDepthBuffer))
		{
			glDeleteTextures(1, &mDepthBuffer);
			mDepthBuffer = 0;
		}
		if (glIsTexture(mBackgroundBuffer))
		{
			glDeleteTextures(1, &mBackgroundBuffer);
			mBackgroundBuffer = 0;
		}
	}
	if (!glIsTexture(mDepthBuffer))
	{
		glGenTextures(1, &mDepthBuffer);
		report_gl_error();
		glActiveTexture(GL_TEXTURE10);
		glBindTexture(vtkgl::TEXTURE_RECTANGLE_ARB, mDepthBuffer);
		report_gl_error();
		glTexImage2D(vtkgl::TEXTURE_RECTANGLE_ARB, 0, vtkgl::DEPTH_COMPONENT32, mWidth, mHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
		report_gl_error();
		glTexParameteri(vtkgl::TEXTURE_RECTANGLE_ARB, vtkgl::TEXTURE_COMPARE_MODE, GL_NONE);
		glTexParameteri(vtkgl::TEXTURE_RECTANGLE_ARB, vtkgl::DEPTH_TEXTURE_MODE, GL_LUMINANCE);
		glTexParameteri(vtkgl::TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(vtkgl::TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
	if (!glIsTexture(mBackgroundBuffer))
	{
		glGenTextures(1, &mBackgroundBuffer);
		report_gl_error();
		glActiveTexture(GL_TEXTURE11);
		glBindTexture(vtkgl::TEXTURE_RECTANGLE_ARB, mBackgroundBuffer);
		report_gl_error();
		glTexImage2D(vtkgl::TEXTURE_RECTANGLE_ARB, 0, GL_RGB, mWidth, mHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		report_gl_error();
		glTexParameteri(vtkgl::TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(vtkgl::TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}
	mBuffersValid = true;

	glActiveTexture(GL_TEXTURE10);
	glBindTexture(vtkgl::TEXTURE_RECTANGLE_ARB, mDepthBuffer);
	glCopyTexSubImage2D( vtkgl::TEXTURE_RECTANGLE_ARB, 0, 0, 0, 0, 0, mWidth, mHeight);
	glActiveTexture(GL_TEXTURE11);
	glBindTexture(vtkgl::TEXTURE_RECTANGLE_ARB, mBackgroundBuffer);
	glCopyTexSubImage2D( vtkgl::TEXTURE_RECTANGLE_ARB, 0, 0, 0, 0, 0, mWidth, mHeight);
	report_gl_error();
	int depthTexture = 10;
	mInternals->Shader->GetUniformVariables()->SetUniformi("depthBuffer", 1, (int*)&depthTexture);
	int backgroundTexture = 11;
	mInternals->Shader->GetUniformVariables()->SetUniformi("backgroundBuffer", 1, (int*)&backgroundTexture);
	mInternals->Shader->Use();
	report_gl_error();

	if (!mInternals->Shader->IsValid())
	{
		vtkErrorMacro(<<" validation of the program failed: "<< mInternals->Shader->GetLastValidateLog());
	}

	this->Superclass::RenderInternal(renderer, actor, typeflags, forceCompileOnly);

	mInternals->Shader->Restore();

	glDisable(vtkgl::TEXTURE_3D);
	glBindTexture(GL_TEXTURE_3D, 0);
	report_gl_error();
}

bool TextureVolumePainter::CanRender(vtkRenderer*, vtkActor*)
{
	return !mInternals->mElement.empty();
}

bool TextureVolumePainter::LoadRequiredExtension(vtkOpenGLExtensionManager* mgr, QString id)
{
	bool loaded = mgr->LoadSupportedExtension(cstring_cast(id));
	if (!loaded)
		std::cout << "TextureVolumePainter Error: GL extension " + id + " not found" << std::endl;
	return loaded;
}

bool TextureVolumePainter::LoadRequiredExtensions(vtkOpenGLExtensionManager* mgr)
{
	GLint value[2];
	glGetIntegerv(vtkgl::MAX_TEXTURE_COORDS,value);
	if( value[0] < 8)
	{
		std::cout<<"GL_MAX_TEXTURE_COORDS="<<value[0]<<" . Number of texture coordinate sets. Min is 2."<<std::endl;
	}
	mgr->LoadSupportedExtension("GL_GREMEDY_string_marker");
	return (LoadRequiredExtension(mgr, "GL_VERSION_2_0")
			&& LoadRequiredExtension(mgr, "GL_VERSION_1_5")
			&& LoadRequiredExtension(mgr, "GL_VERSION_1_3")
	        && LoadRequiredExtension(mgr, "GL_ARB_texture_rectangle")
			&& LoadRequiredExtension(mgr, "GL_ARB_vertex_buffer_object")
			&& LoadRequiredExtension(mgr, "GL_EXT_texture_buffer_object"));
}

void TextureVolumePainter::SetVolumeBuffer(int index, ssc::GPUImageDataBufferPtr buffer)
{
	mInternals->safeIndex(index).SetBuffer(buffer);
}

void TextureVolumePainter::SetLutBuffer(int index, ssc::GPUImageLutBufferPtr buffer)
{
	mInternals->safeIndex(index).SetBuffer(buffer);
}

void TextureVolumePainter::SetColorAttribute(int index, float window, float level, float llr,float alpha)
{
	mInternals->safeIndex(index).SetColorAttribute(window, level, llr, alpha);
}

void TextureVolumePainter::setClipVolume(int index, bool clip)
{
	std::cout << "Turning cutplane " << (clip ? "on" : "off") << " for volume " << index << std::endl;
	mInternals->safeIndex(index).setClip(clip);
}

void TextureVolumePainter::PrintSelf(ostream& os, vtkIndent indent)
{
}

void TextureVolumePainter::setViewport(float width, float height)
{
	if (width != mWidth || height != mHeight)
	{
		mBuffersValid = false;
	}
	mWidth = width;
	mHeight = height;	
}

void TextureVolumePainter::set_nMr(int index, Transform3D nMr)
{
	mInternals->safeIndex(index).set_nMr(nMr);
}

void TextureVolumePainter::setClipper(SlicePlaneClipperPtr clipper)
{
	mClipper = clipper;
}
void TextureVolumePainter::setStepSize(double stepsize)
{
	mStepSize = stepsize;
}
void TextureVolumePainter::setRenderMode(int renderMode)
{
	mRenderMode = renderMode;
}
//---------------------------------------------------------
}//end namespace
//---------------------------------------------------------
#endif //WIN32
