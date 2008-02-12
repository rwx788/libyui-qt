/*---------------------------------------------------------------------\
|								       |
|		       __   __	  ____ _____ ____		       |
|		       \ \ / /_ _/ ___|_   _|___ \		       |
|			\ V / _` \___ \ | |   __) |		       |
|			 | | (_| |___) || |  / __/		       |
|			 |_|\__,_|____/ |_| |_____|		       |
|								       |
|				core system			       |
|							 (C) SuSE GmbH |
\----------------------------------------------------------------------/

  File:	      YQMainWinDock.cc

  Author:     Stefan Hundhammer <sh@suse.de>

/-*/


#define YUILogComponent "qt-ui"
#include "YUILog.h"
#include <QTimer>
#include <QResizeEvent>
#include <YDialog.h>
#include <YQUI.h>
#include <YEvent.h>
#include "YQMainWinDock.h"

#define VERBOSE_RESIZE 0


YQMainWinDock *
YQMainWinDock::mainWinDock()
{
    static YQMainWinDock * mainWinDock = 0;

    if ( ! mainWinDock )
	mainWinDock = new YQMainWinDock();

    return mainWinDock;
}


YQMainWinDock::YQMainWinDock()
    : QWidget( 0, // parent, name
	       YQUI::ui()->noBorder() ?
	       Qt::FramelessWindowHint :
	       Qt::Window )
{
    setFocusPolicy( Qt::StrongFocus );

    resize( YQUI::ui()->defaultSize( YD_HORIZ ),
	    YQUI::ui()->defaultSize( YD_VERT  ) );

    yuiDebug() << "MainWinDock initial size: "
	       << size().width() << " x " << size().height()
	       << endl;
}


YQMainWinDock::~YQMainWinDock()
{
    // NOP
}


void
YQMainWinDock::resizeEvent( QResizeEvent * event )
{
    if ( event )
    {
	resize( event->size() );
	resizeVisibleChild();
    }
}


void
YQMainWinDock::resizeVisibleChild()
{
    if ( ! _widgetStack.empty() )
    {
	QWidget * dialog = _widgetStack.back();

	if ( dialog->size() != size() )
	{
#if VERBOSE_RESIZE
	    yuiDebug() << "Resizing child dialog " << hex << ( (void *) dialog ) << dec
		       << " to " << size().width() << " x " << size().height()
		       << endl;
#endif
	    dialog->resize( size() );
	}
    }
}


void
YQMainWinDock::show()
{
    QWidget::show();

    if ( ! _widgetStack.empty() )
    {
	QWidget * dialog = _widgetStack.back();
        dialog->raise();
        dialog->show();
    }
}


void
YQMainWinDock::add( QWidget * dialog )
{
    YUI_CHECK_PTR( dialog );

    if ( !_widgetStack.empty() )
        _widgetStack.back()->hide();

    dialog->raise();
    dialog->show();

    yuiDebug() << "Adding dialog " << hex << (void *) dialog << dec
	       << "  to mainWinDock"
	       << endl;
    
    _widgetStack.push_back( dialog );
    resizeVisibleChild();

    show();
}


void
YQMainWinDock::showCurrentDialog()
{
    if ( ! _widgetStack.empty() )
    {
	QWidget * dialog = _widgetStack.back();
	yuiDebug() << "Showing dialog " << hex << (void *) dialog << dec << endl;
	dialog->raise();
	update();
    }
}


void
YQMainWinDock::remove( QWidget * dialog )
{
    if ( _widgetStack.empty() )
	return;

    if ( ! dialog )
	dialog = _widgetStack.back();

    if ( dialog == _widgetStack.back() )
    {
	// The most common case:
	// The topmost dialog is to be removed

	_widgetStack.pop_back();

	yuiDebug() << "Removing dialog " << hex << (void *) dialog << dec
		   <<" from mainWinDock"
		   << endl;
    }
    else // The less common (but more generic) case: Remove any dialog
    {
	YQMainWinDock::YQWidgetStack::iterator pos = findInStack( dialog );

	if ( pos == _widgetStack.end() )
	    return;

	yuiWarning() << "Found dialog somewhere in the middle of the widget stack" << endl;
	yuiDebug() << "Removing dialog " << hex << (void *) dialog << dec
		   << " from mainWinDock"
		   << endl;

	_widgetStack.erase( pos );
    }

    if ( _widgetStack.empty() )		// No more main dialog?
	hide();				// -> hide dock
    else
    {
	dialog = _widgetStack.back();	// Get the next dialog from the stack
	dialog->raise();		// and raise it
        dialog->show();
	resizeVisibleChild();
    }
}


YQMainWinDock::YQWidgetStack::iterator
YQMainWinDock::findInStack( QWidget * dialog )
{
    for ( YQMainWinDock::YQWidgetStack::iterator it = _widgetStack.begin();
	  it != _widgetStack.end();
	  ++it )
    {
	if ( *it == dialog )
	    return it;
    }

    return _widgetStack.end();
}


QWidget *
YQMainWinDock::topmostDialog() const
{
    if ( _widgetStack.empty() )
	return 0;
    else
	return _widgetStack.back();
}


bool
YQMainWinDock::couldDock()
{
    YDialog * topDialog = YDialog::topmostDialog( false ); // don't throw

    if ( ! topDialog )	// No dialog at all?
	return true;	// Can dock the next one without problems

    // The next dialog can be docked if there is no popup dialog currently open.
    // This is equivalent to the topmost dialog on the YDialog stack being the
    // same dialog as the topmost dialog of this MainWinDock.

    return topDialog->widgetRep() == this->topmostDialog();
}


void
YQMainWinDock::closeEvent( QCloseEvent * event )
{
    // The window manager "close window" button (and WM menu, e.g. Alt-F4) will be
    // handled just like the user had clicked on the `id`( `cancel ) button in
    // that dialog. It's up to the YCP application to handle this (if desired).

    yuiMilestone() << "Caught window manager close event - returning with YCancelEvent" << endl;
    event->ignore();
    YQUI::ui()->sendEvent( new YCancelEvent() );
}


void
YQMainWinDock::paintEvent( QPaintEvent * event )
{
    // NOP
}


#include "YQMainWinDock.moc"
