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

  File:	      YQPkgList.cc

  Author:     Stefan Hundhammer <sh@suse.de>

  Textdomain "packages-qt"

/-*/

#define SOURCE_RPM_DISABLED 0

#define y2log_component "qt-pkg"
#include <ycp/y2log.h>
#include <qpixmap.h>
#include <qaction.h>
#include <qpopupmenu.h>

#include <Y2PM.h>
#include <y2pm/InstTarget.h>
#include "utf8.h"

#include "YQPkgList.h"
#include "YUIQt.h"
#include "YQi18n.h"
#include "YQIconPool.h"


YQPkgList::YQPkgList( QWidget *parent )
    : YQPkgObjList( parent )
{
    _srpmStatusCol	= -42;
    int installedPkgs	= Y2PM::instTarget().numPackages();

    int numCol = 0;
    addColumn( ""		);	_statusCol	= numCol++;
    // _statusCol = numCol;
    addColumn( _( "Package" ) );	_nameCol	= numCol++;

    if ( installedPkgs > 0 )
    {
	addColumn( _( "Avail. Ver." ) ); _versionCol	= numCol++;
	addColumn( _( "Inst. Ver."  ) ); _instVersionCol = numCol++;
    }
    else
    {
	addColumn( _( "Version"	) );	_versionCol	= numCol++;
	_instVersionCol = -1;
    }
    addColumn( _( "Summary"	) );	_summaryCol	= numCol++;
    addColumn( _( "Size"	) );	_sizeCol	= numCol++;

#if SOURCE_RPM_DISABLED
#warning Selecting source RPMs disabled!
#else
    addColumn( _( "Source"	) );	_srpmStatusCol	= numCol++;
#endif

    saveColumnWidths();
    setSorting( nameCol() );
    setColumnAlignment( sizeCol(), Qt::AlignRight );
    setAllColumnsShowFocus( true );

    createSourceRpmContextMenu();
}


YQPkgList::~YQPkgList()
{
    // NOP
}


void
YQPkgList::addPkgItem( PMPackagePtr pmPkg )
{
    if ( ! pmPkg )
    {
	y2error( "Null PMPackage !" );
	return;
    }

    new YQPkgListItem( this, pmPkg );
}


void
YQPkgList::pkgObjClicked( int 			button,
			  QListViewItem *	listViewItem,
			  int 			col,
			  const QPoint & 	pos )
{
    if ( col == srpmStatusCol() )
    {
	YQPkgListItem * item = dynamic_cast<YQPkgListItem *> (listViewItem);

	if ( item )
	{
	    if ( button == Qt::LeftButton )
	    {
		if ( editable() && item->editable() )
		    item->toggleSourceRpmStatus();
		return;
	    }
	    else if ( button == Qt::RightButton )
	    {
		if ( editable() && item->editable() )
		{
		    _actionInstallSourceRpm->setEnabled( item->hasSourceRpm() );
		    _actionDontInstallSourceRpm->setEnabled( item->hasSourceRpm() );

		    if ( _sourceRpmContextMenu )
			_sourceRpmContextMenu->popup( pos );
		}

		return;
	    }
	}
    }

    YQPkgObjList::pkgObjClicked( button, listViewItem, col, pos );
}


QSize
YQPkgList::sizeHint() const
{
    return QSize( 600, 350 );
}


void
YQPkgList::createSourceRpmContextMenu()
{
    _actionInstallSourceRpm	= createAction( YQIconPool::pkgInstall(),  _( "&Install Source"	      ) );
    _actionDontInstallSourceRpm = createAction( YQIconPool::pkgNoInst(),   _( "Do&n't Install Source" ) );

    connect( _actionInstallSourceRpm,	  SIGNAL( activated() ), this, SLOT( setInstallCurrentSourceRpm()     ) );
    connect( _actionDontInstallSourceRpm, SIGNAL( activated() ), this, SLOT( setDontInstallCurrentSourceRpm() ) );

    _sourceRpmContextMenu = new QPopupMenu( this );

    _actionInstallSourceRpm->addTo( _sourceRpmContextMenu );
    _actionDontInstallSourceRpm->addTo( _sourceRpmContextMenu );
}


void
YQPkgList::setInstallCurrentSourceRpm( bool installSourceRpm,
				       bool selectNextItem )
{
    QListViewItem * listViewItem = selectedItem();

    if ( ! listViewItem )
	return;

    YQPkgListItem * item = dynamic_cast<YQPkgListItem *> (listViewItem);

    if ( item )
    {
	item->setInstallSourceRpm( installSourceRpm );

	if ( selectNextItem && item->nextSibling() )
	{
	    item->setSelected( false );			// doesn't emit signals
	    setSelected( item->nextSibling(), true );	// emits signals
	}
    }
}







YQPkgListItem::YQPkgListItem( YQPkgList * pkgList, PMPackagePtr pmPkg )
    : YQPkgObjListItem( pkgList, pmPkg )
    , _pkgList( pkgList )
    , _pmPkg( pmPkg )
{
    CHECK_PTR( pmPkg );
    CHECK_PTR( pkgList );

    setSourceRpmIcon();
}



YQPkgListItem::~YQPkgListItem()
{
    // NOP
}


void
YQPkgListItem::updateData()
{
    YQPkgObjListItem::updateData();
    setSourceRpmIcon();
}



bool
YQPkgListItem::hasSourceRpm() const
{
    PMSelectablePtr sel = _pmPkg->getSelectable();

    if ( ! sel )
	return false;

    return sel->providesSources();
}


bool
YQPkgListItem::installSourceRpm() const
{
    PMSelectablePtr sel = _pmPkg->getSelectable();

    if ( ! sel )
	return false;

    if ( ! sel->providesSources() )
	return false;

    return sel->source_install();
}


void
YQPkgListItem::setSourceRpmIcon()
{
    if ( srpmStatusCol() < 0 )
	return;
    
    QPixmap icon;

    if ( hasSourceRpm() )
    {

	if ( editable() && _pkgObjList->editable() )
	{
	    icon = installSourceRpm() ?
		YQIconPool::pkgInstall() :
		YQIconPool::pkgNoInst();
	}
	else
	{
	    icon = installSourceRpm() ?
		YQIconPool::disabledPkgInstall() :
		YQIconPool::disabledPkgNoInst();
	}
    }

    setPixmap( srpmStatusCol(), icon );
}


void
YQPkgListItem::setInstallSourceRpm( bool installSourceRpm )
{
    if ( hasSourceRpm() )
    {
	PMSelectablePtr sel = _pmPkg->getSelectable();

	if ( sel )
	    sel->set_source_install( installSourceRpm );
    }

    setSourceRpmIcon();
}


void
YQPkgListItem::toggleSourceRpmStatus()
{
    setInstallSourceRpm( ! installSourceRpm() );
}



/**
 * Comparison function used for sorting the list.
 * Returns:
 * -1 if this <	 other
 *  0 if this == other
 * +1 if this >	 other
 **/
int
YQPkgListItem::compare( QListViewItem *		otherListViewItem,
			int			col,
			bool			ascending ) const
{
    if ( col == srpmStatusCol() )
    {
	YQPkgListItem * other = dynamic_cast<YQPkgListItem *> (otherListViewItem);

	if ( other )
	{
	    int thisPoints  = ( this->hasSourceRpm()  ? 1 : 0 ) + ( this->installSourceRpm()  ? 1 : 0 );
	    int otherPoints = ( other->hasSourceRpm() ? 1 : 0 ) + ( other->installSourceRpm() ? 1 : 0 );

	    // Intentionally inverting order: Pkgs with source RPMs are more interesting than without.
	    if ( thisPoints > otherPoints ) return -1;
	    if ( thisPoints < otherPoints ) return  1;
	    return 0;
	}
    }

    // Fallback: Use parent class method
    return YQPkgObjListItem::compare( otherListViewItem, col, ascending );
}


void
YQPkgListItem::paintCell( QPainter *		painter,
			  const QColorGroup &	colorGroup,
			  int			column,
			  int			width,
			  int			alignment )
{
    if ( installedIsNewer() )
    {
	QColorGroup cg = colorGroup;


	if ( column == instVersionCol() )
	    cg.setColor( QColorGroup::Base, QColor( 0xFF, 0x30, 0x30 ) );	// Background
	else
	    cg.setColor( QColorGroup::Text, QColor( 0xFF, 0, 0 ) );		// Foreground

	QListViewItem::paintCell( painter, cg, column, width, alignment );
    }
    else if ( candidateIsNewer() )
    {
	QColorGroup cg = colorGroup;

	cg.setColor( QColorGroup::Text, QColor( 0, 0, 0xC0 ) );			// Foreground

	if ( column == versionCol() )
	    cg.setColor( QColorGroup::Base, QColor( 0xF0, 0xF0, 0xF0 ) );	// Background

	QListViewItem::paintCell( painter, cg, column, width, alignment );
    }
    else
    {
	QListViewItem::paintCell( painter, colorGroup, column, width, alignment );
    }
}



#include "YQPkgList.moc.cc"
