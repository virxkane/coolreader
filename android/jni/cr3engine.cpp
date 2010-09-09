// CoolReader3 Engine JNI interface
// BASED on Android NDK Plasma example

#include <jni.h>
#include <time.h>


#include <stdio.h>
#include <stdlib.h>

#include "org_coolreader_crengine_Engine.h"
#include "org_coolreader_crengine_ReaderView.h"

#include "cr3java.h"
#include "crengine.h"



class JNICDRLogger : public CRLog
{
public:
    JNICDRLogger()
    {
    	curr_level = CRLog::LL_DEBUG;
    }
protected:
  
	virtual void log( const char * lvl, const char * msg, va_list args)
	{
	    #define MAX_LOG_MSG_SIZE 1024
		static char buffer[MAX_LOG_MSG_SIZE+1];
		vsnprintf(buffer, MAX_LOG_MSG_SIZE, msg, args);
		int level = ANDROID_LOG_DEBUG;
		//LOGD("CRLog::log is called with LEVEL %s, pattern %s", lvl, msg);
		if ( !strcmp(lvl, "FATAL") )
			level = ANDROID_LOG_FATAL;
		else if ( !strcmp(lvl, "ERROR") )
			level = ANDROID_LOG_ERROR;
		else if ( !strcmp(lvl, "WARN") )
			level = ANDROID_LOG_WARN;
		else if ( !strcmp(lvl, "INFO") )
			level = ANDROID_LOG_INFO;
		else if ( !strcmp(lvl, "DEBUG") )
			level = ANDROID_LOG_DEBUG;
		else if ( !strcmp(lvl, "TRACE") )
			level = ANDROID_LOG_VERBOSE;
		__android_log_write(level, LOG_TAG, buffer);
	}
};

/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    initInternal
 * Signature: ([Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_Engine_initInternal
  (JNIEnv * penv, jobject obj, jobjectArray fontArray)
{
	LOGI("initInternal called");
	CRJNIEnv env(penv);
	LOGD("Redirecting CDRLog to Android");
	CRLog::setLogger( new JNICDRLogger() );
	CRLog::setLogLevel( CRLog::LL_TRACE );
	CRLog::info("CREngine log redirected");
	CRLog::info("creating font manager");
	InitFontManager(lString8());
	CRLog::debug("converting fonts array: %d items", (int)env->GetArrayLength(fontArray));
	lString16Collection fonts;
	env.fromJavaStringArray(fontArray, fonts);
	int len = fonts.length();
	CRLog::debug("registering fonts: %d fonts in list", len);
	for ( int i=0; i<len; i++ ) {
		lString8 fontName = UnicodeToUtf8(fonts[i]);
		CRLog::debug("registering font %s", fontName.c_str());
		if ( !fontMan->RegisterFont( fontName ) )
			CRLog::error("cannot load font %s", fontName.c_str());
	}
	CRLog::info("%d fonts registered", (int)fontMan->GetFontCount());
	return fontMan->GetFontCount() ? JNI_TRUE : JNI_FALSE;
}

/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    uninitInternal
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_coolreader_crengine_Engine_uninitInternal
  (JNIEnv *, jobject)
{
	LOGI("uninitInternal called");
	HyphMan::uninit();
	ShutdownFontManager();
	CRLog::setLogger(NULL);
}

/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    getFontFaceListInternal
 * Signature: ()[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL Java_org_coolreader_crengine_Engine_getFontFaceListInternal
  (JNIEnv * penv, jobject obj)
{
	LOGI("getFontFaceListInternal called");
	CRJNIEnv env(penv);
	lString16Collection list;
	fontMan->getFaceList(list);
	return env.toJavaStringArray(list);
}

/*
 * Class:     org_coolreader_crengine_ReaderView
 * Method:    getPageImage
 * Signature: (Landroid/graphics/Bitmap;)V
 */
JNIEXPORT void JNICALL Java_org_coolreader_crengine_ReaderView_getPageImage
  (JNIEnv * env, jclass cls, jobject bitmap)
{
	BitmapAccessor bmp(env,bitmap);
	if ( bmp.isOk() ) {
		int dx = bmp.getWidth();
		int dy = bmp.getHeight();
		LVColorDrawBuf buf( dx, dy );
		buf.FillRect(0, 0, dx, dy, 0xFF8000);
		buf.FillRect(10, 10, dx-20, dy-20, 0xC0C0FF);
		buf.FillRect(20, 20, 30, 30, 0x0000FF);
		LVFontRef fnt = fontMan->GetFont(30, 400, false, css_ff_sans_serif, lString8("Droid Sans"));
		fnt->DrawTextString(&buf, 40, 40, L"Text 1", 6, '?', NULL, false, 0, 0);
		fnt->DrawTextString(&buf, 40, 90, L"Text 2", 6, '?', NULL, false, 0, 0);
		bmp.draw( &buf, 0, 0 );		
//		lUInt32 row[500];
//		for ( int y=0; y<300; y++ ) {
//			for ( int x=0; x<500; x++ ) {
//				row[x] = x*5 + y*30;
//			}
//			bmp.setRowRGB( 0, y, row, 500 );
//		}
	} else {
		CRLog::error("bitmap accessor is invalid");
	}
}



//=====================================================================

static JNINativeMethod sEngineMethods[] = {
  {"initInternal", "([Ljava/lang/String;)Z", (void*)Java_org_coolreader_crengine_Engine_initInternal},
  {"uninitInternal", "()V", (void*)Java_org_coolreader_crengine_Engine_uninitInternal},
  {"getFontFaceListInternal", "()[Ljava/lang/String", (void*)Java_org_coolreader_crengine_Engine_getFontFaceListInternal},
};


static JNINativeMethod sReaderViewMethods[] = {
  /* name, signature, funcPtr */
  {"getPageImage", "(Landroid/graphics/Bitmap;)V", (void*)Java_org_coolreader_crengine_ReaderView_getPageImage},
};
 
/*
 * Register native JNI-callable methods.
 *
 * "className" looks like "java/lang/String".
 */
static int jniRegisterNativeMethods(JNIEnv* env, const char* className,
    const JNINativeMethod* gMethods, int numMethods)
{
    jclass clazz;

    LOGV("Registering %s natives\n", className);
    clazz = env->FindClass(className);
    if (clazz == NULL) {
        LOGE("Native registration unable to find class '%s'\n", className);
        return -1;
    }
    if (env->RegisterNatives(clazz, gMethods, numMethods) < 0) {
        LOGE("RegisterNatives failed for '%s'\n", className);
        return -1;
    }
    return 0;
}


jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
   JNIEnv* env = NULL;
   jint result = -1;
 
   if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
      return result;
   }
 
   jniRegisterNativeMethods(env, "org/coolreader/crengine/Engine", sEngineMethods, 1);
   jniRegisterNativeMethods(env, "org/coolreader/crengine/ReaderView", sReaderViewMethods, 1);
   
   return JNI_VERSION_1_4;
}