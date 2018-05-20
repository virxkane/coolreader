
set(CRENGINE_SOURCES
	src/cp_stats.cpp
	src/lvstring.cpp
	src/props.cpp
	src/rtfimp.cpp
	src/cri18n.cpp
	src/lvmemman.cpp
	src/lvstyles.cpp
	src/crtxtenc.cpp
	src/lvtinydom.cpp
	src/lvstream.cpp
	src/lvxml.cpp
	src/lvstsheet.cpp
	src/txtselector.cpp
	#src/xutils.cpp
	src/crtest.cpp
	src/crlog.cpp
	src/crfilelogger.cpp
	src/serialbuf.cpp
	src/lvstring8collection.cpp
	src/lvstring16collection.cpp
	src/lvstring16hashedcollection.cpp
	src/lvcrc32.cpp
	src/ldomnameidmapitem.cpp
	src/ldomnameidmap.cpp
)

if ( NOT ${GUI} STREQUAL FB2PROPS )
	set(CRENGINE_SOURCES ${CRENGINE_SOURCES}
		src/lvbmpbuf.cpp
		src/lvfnt.cpp
		src/hyphman.cpp
		src/lvfont.cpp
		src/lvbasefont.cpp
		src/lvembeddedfont.cpp
		src/lvbitmapfont.cpp
		src/lvbitmapfontman.cpp
		src/lvfreetypefont.cpp
		src/lvfontboldtrans.cpp
		src/lvfreetypefontman.cpp
		src/lvfontcache.cpp
		src/lvfontglyphcache.cpp
		src/lvfontman.cpp
		src/crgui.cpp
		src/lvimg.cpp
		src/crskin.cpp
		src/lvdrawbuf.cpp
		src/lvbasedrawbuf.cpp
		src/lvgraydrawbuf.cpp
		src/lvcolordrawbuf.cpp
		src/lvdocview.cpp
		src/lvpagesplitter.cpp
		src/lvtextfm.cpp
		src/lvrend.cpp
		src/wolutil.cpp
		src/chmfmt.cpp
		src/epubfmt.cpp
		src/pdbfmt.cpp
		src/wordfmt.cpp
		src/crconcurrent.cpp
		src/crbookmark.cpp
		src/crfilehist.cpp
		src/crfilehistrecord.cpp
		#src/xutils.cpp
	)
	if(WIN32)
		set(CRENGINE_SOURCES ${CRENGINE_SOURCES}
			src/lvwin32font.cpp
			src/lvwin32fontman.cpp
			)
	endif(WIN32)
endif (NOT ${GUI} STREQUAL FB2PROPS)

add_library(crengine STATIC ${CRENGINE_SOURCES})
