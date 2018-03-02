CoolReader 3 - free e-book reader
(c) Vadim Lopatin, 1998-2016

Forked from buggins/coolreader by virxkane.
(c) Aleksey Chernov, 2018

LICENSE: All source codes (except thirdparty directory) 
are provided under the terms of GNU GPL license, version 2

Source codes are available at sourceforge:
git clone https://github.com/virxkane/coolreader.git

Directories:

crengine   - CREngine (DOM/XML/CSS ebook rendering library) sources
cr3gui     - CR3 with CR3GUI for e-ink devices sources
cr3qt      - CR3 with Qt based GUI
cr3wx      - CR3 with wxWidgets based GUI
thirdparty - third party libraries, to use if not found in system (zlib, libpng, libjpeg, freetype)
tinydict   - small library for .dict file format support
tools      - miscellaneous configuration files
android    - Android port
cr3android - Android Studio project files.

External dependencies: 

common: zlib, libpng, libjpeg, freetype
cr3gui/xcb: libxcb, fontconfig
cr3gui/nanoX: libnanoX
cr3/Qt: qt4-core, qt4-gui
cr3/wx: wxWidgets 2.8

This is mod.virxkane branch. Used only to build releases (.apk) for Android.
No commits should be transferred from this branch to the master branch.
Before building the release, move the changes to this branch from the master.
