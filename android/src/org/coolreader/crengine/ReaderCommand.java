package org.coolreader.crengine;

public enum ReaderCommand
{
	DCMD_NONE(0),
	DCMD_REPEAT(1), // repeat last action
	
	//definitions from crengine/include/lvdocview.h
	DCMD_BEGIN(100),
	DCMD_LINEUP(101),
	DCMD_PAGEUP(102),
	DCMD_PAGEDOWN(103),
	DCMD_LINEDOWN(104),
	DCMD_LINK_FORWARD(105),
	DCMD_LINK_BACK(106),
	DCMD_LINK_NEXT(107),
	DCMD_LINK_PREV(108),
	DCMD_LINK_GO(109),
	DCMD_END(110),
	DCMD_GO_POS(111),
	DCMD_GO_PAGE(112),
	DCMD_ZOOM_IN(113),
	DCMD_ZOOM_OUT(114),
	DCMD_TOGGLE_TEXT_FORMAT(115),
	DCMD_BOOKMARK_SAVE_N(116),
	DCMD_BOOKMARK_GO_N(117),
	DCMD_MOVE_BY_CHAPTER(118),
	DCMD_GO_SCROLL_POS(119),
	DCMD_TOGGLE_PAGE_SCROLL_VIEW(120),
	DCMD_LINK_FIRST(121),
	DCMD_ROTATE_BY(122),
	DCMD_ROTATE_SET(123),
	DCMD_SAVE_HISTORY(124),
	DCMD_SAVE_TO_CACHE(125),
	DCMD_TOGGLE_BOLD(126),
	DCMD_SCROLL_BY(127),
	DCMD_REQUEST_RENDER(128),
	DCMD_GO_PAGE_DONT_SAVE_HISTORY(129),
	DCMD_SET_INTERNAL_STYLES(130),
	
    DCMD_SELECT_FIRST_SENTENCE(131), // select first sentence on page
    DCMD_SELECT_NEXT_SENTENCE(132), // move selection to next sentence
    DCMD_SELECT_PREV_SENTENCE(133), // move selection to next sentence
    DCMD_SELECT_MOVE_LEFT_BOUND_BY_WORDS(134), // move selection start by words 
    DCMD_SELECT_MOVE_RIGHT_BOUND_BY_WORDS(135), // move selection end by words 

	DCMD_SET_TEXT_FORMAT(136),

	DCMD_SET_DOC_FONTS(137),

	DCMD_SET_REQUESTED_DOM_VERSION(138),
	DCMD_SET_RENDER_BLOCK_RENDERING_FLAGS(139),


	// definitions from android/jni/readerview.h
	DCMD_OPEN_RECENT_BOOK(2000),
	DCMD_CLOSE_BOOK(2001),
	DCMD_RESTORE_POSITION(2002),

	// application actions
	DCMD_RECENT_BOOKS_LIST(2003),
	DCMD_SEARCH(2004),
	DCMD_EXIT(2005),
	DCMD_BOOKMARKS(2005),
	DCMD_GO_PERCENT_DIALOG(2006),
	DCMD_GO_PAGE_DIALOG(2007),
	DCMD_TOC_DIALOG(2008),
	DCMD_FILE_BROWSER(2009),
	DCMD_OPTIONS_DIALOG(2010),
	DCMD_TOGGLE_DAY_NIGHT_MODE(2011),
	DCMD_READER_MENU(2012),
	DCMD_TOGGLE_TOUCH_SCREEN_LOCK(2013),
	DCMD_TOGGLE_SELECTION_MODE(2014),
	DCMD_TOGGLE_ORIENTATION(2015),
	DCMD_TOGGLE_FULLSCREEN(2016),
	DCMD_SHOW_HOME_SCREEN(2017), // home screen activity
	DCMD_TOGGLE_DOCUMENT_STYLES(2018),
	DCMD_ABOUT(2019),
	DCMD_BOOK_INFO(2020),
	DCMD_TTS_PLAY(2021),
	DCMD_TOGGLE_TITLEBAR(2022),
	DCMD_SHOW_POSITION_INFO_POPUP(2023),
	DCMD_SHOW_DICTIONARY(2024),
	DCMD_OPEN_PREVIOUS_BOOK(2025),
	DCMD_TOGGLE_AUTOSCROLL(2026),
	DCMD_AUTOSCROLL_SPEED_INCREASE(2027),
	DCMD_AUTOSCROLL_SPEED_DECREASE(2028),
	DCMD_START_SELECTION(2029),
	DCMD_SWITCH_PROFILE(2030),
	DCMD_TOGGLE_TEXT_AUTOFORMAT(2031),

	DCMD_FONT_NEXT(2032),
	DCMD_FONT_PREVIOUS(2033),

	DCMD_USER_MANUAL(2034),
	DCMD_CURRENT_BOOK_DIRECTORY(2035),
	
	DCMD_OPDS_CATALOGS(2050),
	DCMD_FILE_BROWSER_ROOT(2051),
	DCMD_FILE_BROWSER_UP(2052),
	DCMD_CURRENT_BOOK(2053),
	DCMD_SCAN_DIRECTORY_RECURSIVE(2054),
	DCMD_FILE_BROWSER_SORT_ORDER(2055),

	DCMD_TOGGLE_DICT_ONCE(2056),
	DCMD_TOGGLE_DICT(2057),

	DCMD_BACKLIGHT_SET_DEFAULT(2058),

	DCMD_GOOGLEDRIVE_SYNC(2100),
	;
	
	final int nativeId;
	private ReaderCommand( int nativeId )
	{
		this.nativeId = nativeId;
	}
	
	public int getNativeId() {
		return nativeId;
	}
}