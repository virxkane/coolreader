// CoolReader3 Engine JNI interface
// BASED on Android NDK Plasma example

#include <jni.h>
#include <time.h>
#include <android/log.h>
#include <android/api-level.h>


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "org_coolreader_crengine_Engine.h"
#include "org_coolreader_crengine_DocView.h"

#include "cr3java.h"
#include "../../crengine/include/cr3version.h"
#include "docview.h"
#include "../../crengine/include/crengine.h"
#include "../../crengine/include/epubfmt.h"
#include "../../crengine/include/pdbfmt.h"
#include "../../crengine/include/lvopc.h"
#include "../../crengine/include/fb3fmt.h"
#include "../../crengine/include/docxfmt.h"
#include "../../crengine/include/odtfmt.h"
#include "../../crengine/include/lvstream.h"


#include <../../crengine/include/fb2def.h>

#include "fc-lang-cat.h"

#define XS_IMPLEMENT_SCHEME 1
#include <../../crengine/include/fb2def.h>
#include <sys/stat.h>

#if defined(__arm__) || defined(__aarch64__) || defined(__i386__) || defined(__mips__)
#define USE_COFFEECATCH 1
#endif


#if USE_COFFEECATCH == 1
#include "coffeecatch/coffeecatch.h"
#include "coffeecatch/coffeejni.h"
#else
#define COFFEE_TRY_JNI(ENV, CODE) CODE;
#endif

#ifdef _DEBUG
// missing in system ZLIB with DEBUG option turned off
int z_verbose=0;
extern "C" void z_error(char * msg);
void z_error(char * msg) {
	fprintf(stderr, "%s\n", msg);
	exit(1);
}
#endif
/// returns current time representation string
static lString16 getDateTimeString( time_t t )
{
    tm * bt = localtime(&t);
    char str[32];
#ifdef _LINUX
    snprintf(str, 32,
#else
    sprintf(str, 
#endif
        "%04d/%02d/%02d %02d:%02d", bt->tm_year+1900, bt->tm_mon+1, bt->tm_mday, bt->tm_hour, bt->tm_min);
    str[31] = 0;
    return Utf8ToUnicode( lString8( str ) );
}

#if 0
static lString16 extractDocSeriesReverse( ldomDocument * doc, int & seriesNumber )
{
	seriesNumber = 0;
    lString16 res;
    ldomXPointer p = doc->createXPointer(L"/FictionBook/description/title-info/sequence");
    if ( p.isNull() )
        return res;
    ldomNode * series = p.getNode();
    if ( series ) {
        lString16 sname = series->getAttributeValue( attr_name );
        lString16 snumber = series->getAttributeValue( attr_number );
        if ( !sname.empty() ) {
            res << L"(";
            if ( !snumber.empty() ) {
                res << L"#" << snumber << L" ";
                seriesNumber = snumber.atoi();
            }
            res << sname;
            res << L")";
        }
    }
    return res;
}
#endif

class BookProperties
{
public:
    lString16 filename;
    lString16 title;
    lString16 author;
    lString16 series;
    int filesize;
    lString16 filedate;
    int seriesNumber;
    lString16 language;
    lUInt32 crc32;
    lString16 description;
};

static bool GetEPUBBookProperties(const char *name, LVStreamRef stream, BookProperties * pBookProps)
{
    LVContainerRef arc = LVOpenArchieve(stream );
    if ( arc.isNull() )
        return false; // not a ZIP archive

    // check root media type
    lString16 rootfilePath = EpubGetRootFilePath(arc);
    if ( rootfilePath.empty() )
    	return false;

    lString16 codeBase;
    codeBase=LVExtractPath(rootfilePath, false);

    LVStreamRef content_stream = arc->OpenStream(rootfilePath.c_str(), LVOM_READ);
    if ( content_stream.isNull() )
        return false;

    ldomDocument * doc = LVParseXMLStream( content_stream );
    if ( !doc )
        return false;

    time_t t = (time_t)time(0);
    struct stat fs;
    if ( !stat( name, &fs ) ) {
        t = fs.st_mtime;
    }

    lString16 author = doc->textFromXPath( lString16("package/metadata/creator")).trim();
    lString16 title = doc->textFromXPath( lString16("package/metadata/title")).trim();
    lString16 language = doc->textFromXPath( lString16("package/metadata/language")).trim();
    lString16 description = doc->textFromXPath( cs16("package/metadata/description")).trim();

    pBookProps->author = author;
    pBookProps->title = title;
    pBookProps->language = language;
    pBookProps->description = description;

    for ( int i=1; i<20; i++ ) {
        ldomNode * item = doc->nodeFromXPath( lString16("package/metadata/meta[") << fmt::decimal(i) << "]" );
        if ( !item )
            break;
        lString16 name = item->getAttributeValue("name");
        lString16 content = item->getAttributeValue("content");
        if (name == "calibre:series")
        	pBookProps->series = content.trim();
        else if (name == "calibre:series_index")
        	pBookProps->seriesNumber = content.trim().atoi();
    }

    pBookProps->filesize = (long)stream->GetSize();
    pBookProps->filename = lString16(name);
    pBookProps->filedate = getDateTimeString( t );
    pBookProps->crc32 = stream->getcrc32();

    delete doc;

    return true;
}

static bool GetFB3BookProperties(const char *name, LVStreamRef stream, BookProperties * pBookProps)
{
	LVContainerRef arc = LVOpenArchieve( stream );
	if ( arc.isNull() )
		return false; // not a ZIP archive

	OpcPackage package(arc);

	fb3ImportContext context(&package);

	CRPropRef doc_props = LVCreatePropsContainer();
	package.readCoreProperties(doc_props);
	pBookProps->title = doc_props->getStringDef(DOC_PROP_TITLE, "");
	pBookProps->author = doc_props->getStringDef(DOC_PROP_AUTHORS, "");
	pBookProps->description = doc_props->getStringDef(DOC_PROP_DESCRIPTION, "");

	ldomDocument * descDoc = context.getDescription();
	if ( descDoc ) {
		pBookProps->language = descDoc->textFromXPath( cs16("fb3-description/lang") );
	} else {
		CRLog::error("Couldn't parse description doc");
	}

	time_t t = (time_t)time(0);
	struct stat fs;
	if ( !stat( name, &fs ) ) {
		t = fs.st_mtime;
	}
	pBookProps->filesize = (long)stream->GetSize();
	pBookProps->filename = lString16(name);
	pBookProps->filedate = getDateTimeString( t );
	pBookProps->crc32 = stream->getcrc32();
	return true;
}

static bool GetDOCXBookProperties(const char *name, LVStreamRef stream, BookProperties * pBookProps)
{
	LVContainerRef arc = LVOpenArchieve( stream );
	if ( arc.isNull() )
		return false; // not a ZIP archive

	OpcPackage package(arc);

	CRPropRef doc_props = LVCreatePropsContainer();
	package.readCoreProperties(doc_props);
	pBookProps->title = doc_props->getStringDef(DOC_PROP_TITLE, "");
	pBookProps->author = doc_props->getStringDef(DOC_PROP_AUTHORS, "");
	pBookProps->description = doc_props->getStringDef(DOC_PROP_DESCRIPTION, "");
	pBookProps->language = doc_props->getStringDef(DOC_PROP_LANGUAGE, "");

	time_t t = (time_t)time(0);
	struct stat fs;
	if ( !stat( name, &fs ) ) {
		t = fs.st_mtime;
	}
	pBookProps->filesize = (long)stream->GetSize();
	pBookProps->filename = lString16(name);
	pBookProps->filedate = getDateTimeString( t );
	pBookProps->crc32 = stream->getcrc32();

	return true;
}

static bool GetODTBookProperties(const char *name, LVStreamRef stream, BookProperties * pBookProps)
{
	LVContainerRef arc = LVOpenArchieve( stream );
	if ( arc.isNull() )
		return false; // not a ZIP archive

	OpcPackage package(arc);

	//Read document metadata
	LVStreamRef meta_stream = arc->OpenStream(L"meta.xml", LVOM_READ);
	if ( meta_stream.isNull() )
		return false;
	ldomDocument * metaDoc = LVParseXMLStream( meta_stream );
	if ( !metaDoc ) {
		CRLog::error("Couldn't parse document meta data");
		return false;
	} else {
		CRPropRef doc_props = LVCreatePropsContainer();

		lString16 author = metaDoc->textFromXPath( cs16("document-meta/meta/creator") );
		lString16 title = metaDoc->textFromXPath( cs16("document-meta/meta/title") );
		lString16 description = metaDoc->textFromXPath( cs16("document-meta/meta/description") );
		doc_props->setString(DOC_PROP_TITLE, title);
		doc_props->setString(DOC_PROP_AUTHORS, author );
		doc_props->setString(DOC_PROP_DESCRIPTION, description );
		delete metaDoc;
	}

	time_t t = (time_t)time(0);
	struct stat fs;
	if ( !stat( name, &fs ) ) {
		t = fs.st_mtime;
	}
	pBookProps->filesize = (long)stream->GetSize();
	pBookProps->filename = lString16(name);
	pBookProps->filedate = getDateTimeString( t );
	pBookProps->crc32 = stream->getcrc32();

	return true;
}

static bool GetBookProperties(const char *name,  BookProperties * pBookProps)
{
    CRLog::trace("GetBookProperties( %s )", name);

    // check archieve
    lString16 arcPathName;
    lString16 arcItemPathName;
    bool isArchiveFile = LVSplitArcName( lString16(name), arcPathName, arcItemPathName );

    // open stream
    LVStreamRef stream = LVOpenFileStream( (isArchiveFile ? arcPathName : Utf8ToUnicode(lString8(name))).c_str() , LVOM_READ);
    if (!stream) {
        CRLog::error("cannot open file %s", name);
        return false;
    }


    if ( DetectEpubFormat( stream ) ) {
        CRLog::trace("GetBookProperties() : epub format detected");
    	return GetEPUBBookProperties( name, stream, pBookProps );
    }
    if ( DetectFb3Format( stream ) ) {
        CRLog::trace("GetBookProperties() : fb3 format detected");
        return GetFB3BookProperties( name, stream, pBookProps );
    }
	if ( DetectDocXFormat( stream ) ) {
		CRLog::trace("GetBookProperties() : docx format detected");
		return GetDOCXBookProperties( name, stream, pBookProps );
	}
	if ( DetectOpenDocumentFormat( stream ) ) {
		CRLog::trace("GetBookProperties() : odt format detected");
		return GetODTBookProperties( name, stream, pBookProps );
	}

    time_t t = (time_t)time(0);

    if ( isArchiveFile ) {
        int arcsize = (int)stream->GetSize();
        LVContainerRef container = LVOpenArchieve(stream);
        if ( container.isNull() ) {
            CRLog::error( "Cannot read archive contents from %s", LCSTR(arcPathName) );
            return false;
        }
        stream = container->OpenStream(arcItemPathName.c_str(), LVOM_READ);
        if ( stream.isNull() ) {
            CRLog::error( "Cannot open archive file item stream %s", LCSTR(lString16(name)) );
            return false;
        }
    }
    struct stat fs;
    if ( !stat( name, &fs ) ) {
        t = fs.st_mtime;
    }

    // read document
#if COMPACT_DOM==1
    ldomDocument doc(stream, 0);
#else
    ldomDocument doc;
#endif
    ldomDocumentWriter writer(&doc, true);
    doc.setNodeTypes( fb2_elem_table );
    doc.setAttributeTypes( fb2_attr_table );
    doc.setNameSpaceTypes( fb2_ns_table );
    LVXMLParser parser( stream, &writer );
    CRLog::trace( "checking format..." );
    if ( !parser.CheckFormat() ) {
        return false;
    }
    CRLog::trace( "parsing..." );
    if ( !parser.Parse() ) {
        return false;
    }
    CRLog::trace( "parsed" );
    #if 0
        char ofname[512];
        sprintf(ofname, "%s.xml", name);
        CRLog::trace("    writing to file %s", ofname);
        LVStreamRef out = LVOpenFileStream(ofname, LVOM_WRITE);
        doc.saveToStream(out, "utf16");
    #endif
    lString16 authors = extractDocAuthors( &doc, lString16("|"), false );
    lString16 title = extractDocTitle( &doc );
    lString16 language = extractDocLanguage( &doc );
    lString16 series = extractDocSeries( &doc, &pBookProps->seriesNumber );
    lString16 description = extractDocDescription( &doc );
#if SERIES_IN_AUTHORS==1
    if ( !series.empty() )
        authors << "    " << series;
#endif
    pBookProps->title = title;
    pBookProps->author = authors;
    pBookProps->series = series;
    pBookProps->filesize = (long)stream->GetSize();
    pBookProps->filename = lString16(name);
    pBookProps->filedate = getDateTimeString( t );
    pBookProps->language = language;
    pBookProps->description = description;
    pBookProps->crc32 = stream->getcrc32();
    return true;
}


/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    scanBookPropertiesInternal
 * Signature: (Lorg/coolreader/crengine/FileInfo;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_Engine_scanBookPropertiesInternal
  (JNIEnv * _env, jclass _engine, jobject _fileInfo)
{
	CRJNIEnv env(_env);
	jclass objclass = env->GetObjectClass(_fileInfo);
	jfieldID fid = env->GetFieldID(objclass, "pathname", "Ljava/lang/String;");
	lString16 filename = env.fromJavaString( (jstring)env->GetObjectField(_fileInfo, fid) );
    fid = env->GetFieldID(objclass, "arcname", "Ljava/lang/String;");
    lString16 arcname = env.fromJavaString( (jstring)env->GetObjectField(_fileInfo, fid) );
	if ( filename.empty() )
		return JNI_FALSE;
	if ( !arcname.empty() )
       filename = arcname + "@/" + filename;

	BookProperties props;
	CRLog::debug("Looking for properties of file %s", LCSTR(filename));
	bool res = GetBookProperties(LCSTR(filename),  &props);
	if ( !res )
		return JNI_FALSE;
	#define SET_STR_FLD(fldname,src) \
	{ \
	    jfieldID fid = env->GetFieldID(objclass, fldname, "Ljava/lang/String;"); \
	    env->SetObjectField(_fileInfo,fid,env.toJavaString(src)); \
	}
	#define SET_INT_FLD(fldname,src) \
	{ \
	    jfieldID fid = env->GetFieldID(objclass, fldname, "I"); \
	    env->SetIntField(_fileInfo,fid,src); \
	}
	#define SET_LONG_FLD(fldname,src) \
	{ \
	    jfieldID fid = env->GetFieldID(objclass, fldname, "J"); \
	    env->SetLongField(_fileInfo,fid,src); \
	}
	SET_STR_FLD("title",props.title);
	SET_STR_FLD("authors",props.author);
	SET_STR_FLD("series",props.series);
	SET_INT_FLD("seriesNumber",props.seriesNumber);
	SET_STR_FLD("language",props.language);
	SET_LONG_FLD("crc32",props.crc32);
	SET_STR_FLD("description",props.description);

	return JNI_TRUE;
}

/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    updateFileCRC32Internal
 * Signature: (Lorg/coolreader/crengine/FileInfo;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_Engine_updateFileCRC32Internal
		(JNIEnv * _env, jclass _engine, jobject _fileInfo)
{
	CRJNIEnv env(_env);
	jclass objclass = env->GetObjectClass(_fileInfo);
	jfieldID fid = env->GetFieldID(objclass, "pathname", "Ljava/lang/String;");
	lString16 filename = env.fromJavaString( (jstring)env->GetObjectField(_fileInfo, fid) );
	fid = env->GetFieldID(objclass, "arcname", "Ljava/lang/String;");
	lString16 arcname = env.fromJavaString( (jstring)env->GetObjectField(_fileInfo, fid) );
	if ( filename.empty() )
		return JNI_FALSE;
	bool isArchiveFile = !arcname.empty();
	// open stream
	LVStreamRef stream = LVOpenFileStream( (isArchiveFile ? arcname : filename).c_str() , LVOM_READ );
	if (!stream.isNull()) {
		if (isArchiveFile) {
			LVContainerRef container = LVOpenArchieve(stream);
			if (!container.isNull()) {
				stream = container->OpenStream(filename.c_str(), LVOM_READ);
				if (stream.isNull()) {
					CRLog::error("Cannot open archive file item stream %s", LCSTR(filename));
				}
			} else {
				CRLog::error("Cannot read archive contents from %s", LCSTR(arcname));
				stream = LVStreamRef();
			}
		}
	}
	if (!stream.isNull()) {
		fid = env->GetFieldID(objclass, "crc32", "J");
	    env->SetLongField(_fileInfo, fid, stream->getcrc32());
	} else {
		CRLog::error("cannot open file %s", LCSTR(isArchiveFile ? arcname : filename));
		return JNI_FALSE;
	}
	return JNI_TRUE;
}


void drawBookCoverInternal(JNIEnv * _env, jclass _engine, jobject bitmap, jbyteArray _data, jstring _fontFace, jstring _title, jstring _authors, jstring _seriesName, jint seriesNumber, jint bpp)
{
	CRJNIEnv env(_env);
	CRLog::debug("drawBookCoverInternal called");
	lString8 fontFace = UnicodeToUtf8(env.fromJavaString(_fontFace));
	lString16 title = env.fromJavaString(_title);
	lString16 authors = env.fromJavaString(_authors);
	lString16 seriesName = env.fromJavaString(_seriesName);
	LVStreamRef stream;
	LVDrawBuf * drawbuf = BitmapAccessorInterface::getInstance()->lock(_env, bitmap);
	if (drawbuf != NULL) {
		LVImageSourceRef image;
		if (_data != NULL && _env->GetArrayLength(_data) > 0) {
			CRLog::debug("drawBookCoverInternal : cover image from array");
			stream = env.jbyteArrayToStream(_data);
			if (!stream.isNull())
				image = LVCreateStreamImageSource(stream);
		}

		int factor = 1;
		int dx = drawbuf->GetWidth();
		int dy = drawbuf->GetHeight();
		int MIN_WIDTH = 300;
		int MIN_HEIGHT = 400;
		if (dx < MIN_WIDTH || dy < MIN_HEIGHT) {
			if (dx * 2 < MIN_WIDTH || dy * 2 < MIN_HEIGHT) {
				dx *= 3;
				dy *= 3;
				factor = 3;
			} else {
				dx *= 2;
				dy *= 2;
				factor = 2;
			}
		}
		LVDrawBuf * drawbuf2 = drawbuf;
		if (factor > 1)
			drawbuf2 = new LVColorDrawBuf(dx, dy, drawbuf->GetBitsPerPixel());

		if (bpp >= 16) {
			// native color resolution
			CRLog::debug("drawBookCoverInternal : calling LVDrawBookCover");
			LVDrawBookCover(*drawbuf2, image, fontFace, title, authors, seriesName, seriesNumber);
			image.Clear();
		} else {
			LVGrayDrawBuf grayBuf(drawbuf2->GetWidth(), drawbuf2->GetHeight(), bpp);
			LVDrawBookCover(grayBuf, image, fontFace, title, authors, seriesName, seriesNumber);
			image.Clear();
			grayBuf.DrawTo(drawbuf2, 0, 0, 0, NULL);
		}

		if (factor > 1) {
			CRLog::debug("drawBookCoverInternal : rescaling");
			drawbuf->DrawRescaled(drawbuf2, 0, 0, drawbuf->GetWidth(), drawbuf->GetHeight(), 0);
			delete drawbuf2;
		}

		//CRLog::trace("getPageImageInternal calling bitmap->unlock");
		BitmapAccessorInterface::getInstance()->unlock(_env, bitmap, drawbuf);
	} else {
		CRLog::error("bitmap accessor is invalid");
	}
	CRLog::debug("drawBookCoverInternal finished");
}

/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    drawBookCoverInternal
 * Signature: (Landroid/graphics/Bitmap;[BLjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;II)V
 */
JNIEXPORT void JNICALL Java_org_coolreader_crengine_Engine_drawBookCoverInternal
  (JNIEnv * _env, jclass _engine, jobject bitmap, jbyteArray _data, jstring _fontFace, jstring _title, jstring _authors, jstring _seriesName, jint seriesNumber, jint bpp)
{
	COFFEE_TRY_JNI(_env, drawBookCoverInternal(_env, _engine, bitmap, _data, _fontFace, _title, _authors, _seriesName, seriesNumber, bpp));
}

jbyteArray scanBookCoverInternal
  (JNIEnv * _env, jclass _class, jstring _path)
{
	CRJNIEnv env(_env);
	lString16 path = env.fromJavaString(_path);
	CRLog::debug("scanBookCoverInternal(%s) called", LCSTR(path));
	lString16 arcname, item;
    LVStreamRef res;
    jbyteArray array = NULL;
    LVContainerRef arc;
	if (!LVSplitArcName(path, arcname, item)) {
		// not in archive
		LVStreamRef stream = LVOpenFileStream(path.c_str(), LVOM_READ);
		if (!stream.isNull()) {
			arc = LVOpenArchieve(stream);
			if (!arc.isNull()) {
				// ZIP-based format
				if (DetectEpubFormat(stream)) {
					// EPUB
					// extract coverpage from epub
					res = GetEpubCoverpage(arc);
				}
			} else {
				res = GetFB2Coverpage(stream);
				if (res.isNull()) {
					doc_format_t fmt;
					if (DetectPDBFormat(stream, fmt)) {
						res = GetPDBCoverpage(stream);
					}
				}
			}
		}
	} else {
    	CRLog::debug("scanBookCoverInternal() : is archive, item=%s, arc=%s", LCSTR(item), LCSTR(arcname));
		LVStreamRef arcstream = LVOpenFileStream(arcname.c_str(), LVOM_READ);
		if (!arcstream.isNull()) {
			arc = LVOpenArchieve(arcstream);
			if (!arc.isNull()) {
				LVStreamRef stream = arc->OpenStream(item.c_str(), LVOM_READ);
				if (!stream.isNull()) {
			    	CRLog::debug("scanBookCoverInternal() : archive stream opened ok, parsing");
					res = GetFB2Coverpage(stream);
					if (res.isNull()) {
						doc_format_t fmt;
						if (DetectPDBFormat(stream, fmt)) {
							res = GetPDBCoverpage(stream);
						}
					}
				}
			}
		}
	}
	if (!res.isNull())
		array = env.streamToJByteArray(res);
    if (array != NULL)
    	CRLog::debug("scanBookCoverInternal() : returned cover page array");
    else
    	CRLog::debug("scanBookCoverInternal() : cover page data not found");
    return array;
}

/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    scanBookCoverInternal
 * Signature: (Ljava/lang/String;)[B
 */
JNIEXPORT jbyteArray JNICALL Java_org_coolreader_crengine_Engine_scanBookCoverInternal
  (JNIEnv * _env, jclass _class, jstring _path)
{
	jbyteArray res = NULL;
	COFFEE_TRY_JNI(_env, res = scanBookCoverInternal( _env, _class, _path));
	return res;
}

/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    getArchiveItemsInternal
 * Signature: (Ljava/lang/String;)[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL Java_org_coolreader_crengine_Engine_getArchiveItemsInternal
  (JNIEnv * _env, jclass, jstring jarcName)
{
    CRJNIEnv env(_env);
    lString16 arcName = env.fromJavaString(jarcName);
    lString16Collection list;
    
    //fontMan->getFaceList(list);
    LVStreamRef stream = LVOpenFileStream( arcName.c_str(), LVOM_READ );
    if ( !stream.isNull() ) {
        LVContainerRef arc = LVOpenArchieve(stream);
        if ( !arc.isNull() ) {
            // convert
            for ( int i=0; i<arc->GetObjectCount(); i++ ) {
                const LVContainerItemInfo * item = arc->GetObjectInfo(i);
                if ( item->IsContainer())
                    continue;
                list.add( item->GetName() );
                list.add( lString16::itoa(item->GetSize()) );
            }
        }
    }
    return env.toJavaStringArray(list);
}


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

//typedef void (lv_FatalErrorHandler_t)(int errorCode, const char * errorText );

void cr3androidFatalErrorHandler(int errorCode, const char * errorText )
{
	LOGE("CoolReader Fatal Error #%d: %s", errorCode, errorText);
	LOGASSERTFAILED("CoolReader Fatal Error", "CoolReader Fatal Error #%d: %s", errorCode, errorText);
	//static char str[1001];
	//snprintf(str, 1000, "CoolReader Fatal Error #%d: %s", errorCode, errorText);
	//LOGE("CoolReader Fatal Error #%d: %s", errorCode, errorText);
	//LOGASSERTFAILED(errorText, "CoolReader Fatal Error #%d: %s", errorCode, errorText);
}

/// set fatal error handler
void crSetFatalErrorHandler( lv_FatalErrorHandler_t * handler );

jboolean initInternal(JNIEnv * penv, jclass obj, jobjectArray fontArray, jint sdk_int) {

	CRJNIEnv::sdk_int = sdk_int;

	CRJNIEnv env(penv);

	// to catch crashes and remove current cache file on crash (SIGSEGV etc.)
	crSetSignalHandler();

	LOGI("initInternal called");
	// set fatal error handler
	crSetFatalErrorHandler( &cr3androidFatalErrorHandler );
	LOGD("Redirecting CDRLog to Android");
	CRLog::setLogger( new JNICDRLogger() );
	CRLog::setLogLevel( CRLog::LL_TRACE );
	CRLog::info("CREngine log redirected");
	CRLog::info("CRENGINE version %s %s", CR_ENGINE_VERSION, CR_ENGINE_BUILD_DATE);
	
	CRLog::info("initializing hyphenation manager");
    HyphMan::initDictionaries(lString16::empty_str); //don't look for dictionaries
	HyphMan::activateDictionary(lString16(HYPH_DICT_ID_NONE));
	CRLog::info("creating font manager");
    InitFontManager(lString8::empty_str);
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
    CRLog::info("%d fonts registered", fontMan->GetFontCount());
	return fontMan->GetFontCount() ? JNI_TRUE : JNI_FALSE;
}

/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    initInternal
 * Signature: ([Ljava/lang/String;I)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_Engine_initInternal
  (JNIEnv * penv, jclass obj, jobjectArray fontArray, jint sdk_int)
{
	jboolean res = JNI_FALSE;
	COFFEE_TRY_JNI(penv, res = initInternal(penv, obj, fontArray, sdk_int));
	return res;
}

class HyphDataLoaderProxy : public HyphDataLoader {
	JavaVM *mJavaVM;
public:
	HyphDataLoaderProxy(JavaVM *jvm) :
			HyphDataLoader(), mJavaVM(jvm) {
	}

	virtual ~HyphDataLoaderProxy() {}

	virtual LVStreamRef loadData(lString16 id) {
		JNIEnv *penv = NULL;
		bool attached = false;
		mJavaVM->GetEnv((void **) &penv, JNI_VERSION_1_6);
		if (NULL == penv) {
			// caller thread is not attached yet
			mJavaVM->AttachCurrentThread(&penv, NULL);
			attached = true;
		}
		LVStreamRef stream = LVStreamRef();
		jclass pjcEngine = penv->FindClass("org/coolreader/crengine/Engine");
		if (NULL == pjcEngine)
			return stream;
		jmethodID pjmEngine_loadHyphDictData = penv->GetStaticMethodID(pjcEngine, "loadHyphDictData", "(Ljava/lang/String;)[B");
		if (NULL == pjmEngine_loadHyphDictData)
			return stream;
		CRJNIEnv env(penv);
		jstring jid = env.toJavaString(id);
		jbyteArray data = static_cast<jbyteArray>(penv->CallStaticObjectMethod(pjcEngine, pjmEngine_loadHyphDictData, jid));
		stream = env.jbyteArrayToStream(data);
		if (attached)
			mJavaVM->DetachCurrentThread();
		return stream;
	}
};

jboolean initDictionaries(JNIEnv *penv, jclass clazz, jobjectArray dictArray) {
	jclass pjcHyphDict = penv->FindClass("org/coolreader/crengine/Engine$HyphDict");
	if (NULL == pjcHyphDict)
		return JNI_FALSE;
	jfieldID pjfHyphDict_type = penv->GetFieldID(pjcHyphDict, "type", "I");
	if (NULL == pjfHyphDict_type)
		return JNI_FALSE;
    jfieldID pjfHyphDict_code = penv->GetFieldID(pjcHyphDict, "code", "Ljava/lang/String;");
    if (NULL == pjfHyphDict_code)
        return JNI_FALSE;

	int len = penv->GetArrayLength(dictArray);
	HyphDictionary *dict;
	CRJNIEnv env(penv);
	HyphDictType dict_type;
	for (int i = 0; i < len; i++) {
		jobject obj = penv->GetObjectArrayElement(dictArray, i);
		int type = penv->GetIntField(obj, pjfHyphDict_type);
		jstring code = static_cast<jstring>(penv->GetObjectField(obj, pjfHyphDict_code));
		switch (type) {     // convert org/coolreader/crengine/Engine$HyphDict$type into HyphDictType
			case 0:         // org/coolreader/crengine/Engine$HYPH_NONE
				dict_type = HDT_NONE;
				break;
			case 1:         // org/coolreader/crengine/Engine$HYPH_ALGO
				dict_type = HDT_ALGORITHM;
				break;
			case 2:         // org/coolreader/crengine/Engine$HYPH_DICT
				dict_type = HDT_DICT_ALAN;
				break;
			default:
				dict_type = HDT_NONE;
				break;
		}
		lString16 dict_code = env.fromJavaString(code);
		dict = new HyphDictionary(dict_type, dict_code, dict_code, dict_code);
		if (!HyphMan::addDictionaryItem(dict))
		    delete dict;
	}
	JavaVM *jvm;
	env->GetJavaVM(&jvm);
	HyphMan::setDataLoader(new HyphDataLoaderProxy( jvm ));
	return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_org_coolreader_crengine_Engine_initDictionaries
 (JNIEnv * penv, jclass clazz, jobjectArray dictArray)
{
    jboolean res = JNI_FALSE;
    COFFEE_TRY_JNI(penv, res = initDictionaries(penv, clazz, dictArray));
    return res;
}

/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    uninitInternal
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_coolreader_crengine_Engine_uninitInternal
  (JNIEnv *, jclass)
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
  (JNIEnv * penv, jclass obj)
{
	LOGI("getFontFaceListInternal called");
	CRJNIEnv env(penv);
	lString16Collection list;
	COFFEE_TRY_JNI(penv, fontMan->getFaceList(list));
	return env.toJavaStringArray(list);
}

/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    getFontFileNameListInternal
 * Signature: ()[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL Java_org_coolreader_crengine_Engine_getFontFileNameListInternal
        (JNIEnv * penv, jclass cls)
{
    LOGI("getFontFileListInternal called");
    CRJNIEnv env(penv);
    lString16Collection list;
    COFFEE_TRY_JNI(penv, fontMan->getFontFileNameList(list));
    return env.toJavaStringArray(list);
}

/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    setCacheDirectoryInternal
 * Signature: (Ljava/lang/String;I)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_Engine_setCacheDirectoryInternal
  (JNIEnv * penv, jclass obj, jstring dir, jint size)
{
	CRJNIEnv env(penv);
	bool res = false;
	COFFEE_TRY_JNI(penv, res = ldomDocCache::init(env.fromJavaString(dir), size ));
	return res ? JNI_TRUE : JNI_FALSE;
}

/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    haveFcLangCodeInternal
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_Engine_haveFcLangCodeInternal
		(JNIEnv *env, jclass cls, jstring langCode)
{
	jboolean res = JNI_FALSE;
	const char* langCode_ptr = env->GetStringUTFChars(langCode, 0);
	if (langCode_ptr) {
		struct fc_lang_catalog* lang_ptr = fc_lang_cat;
		for (int i = 0; i < fc_lang_cat_sz; i++)
		{
			if (strcmp(lang_ptr->lang_code, langCode_ptr) == 0)
			{
				res = JNI_TRUE;
				break;
			}
			lang_ptr++;
		}
		env->ReleaseStringUTFChars(langCode, langCode_ptr);
	}
	return res;
}


/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    checkFontLanguageCompatibilityInternal
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_Engine_checkFontLanguageCompatibilityInternal
		(JNIEnv *env, jclass cls, jstring fontFace, jstring langCode)
{
	jboolean res = JNI_TRUE;
	const char* fontFace_ptr = env->GetStringUTFChars(fontFace, 0);
	const char* langCode_ptr = env->GetStringUTFChars(langCode, 0);
	if (fontFace_ptr && langCode_ptr) {
		res = fontMan->checkFontLangCompat(lString8(fontFace_ptr), lString8(langCode_ptr)) ? JNI_TRUE : JNI_FALSE;
	}
	if (langCode_ptr)
		env->ReleaseStringUTFChars(langCode, langCode_ptr);
	if (fontFace_ptr)
		env->ReleaseStringUTFChars(fontFace, fontFace_ptr);
	return res;
}

/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    listFilesInternal
 * Signature: (Ljava/io/File;)[Ljava/io/File;
 */
JNIEXPORT jobjectArray JNICALL Java_org_coolreader_crengine_Engine_listFilesInternal
		(JNIEnv *penv, jclass, jobject jdir)
{
	CRJNIEnv env(penv);
	if (NULL == jdir)
		return NULL;
	jclass pjcFile = env->GetObjectClass(jdir);
	if (NULL == pjcFile)
		return NULL;
	jmethodID pjmFile_GetAbsolutePath = env->GetMethodID(pjcFile, "getAbsolutePath", "()Ljava/lang/String;");
	if (NULL == pjmFile_GetAbsolutePath)
		return NULL;
	jmethodID pjmFile_Ctor = env->GetMethodID(pjcFile, "<init>", "(Ljava/lang/String;)V");
	if (NULL == pjmFile_Ctor)
		return NULL;
	jstring jpathname = (jstring)env->CallObjectMethod(jdir, pjmFile_GetAbsolutePath);
	if (NULL == jpathname)
		return NULL;
	lString16 path = env.fromJavaString(jpathname);
	jobjectArray jarray = NULL;
	LVContainerRef dir = LVOpenDirectory(path);
	if ( !dir.isNull() ) {
		jstring emptyString = env->NewStringUTF("");
		jobject emptyFile = env->NewObject(pjcFile, pjmFile_Ctor, emptyString);
		jarray = env->NewObjectArray(dir->GetObjectCount(), pjcFile, emptyFile);
		if (NULL != jarray) {
			for (int i = 0; i < dir->GetObjectCount(); i++) {
				const LVContainerItemInfo *item = dir->GetObjectInfo(i);
				if (item && item->GetName()) {
					lString16 fileName = path + "/" + item->GetName();
					jstring jfilename = env.toJavaString(fileName);
					if (NULL != jfilename) {
						env->ExceptionClear();
						jobject jfile = env->NewObject(pjcFile, pjmFile_Ctor, jfilename);
						if (env->ExceptionCheck() == JNI_TRUE)
							env->ExceptionClear();
						else {
							if (NULL != jfile)
								env->SetObjectArrayElement(jarray, i, jfile);
						}
						env->DeleteLocalRef(jfile);
						env->DeleteLocalRef(jfilename);
					}
				}
			}
		}
	}
	return jarray;
}

/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    isLink
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_coolreader_crengine_Engine_isLink
  (JNIEnv * env, jclass obj, jstring pathname)
{
	//CRLog::trace("isLink : enter");
	if (!pathname)
		return NULL;
	//CRLog::trace("isLink : pathname is not null");
	int res = JNI_FALSE;
	jboolean iscopy;
	const char * s = env->GetStringUTFChars(pathname, &iscopy);
	//CRLog::trace("isLink : read utf from pathname");
	struct stat st;
	lString8 path;
	if ( !lstat( s, &st) ) {
		if ( S_ISLNK(st.st_mode) ) {
			char buf[2048];
			int len = readlink(s, buf, sizeof(buf) - 1);
			if (len != -1) {
				buf[len] = 0;
				path = lString8(buf);
			}
		}
	}
	//CRLog::trace("isLink : releasing utf pathname");
	env->ReleaseStringUTFChars(pathname, s);
	//CRLog::trace("isLink : returning");
	return !path.empty() ? (jstring)env->NewGlobalRef(env->NewStringUTF(path.c_str())) : NULL;
}


/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    suspendLongOperationInternal
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_coolreader_crengine_Engine_suspendLongOperationInternal
  (JNIEnv *, jclass)
{
	_timeoutControl.cancel();
}


#define BUTTON_BACKLIGHT_CONTROL_PATH "/sys/class/leds/button-backlight/brightness"
/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    setKeyBacklightInternal
 * Signature: (I)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_Engine_setKeyBacklightInternal
  (JNIEnv *, jclass, jint n)
{
	FILE * f = fopen(BUTTON_BACKLIGHT_CONTROL_PATH, "wb");
	if (!f)
		return JNI_FALSE;
	fwrite(n ? "1" : "0", 1, 1, f);
	fclose(f);
	return JNI_TRUE;
}

/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    getDomVersionCurrent
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_coolreader_crengine_Engine_getDomVersionCurrent
  (JNIEnv *, jclass)
{
	return gDOMVersionCurrent;
}

//=====================================================================

static JNINativeMethod sEngineMethods[] = {
  /* name, signature, funcPtr */
  {"initInternal", "([Ljava/lang/String;I)Z", (void*)Java_org_coolreader_crengine_Engine_initInternal},
  {"uninitInternal", "()V", (void*)Java_org_coolreader_crengine_Engine_uninitInternal},
  {"initDictionaries", "([Lorg/coolreader/crengine/Engine$HyphDict;)Z", (void*)Java_org_coolreader_crengine_Engine_initDictionaries},
  {"getFontFaceListInternal", "()[Ljava/lang/String;", (void*)Java_org_coolreader_crengine_Engine_getFontFaceListInternal},
  {"setCacheDirectoryInternal", "(Ljava/lang/String;I)Z", (void*)Java_org_coolreader_crengine_Engine_setCacheDirectoryInternal},
  {"scanBookPropertiesInternal", "(Lorg/coolreader/crengine/FileInfo;)Z", (void*)Java_org_coolreader_crengine_Engine_scanBookPropertiesInternal},
  {"updateFileCRC32Internal", "(Lorg/coolreader/crengine/FileInfo;)Z", (void*)Java_org_coolreader_crengine_Engine_updateFileCRC32Internal},
  {"getArchiveItemsInternal", "(Ljava/lang/String;)[Ljava/lang/String;", (void*)Java_org_coolreader_crengine_Engine_getArchiveItemsInternal},
  {"isLink", "(Ljava/lang/String;)Ljava/lang/String;", (void*)Java_org_coolreader_crengine_Engine_isLink},
  {"suspendLongOperationInternal", "()V", (void*)Java_org_coolreader_crengine_Engine_suspendLongOperationInternal},
  {"setKeyBacklightInternal", "(I)Z", (void*)Java_org_coolreader_crengine_Engine_setKeyBacklightInternal},
  {"scanBookCoverInternal", "(Ljava/lang/String;)[B", (void*)Java_org_coolreader_crengine_Engine_scanBookCoverInternal},
  {"drawBookCoverInternal", "(Landroid/graphics/Bitmap;[BLjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;II)V", (void*)Java_org_coolreader_crengine_Engine_drawBookCoverInternal},
  {"haveFcLangCodeInternal", "(Ljava/lang/String;)Z", (void*)Java_org_coolreader_crengine_Engine_haveFcLangCodeInternal},
  {"checkFontLanguageCompatibilityInternal", "(Ljava/lang/String;Ljava/lang/String;)Z", (void*)Java_org_coolreader_crengine_Engine_checkFontLanguageCompatibilityInternal},
  {"listFilesInternal", "(Ljava/io/File;)[Ljava/io/File;", (void*)Java_org_coolreader_crengine_Engine_listFilesInternal},
  {"getDomVersionCurrent", "()I", (void*)Java_org_coolreader_crengine_Engine_getDomVersionCurrent}
};


static JNINativeMethod sDocViewMethods[] = {
  /* name, signature, funcPtr */
  {"createInternal", "()V", (void*)Java_org_coolreader_crengine_DocView_createInternal},
  {"destroyInternal", "()V", (void*)Java_org_coolreader_crengine_DocView_destroyInternal},
  {"getPageImageInternal", "(Landroid/graphics/Bitmap;I)V", (void*)Java_org_coolreader_crengine_DocView_getPageImageInternal},
  {"loadDocumentInternal", "(Ljava/lang/String;)Z", (void*)Java_org_coolreader_crengine_DocView_loadDocumentInternal},
  {"loadDocumentFromMemoryInternal", "([BLjava/lang/String;)Z", (void*)Java_org_coolreader_crengine_DocView_loadDocumentFromMemoryInternal},
  {"getSettingsInternal", "()Ljava/util/Properties;", (void*)Java_org_coolreader_crengine_DocView_getSettingsInternal},
  {"getDocPropsInternal", "()Ljava/util/Properties;", (void*)Java_org_coolreader_crengine_DocView_getDocPropsInternal},
  {"applySettingsInternal", "(Ljava/util/Properties;)Z", (void*)Java_org_coolreader_crengine_DocView_applySettingsInternal},
  {"setStylesheetInternal", "(Ljava/lang/String;)V", (void*)Java_org_coolreader_crengine_DocView_setStylesheetInternal},
  {"resizeInternal", "(II)V", (void*)Java_org_coolreader_crengine_DocView_resizeInternal},
  {"doCommandInternal", "(II)Z", (void*)Java_org_coolreader_crengine_DocView_doCommandInternal},
  {"getCurrentPageBookmarkInternal", "()Lorg/coolreader/crengine/Bookmark;", (void*)Java_org_coolreader_crengine_DocView_getCurrentPageBookmarkInternal},
  {"goToPositionInternal", "(Ljava/lang/String;Z)Z", (void*)Java_org_coolreader_crengine_DocView_goToPositionInternal},
  {"getPositionPropsInternal", "(Ljava/lang/String;Z)Lorg/coolreader/crengine/PositionProperties;", (void*)Java_org_coolreader_crengine_DocView_getPositionPropsInternal},
  {"updateBookInfoInternal", "(Lorg/coolreader/crengine/BookInfo;)V", (void*)Java_org_coolreader_crengine_DocView_updateBookInfoInternal},
  {"getTOCInternal", "()Lorg/coolreader/crengine/TOCItem;", (void*)Java_org_coolreader_crengine_DocView_getTOCInternal},
  {"clearSelectionInternal", "()V", (void*)Java_org_coolreader_crengine_DocView_clearSelectionInternal},
  {"findTextInternal", "(Ljava/lang/String;III)Z", (void*)Java_org_coolreader_crengine_DocView_findTextInternal},
  {"setBatteryStateInternal", "(I)V", (void*)Java_org_coolreader_crengine_DocView_setBatteryStateInternal},
  {"getCoverPageDataInternal", "()[B", (void*)Java_org_coolreader_crengine_DocView_getCoverPageDataInternal},
  {"setPageBackgroundTextureInternal", "([BI)V", (void*)Java_org_coolreader_crengine_DocView_setPageBackgroundTextureInternal},
  {"updateSelectionInternal", "(Lorg/coolreader/crengine/Selection;)V", (void*)Java_org_coolreader_crengine_DocView_updateSelectionInternal},
  {"checkLinkInternal", "(III)Ljava/lang/String;", (void*)Java_org_coolreader_crengine_DocView_checkLinkInternal},
  {"goLinkInternal", "(Ljava/lang/String;)I", (void*)Java_org_coolreader_crengine_DocView_goLinkInternal},
  {"moveSelectionInternal", "(Lorg/coolreader/crengine/Selection;II)Z", (void*)Java_org_coolreader_crengine_DocView_moveSelectionInternal},
  {"swapToCacheInternal", "()I", (void*)Java_org_coolreader_crengine_DocView_swapToCacheInternal},
  {"checkImageInternal", "(IILorg/coolreader/crengine/ImageInfo;)Z", (void*)Java_org_coolreader_crengine_DocView_checkImageInternal},
  {"drawImageInternal", "(Landroid/graphics/Bitmap;ILorg/coolreader/crengine/ImageInfo;)Z", (void*)Java_org_coolreader_crengine_DocView_drawImageInternal},
  {"closeImageInternal", "()Z", (void*)Java_org_coolreader_crengine_DocView_closeImageInternal},
  {"hilightBookmarksInternal", "([Lorg/coolreader/crengine/Bookmark;)V", (void*)Java_org_coolreader_crengine_DocView_hilightBookmarksInternal},
  {"checkBookmarkInternal", "(IILorg/coolreader/crengine/Bookmark;)Z", (void*)Java_org_coolreader_crengine_DocView_checkBookmarkInternal},
  {"isRenderedInternal", "()Z", (void*)Java_org_coolreader_crengine_DocView_isRenderedInternal}
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
   jint res = -1;
 
#ifdef JNI_VERSION_1_6
    if (res==-1 && vm->GetEnv((void**) &env, JNI_VERSION_1_6) == JNI_OK) {
        LOGI("JNI_OnLoad: JNI_VERSION_1_6\n");
   	    res = JNI_VERSION_1_6;
    }
#endif
#ifdef JNI_VERSION_1_4
    if (res==-1 && vm->GetEnv((void**) &env, JNI_VERSION_1_4) == JNI_OK) {
        LOGI("JNI_OnLoad: JNI_VERSION_1_4\n");
   	    res = JNI_VERSION_1_4;
    }
#endif
#ifdef JNI_VERSION_1_2
    if (res==-1 && vm->GetEnv((void**) &env, JNI_VERSION_1_2) == JNI_OK) {
        LOGI("JNI_OnLoad: JNI_VERSION_1_2\n");
   	    res = JNI_VERSION_1_2;
    }
#endif
	if ( res==-1 )
		return res;
 
    jniRegisterNativeMethods(env, "org/coolreader/crengine/Engine", sEngineMethods, sizeof(sEngineMethods)/sizeof(JNINativeMethod));
    jniRegisterNativeMethods(env, "org/coolreader/crengine/DocView", sDocViewMethods, sizeof(sDocViewMethods)/sizeof(JNINativeMethod));
    LOGI("JNI_OnLoad: native methods are registered!\n");
    return res;
}
