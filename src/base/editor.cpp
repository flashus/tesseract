/* Copyright (C) 2006, 2007, 2008 P.L. Lucas
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <fstream>

#include <boost/assert.hpp>

#include <QIcon>
#include <QToolBar>
#include <QMdiArea>
#include <QMenuBar>
#include <QPrinter>
#include <QBoxLayout>
#include <QClipboard>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextCursor>
#include <QPrintDialog>

#include "main.hpp"
#include "editor.hpp"
#include "config.hpp"
#include "projects.hpp"
#include "navigator.hpp"
#include "search_dialog.hpp"
#include "numberedtextview.hpp"

// Constructor
Editor::Editor( QWidget *parent ) : 
BaseWidget( parent ),
currentNtv( NULL ),
search_dialog( NULL ),
toolBar( addToolBar( tr( "Editor Toolbar" ) ) ),
contextMenu( new QMenu( this ) ),
tabWidget( new QTabWidget( this ) ),
menuContext( new QMenu( this ) ),
list_files_dock( new QDockWidget( tr("File list") , this ) ),
project_name( tr( "Empty" ) )
{
	// Title
	setWindowTitle( tr( "Editor" ) );
	setWindowIcon( QIcon( QApplication::applicationDirPath() + "/styles/default/images/xedit.png" ) );

	widget_type = EDITOR;

	createActions();
	createMenus();
	createToolBars();
	createConnections();

	// Layout
	QVBoxLayout *vLayout = new QVBoxLayout( 0 );
	centralWidget()->setLayout( vLayout );

	// TabWidget

	tabWidget->setTabsClosable( true );
	tabWidget->show();

	connect( tabWidget , SIGNAL( currentChanged( int )    ) , this , SLOT( tabChanged( int ) ) );
	connect( tabWidget , SIGNAL( tabCloseRequested( int ) ) , this , SLOT( closeTab( int )   ) );

	vLayout->addWidget( tabWidget );

	//List of files
	list_files_dock->setObjectName(list_files_dock->windowTitle());
	addDockWidget(Qt::LeftDockWidgetArea, list_files_dock);
	//list_files_dock->show();

	list_files = new QListView(list_files_dock);
	list_files_dock->setWidget(list_files);
	list_files_dock->show();
	list_files->setAcceptDrops(true);
	list_files->setDropIndicatorShown(true);
	list_files->setDragEnabled(true);
	list_files->viewport()->setAcceptDrops(true);
	list_files->setModel(new ListModel(parent, this));
	connect(list_files,SIGNAL(clicked (const QModelIndex &)), this, SLOT(file_selected(const QModelIndex &)) );

	//Clipboard list
	QDockWidget *list_clipboard_dock = new QDockWidget(tr("Small Clipboard") , this );
	list_clipboard_dock->setObjectName( list_clipboard_dock->windowTitle() );
	addDockWidget( Qt::LeftDockWidgetArea , list_clipboard_dock );

	list_clipboard = new ClipboardListView( list_clipboard_dock );
	list_clipboard_dock->setWidget( list_clipboard );
	list_clipboard_dock->show();

	connect(list_clipboard,SIGNAL(doubleClicked (const QModelIndex &)), this, SLOT(clipboard_double_clicked(const QModelIndex &)) );

	//setAcceptDrops(true);


	toolbar_action( actionNew );
}

void Editor::createConnectionsMenuEdit()
{
//	connect(menuEdit, SIGNAL( aboutToShow() ), this, SLOT(show_edit_menu()));
	connect( actionIndent    , SIGNAL( triggered() ) , this , SLOT( indent_callback()    ) );
	connect( actionUnindent  , SIGNAL( triggered() ) , this , SLOT( unindent_callback()  ) );
	connect( actionComment   , SIGNAL( triggered() ) , this , SLOT( comment_callback()   ) );
	connect( actionUncomment , SIGNAL( triggered() ) , this , SLOT( uncomment_callback() ) );
}

void Editor::createMenus()
{
	createMenuFile();
	createMenuEdit();
	createMenuView();
	createMenuRun();
	createMenuDocks();
	createMenuConfig();
	createMenuContext();
}

void Editor::createMenuDocks()
{
	menuDocks = createPopupMenu();
	menuDocks->setTitle( tr( "Show/Hide Objects" ) );
}

void Editor::createMenuConfig()
{
	menuToolBars = menuBar()->addMenu( tr( "Toolbars" ) );
	menuToolBars->addMenu(menuDocks);
}

void Editor::createMenuRun()
{
	menuRun = menuBar()->addMenu( tr( "Run" ) );

	menuRun->addAction( actionRun );
	menuRun->addAction( actionDebug );
	menuRun->addAction( actionStep );
	menuRun->addAction( actionSendToOctave );

	menuRun->addSeparator();

	menuRun->addAction( actionToggleBreakPoint );
}

void Editor::createMenuEdit()
{
	menuEdit = menuBar()->addMenu( tr( "Edit" ) );

	menuEdit->addAction( actionCopy );
	menuEdit->addAction( actionPaste );
	menuEdit->addAction( actionCut );

	menuEdit->addSeparator();

	menuEdit->addAction( actionIndent );
	menuEdit->addAction( actionUnindent );

	menuEdit->addSeparator();

	menuEdit->addAction( actionComment );
	menuEdit->addAction( actionUncomment );
}

void Editor::createMenuContext()
{
	menuContext->addAction( actionToggleBreakPoint );

	menuContext->addSeparator();

	menuContext->addActions( menuEdit->actions() );
}

void Editor::createMenuFile()
{
	menuFile = menuBar()->addMenu( tr( "File" ) );

	menuFile->addAction( actionNew );
	menuFile->addAction( actionOpen );
	menuFile->addAction( actionSave );
	menuFile->addAction( actionSaveAll );
	menuFile->addAction( actionSaveAs );

	menuFile->addSeparator();

	menuFile->addAction( actionPrint );

	menuFile->addSeparator();

	menuFile->addAction( actionClone );

	menuFile->addSeparator();

	menuFile->addAction( actionClose );

}

void Editor::resizeEvent( QResizeEvent * /* ATM unused */ )
{
	const bool notDocked = isWindow();

	if( isHidden() )
	{
		return;
	}

	//TODO: find a better way to avoid double code
	QFile file;

	if( notDocked )
	{
		file.setFileName( QApplication::applicationDirPath() + "/styles/default/editor/editor.css" );
	}
	else
	{
		file.setFileName( QApplication::applicationDirPath() + "/styles/default/editor/editor_docked.css" );
	}

	if( file.open( QFile::ReadOnly ) )
	{
		setStyleSheet( QLatin1String( file.readAll() ) ) ;
	}
	else
	{
		BOOST_ASSERT_MSG( false , "Could not open editor stylesheet file." );
	}

	file.close();

	// Toolbar

	if( notDocked )
	{
		file.setFileName( QApplication::applicationDirPath() + "/styles/default/editor/widgets/tab.css" );
	}
	else
	{
		file.setFileName( QApplication::applicationDirPath() + "/styles/default/editor/widgets/tab_docked.css" );
	}

	if( file.open( QFile::ReadOnly ) )
	{
		tabWidget->setStyleSheet( QLatin1String( file.readAll() ) ) ;
	}
	else
	{
		BOOST_ASSERT_MSG( false , "Could not open tab widget stylesheet file." );
	}

	file.close();

	// Clipboard

	if( notDocked )
	{
		file.setFileName( QApplication::applicationDirPath() + "/styles/default/editor/widgets/clipboard.css" );
	}
	else
	{
		file.setFileName( QApplication::applicationDirPath() + "/styles/default/editor/widgets/clipboard_docked.css" );
	}

	if( file.open( QFile::ReadOnly ) )
	{
		list_clipboard->setStyleSheet( QLatin1String( file.readAll() ) ) ;
	}
	else
	{
		BOOST_ASSERT_MSG( false , "Could not open clipboard stylesheet file." );
	}

	file.close();

	// Menubar

	if( notDocked )
	{
		file.setFileName( QApplication::applicationDirPath() + "/styles/default/editor/menubar.css" );
	}
	else
	{
		file.setFileName( QApplication::applicationDirPath() + "/styles/default/editor/menubar_docked.css" );
	}

	if( file.open( QFile::ReadOnly ) )
	{
		menuBar()->setStyleSheet( QLatin1String( file.readAll() ) ) ;
	}
	else
	{
		BOOST_ASSERT_MSG( false , "Could not open editor menubar stylesheet file." );
	}

	file.close();

	// Toolbar

	if( notDocked )
	{
		file.setFileName(QApplication::applicationDirPath()+"/styles/default/editor/toolbar.css" );
	}
	else
	{
		file.setFileName(QApplication::applicationDirPath()+"/styles/default/editor/toolbar_docked.css" );
	}

	if( file.open( QFile::ReadOnly ) )
	{
		toolBar->setStyleSheet( QLatin1String( file.readAll() ) ) ;
	}
	else
	{
		BOOST_ASSERT_MSG(false , "Could not open editor stylesheet file." );
	}

	file.close();

	// File List

	if( notDocked )
	{
		file.setFileName(QApplication::applicationDirPath()+"/styles/default/editor/widgets/filelist.css" );
	}
	else
	{
		file.setFileName(QApplication::applicationDirPath()+"/styles/default/editor/widgets/filelist_docked.css" );
	}

	if( file.open( QFile::ReadOnly ) )
	{
		list_files_dock->setStyleSheet( QLatin1String( file.readAll() ) ) ;
	}
	else
	{
		BOOST_ASSERT_MSG(false,"Could not open editor file list stylesheet file.");
	}

	file.close();

}

void Editor::createActionsMenu()
{
	createActionsMenuFile();
	createActionsMenuRun();
	createActionsMenuEdit();
}

void Editor::createActionsMenuFile()
{
	actionClone = new QAction( QIcon(QApplication::applicationDirPath() + "/styles/default/images/test.png") , tr("Clone View") , this );
	actionClone->setShortcut( tr("Alt+P") );
	
	actionPrint = new QAction( QIcon(QApplication::applicationDirPath() + "/styles/default/images/print.png") , tr("Print") , this );
	actionPrint->setShortcut(  tr("Ctrl+P") );

	actionCloseAll = new QAction( QIcon(QApplication::applicationDirPath() + "/styles/default/images/exit.png") , tr("Close") , this );
	actionCloseAll->setShortcut( tr("Ctrl+X") );
}

void Editor::createActionsMenuRun()
{
	actionToggleBreakPoint = new QAction( QIcon(QApplication::applicationDirPath() + "/styles/default/images/breakpoint.png") , tr("Insert/Remove Breakpoint") , this );
	actionToggleBreakPoint->setShortcut( tr("F9") );
}

void Editor::createActions() 
{
	createActionsMenu();
	createActionsMenuEdit();
	createActionsToolBarMain();
	//newFolder = new QAction(QIcon(QApplication::applicationDirPath() + "/styles/default/images/newfolder.png"), tr("&New folder"), this);
	//exit = new QAction(QIcon(QApplication::applicationDirPath() + "/styles/default/images/close.bmp"), tr("Exit"), this);

	//cut = new QAction(QIcon(QApplication::applicationDirPath() + "/styles/default/images/editcut.png"), tr("Cu&t"), this);
	//copy = new QAction(QIcon(QApplication::applicationDirPath() + "/styles/default/images/editcopy.png"), tr("&Copy"), this);
	//paste = new QAction(QIcon(QApplication::applicationDirPath() + "/styles/default/images/editpaste.png"), tr("&Paste"), this);

	//back = new QAction(QIcon(QApplication::applicationDirPath() + "/styles/default/images/back.png"), tr("&Back"), this);
	//forward = new QAction(QIcon(QApplication::applicationDirPath() + "/styles/default/images/forward.png"), tr("&Forward"), this);
	//up = new QAction(QIcon(QApplication::applicationDirPath() + "/styles/default/images/up.png"), tr("Up"), this);
	//remove = new QAction(QIcon(QApplication::applicationDirPath() + "/styles/default/images/delete.png"), tr("Delete file"), this);
	//rename = new QAction(tr("Rename"), this);
}

void Editor::createActionsMenuEdit()
{
	actionIndent = new QAction( QIcon(QApplication::applicationDirPath() + "/styles/default/images/indent.png") , tr("Indent") , this );
	actionIndent->setShortcut( tr("Ctrl+Tab") );

	actionUnindent = new QAction( QIcon(QApplication::applicationDirPath() + "/styles/default/images/outdent.png") , tr("Outdent") , this );
	actionUnindent->setShortcut(  tr("Shift+Tab")  );

	actionComment = new QAction( QIcon(QApplication::applicationDirPath() + "/styles/default/images/comment.png") , tr("Comment") , this );
	actionComment->setShortcut( tr("Ctrl+K") );

	actionUncomment = new QAction( QIcon(QApplication::applicationDirPath() + "/styles/default/images/uncomment.png") , tr("Uncomment") , this );
	actionUncomment->setShortcut( tr("Ctrl+U") );
}

void Editor::createActionsToolBarMain()
{
	actionNew = new QAction(QIcon(QApplication::applicationDirPath() + "/styles/default/images/filenew.png"), tr("New") , this );
	actionNew->setShortcut(tr("Ctrl+N"));
	actionNew->setShortcutContext(Qt::WindowShortcut);

	actionOpen = new QAction(QIcon(QApplication::applicationDirPath() + "/styles/default/images/fileopen.png"), tr("Open") , this );
	actionOpen->setShortcut(tr("Ctrl+O"));
	actionOpen->setShortcutContext(Qt::WindowShortcut);

	actionSave = new QAction(QIcon(QApplication::applicationDirPath() + "/styles/default/images/filesave.png"), tr("Save") , this );
	actionSave->setShortcut(tr("Ctrl+S"));
	actionSave->setShortcutContext(Qt::WindowShortcut);

	actionSaveAll = new QAction(QIcon(QApplication::applicationDirPath() + "/styles/default/images/filesaveall.png"), tr("Save All") , this );
	actionSaveAll->setShortcut(tr("Ctrl+Shift+S"));
	actionSaveAll->setShortcutContext(Qt::WindowShortcut);

	actionSaveAs = new QAction(QIcon(QApplication::applicationDirPath() + "/styles/default/images/filesaveas.png"), tr("Save as") , this );
	actionClose = new QAction(QIcon(QApplication::applicationDirPath() + "/styles/default/images/fileclose.png"), tr("Close tab") , this );

	actionRun = new QAction(QIcon(QApplication::applicationDirPath() + "/styles/default/images/run.png"), tr("Run") , this );
	actionRun->setShortcut(tr("Shift+F5"));
	actionRun->setShortcutContext(Qt::WindowShortcut);

	actionDebug = new QAction(QIcon(QApplication::applicationDirPath() + "/styles/default/images/debug.png"), tr("Debug") , this );
	actionDebug->setShortcut(tr("F5"));
	actionDebug->setShortcutContext(Qt::WindowShortcut);

	actionDebug->setToolTip
	(
		"<html>"
		"<b>Debug:</b>"
		"<p>Octave includes a built-in debugger to aid in the development of scripts. This can be used to interrupt the execution of an Octave script at a certain point.</p>"
		"<p>Click over this button to start debugging or continue to next breakpoint.</p>"
		"</html>"
	);

	actionStep = new QAction(QIcon(QApplication::applicationDirPath() + "/styles/default/images/step.png"), tr("Detailed debugging") , this );
	actionStep->setShortcut(tr("Shift+F6"));
	actionStep->setEnabled(false);

	actionSendToOctave = new QAction(QIcon(QApplication::applicationDirPath() + "/styles/default/images/console.png"), tr("Send to Octave") , this );
	actionSendToOctave->setShortcut(tr("F9"));
	actionSendToOctave->setShortcutContext(Qt::WindowShortcut);

	actionSendToOctave->setToolTip
	(
		"<html>"
		"<b>Send to Octave:</b>"
		"<p>Sends selected text to Octave. If there is not selected text, the full text will be sent.</p>"
		"</html>"
	);

	actionUndo = new QAction(QIcon(QApplication::applicationDirPath() + "/styles/default/images/undo.png"), tr("Undo") , this );
	actionUndo->setShortcut(tr("Ctrl+Z"));
	actionUndo->setShortcutContext(Qt::WindowShortcut);

	actionRedo = new QAction(QIcon(QApplication::applicationDirPath() + "/styles/default/images/redo.png"), tr("Redo") , this );
	actionRedo->setShortcut(tr("Ctrl+Shift+Z"));
	actionRedo->setShortcutContext(Qt::WindowShortcut);

	actionCut = new QAction(QIcon(QApplication::applicationDirPath() + "/styles/default/images/editcut.png"), tr("Cut") , this );
	actionCut->setShortcut(tr("Ctrl+X"));
	actionCut->setShortcutContext(Qt::WindowShortcut);

	actionCopy = new QAction(QIcon(QApplication::applicationDirPath() + "/styles/default/images/editcopy.png"), tr("Copy") , this );
	actionCopy->setShortcut(tr("Ctrl+C"));
	actionCopy->setShortcutContext(Qt::WindowShortcut);

	actionPaste = new QAction(QIcon(QApplication::applicationDirPath() + "/styles/default/images/editpaste.png"),  tr("Paste") , this );
	actionPaste->setShortcut(tr("Ctrl+V"));
	actionPaste->setShortcutContext(Qt::WindowShortcut);

	actionSearch = new QAction(QIcon(QApplication::applicationDirPath() + "/styles/default/images/find.png"),  tr("Search and replace") , this );
	actionSearch->setShortcut(tr("Ctrl+F"));
	actionSearch->setShortcutContext(Qt::WindowShortcut);
}

void Editor::customContextMenuPopUp( const QPoint & /*pos*/)
{
	menuContext->popup( QCursor::pos() );
}

void Editor::createToolBars()
{
	createToolBarMain();
}

// Toolbar & Context Menu
void Editor::createToolBarMain()
{
	toolBar->addAction( actionNew );
	toolBar->addAction( actionOpen );
	toolBar->addAction( actionSave );
	toolBar->addAction( actionSaveAs );
	toolBar->addAction( actionSaveAll );

	toolBar->addSeparator();

	toolBar->addAction( actionRun );
	toolBar->addAction( actionDebug );
	toolBar->addAction( actionStep );
	toolBar->addAction( actionSendToOctave );

	toolBar->addSeparator();

	toolBar->addAction( actionUndo );
	toolBar->addAction( actionRedo );
	toolBar->addAction( actionCut );
	toolBar->addAction( actionCopy );
	toolBar->addAction( actionPaste );

	toolBar->addSeparator();

	toolBar->addAction( actionSearch );

	toolBar->setObjectName( tr( "Editor Options" ) );
}


void Editor::createConnections()
{
	createConnectionsMenuFile();
	createConnectionsMenuEdit();
	connect( toolBar, SIGNAL( actionTriggered( QAction* ) ) , this , SLOT( toolbar_action( QAction* ) ) );
	connect( actionClone , SIGNAL( triggered() ), this , SLOT( clone_callback() ) );
}

void Editor::createConnectionsMenuFile()
{
	connect( actionPrint            , SIGNAL( triggered() ) , this , SLOT( print_callback()            ) );
	connect( actionCloseAll         , SIGNAL( triggered() ) , this , SLOT( close_editor()              ) );
	connect( actionToggleBreakPoint , SIGNAL( triggered() ) , this , SLOT( toggleBreakPoint_callback() ) );
};

Editor::~Editor()
{
	saveProject();

	if( search_dialog != NULL )
	{
		delete search_dialog;
	}
}

void Editor::show_edit_menu()
{
	menuEdit->clear();

	if( currentNtv != NULL )
	{
		menuEdit->addMenu( currentNtv->textEdit()->createStandardContextMenu() );
	}

	menuEdit->addAction( actionSearch );
}

void Editor::saveProject()
{
	QStringList files;

	for( int i = 0 ; i < tabWidget->count() ; i++ )
	{
		QString path = static_cast<NumberedTextView*>( tabWidget->widget( i ) )->path();
		
		if( ! path.isEmpty() )
		{
			files.append( path );
		}
	}

	files.removeDuplicates();
	
	if( project_name.isEmpty() )
	{
		Projects::saveListFiles( tr( "Empty" ) , files );
	}
	else
	{
		Projects::saveListFiles( project_name , files );
	}
}


// void Editor::dragEnterEvent(QDragEnterEvent *event)
// {
// 	if (event->mimeData()->hasFormat("text/plain"))
// 		event->acceptProposedAction();
// }
//
//
// void Editor::dropEvent ( QDropEvent * event )
// {
// 	QString path=event->mimeData()->text();
//
// 	openFile(path);
//
// 	event->acceptProposedAction();
// }


void Editor::setOctaveConnection( OctaveConnection *oc )
{
  octave_connection = oc;
}


void Editor::toolbar_action( QAction *action )
{
	/** New **/
	if( action == actionNew )
	{
		NumberedTextView *ntv = new NumberedTextView( this , new SimpleEditor( this ) );

		// Enable custom context menu
		ntv->textEdit()->setContextMenuPolicy( Qt::CustomContextMenu );

		const int actTabIndex =  tabWidget->addTab( ntv , tr( "New" ) );

		tabWidget->setCurrentIndex( actTabIndex );

		connectTabSignals( actTabIndex );

		currentNtv = ntv;
	}
	else if( action == actionOpen ) 	
	{
		/** Open **/
		openFile();
	}
	else if( action == actionSave )	
	{
		saveTab();
	}
	else if( action == actionSaveAs ) 	
	{
		saveAsTab();
	}
	else if( action == actionSaveAll )
	{
		saveAll();
	}
	else if( action == actionRun )
	{
		if( currentNtv->modified() ) 
		{
			saveTab();
		}

		QFileInfo finfo( currentNtv->path() );
		octave_connection->command_enter( QString("cd '") + finfo.path() + "'" , false );
		octave_connection->command_enter( QString("source (\"") +  finfo.fileName() + "\")" , false );
	}
	else if( action == actionDebug )
	{
		/** Run */
		if( currentNtv->path().isEmpty() )
		{
		  QMessageBox::critical( NULL , tr( "Error" ) , tr( "You must save the file first" ) );

		  return;
		}

		// Debug?
		if( actionStep->isEnabled() )
		{
			octave_connection->command_enter( QString( "dbcont" ) );
		}
		else
		{
			QFileInfo finfo( currentNtv->path() );
			QList<int> *breakpoints = currentNtv->getBreakpoints();

			if( breakpoints != NULL )
			{
				// Source
				//octave_connection->command_enter(QString("source('") + finfo.absoluteFilePath() + "')");

				// Clear breakpoints
				//octave_connection->command_enter(QString("dbclear('") + finfo.baseName() + "',dbstatus('"+finfo.baseName()+"') )");

				//Change to dir
				octave_connection->command_enter( "cd '" + finfo.path() + "'" );

				octave_connection->command_enter
				(
					"while (  length (dbstatus('" + finfo.baseName() + "')) >0  )"
					"dbclear('" + finfo.baseName() + "', dbstatus('" + finfo.baseName() + "')(1).line);"
					"endwhile"
				);

				// Insert breakpoints
				for( QList<int>::const_iterator i = breakpoints->constBegin(); i != breakpoints->constEnd(); i++ )
				{
					octave_connection->command_enter
					(
						QString("dbstop('") + finfo.baseName() + "','" + QString::number(*i) + "')"
					);
				}

				// Connect debug
				connect( octave_connection , SIGNAL( debug( int , int ) ) , this , SLOT( debug( int , int ) ) );
				connect( octave_connection , SIGNAL( endDebug()         ) , this , SLOT( endDebug()         ) );

				// Run
				octave_connection->command_enter(finfo.baseName());
			}
		} // End debug?
	}
	else if(action == actionUndo)
	{
		currentNtv->textEdit()->undo();
	}
	else if( action == actionRedo )
	{
		currentNtv->textEdit()->document()->redo();
	}
	else if( action == actionCut )
	{
		currentNtv->textEdit()->cut();
	}
	else if( action == actionCopy )
	{
		currentNtv->textEdit()->copy();
	}
	else if( action == actionPaste )
	{
		currentNtv->textEdit()->paste();
	}
	else if(action == actionSearch)
	{
		if( search_dialog == NULL )
		{
			search_dialog = new SearchDialog( this );

			connect( search_dialog , SIGNAL( search_signal()  ) , this , SLOT( search()  ) );
			connect( search_dialog , SIGNAL( replace_signal() ) , this , SLOT( replace() ) );
		}
		
		search_dialog->show();
	}
	else if( action == actionSendToOctave )
	{
		QTextCursor cursor = currentNtv->textEdit()->textCursor();
		
		if( cursor.hasSelection() )
		{
			octave_connection->command_enter( cursor.selectedText().replace( QChar( 0x2029 ) , '\n' ) );
		}
		else
		{
			octave_connection->command_enter( currentNtv->textEdit()->document()->toPlainText() );
		}
	}
	else if( action == actionStep )
	{
		octave_connection->command_enter( "dbstep" );
	}
	else if( action == actionClose )
	{
		closeTabs( false );
	}
	else
	{
		BOOST_ASSERT_MSG( false , "Unhanded action" );
	}
}

/*
* This function will be called if save all is pressed
*/
bool Editor::saveAll()
{
	const int currentIndex = tabWidget->currentIndex();

	for( int i = 0 ; i < tabWidget->count() ; i++ )
	{
		tabWidget->setCurrentIndex( i );
		
		if( ! saveTab() )
		{
			return false;
		}
	}

	tabWidget->setCurrentIndex( currentIndex );

	return true;
}

/*
* This function will be called if save is pressed.
* It calls save as if the path variable is empty.
*/
bool Editor::saveTab()
{
	if( ! currentNtv->path().isEmpty() )	
	{
		if( currentNtv->save() )
		{
			setTabText( tabWidget->currentIndex() , currentNtv->path().split( "/" ).last() );
			return true;
		}
		else
		{
			QMessageBox::critical( NULL , tr("Error") , tr( "Can not be saved" ) );
			return false;
		}
	}
	else
	{
		return saveAsTab();
	}
}

/* 
* This function will be called if save is pressed but no path is set
* or if save as is pressed.
*/
bool Editor::saveAsTab()
{

	QStringList filters;
	filters << "Octave (*.m; *.M)" << "Plain text (*.txt)" << "All files (*.*)";

	QFileDialog saveDialog( this , Qt::Dialog );
	
	saveDialog.setFilters( filters );
	saveDialog.setDefaultSuffix( "m" );
	saveDialog.setAcceptMode( QFileDialog::AcceptSave );

	//Use Navigator path if current path is empty
	if( currentNtv->path().isEmpty() )
	{
		QObject *obj= session->getFirstTool( NAVIGATOR );

		if( obj != NULL )
		{
			saveDialog.setDirectory( static_cast<Navigator*>( obj )->getNavigatorCurrentPath() );
		}
	}
	else
	{
		QFileInfo current_file( currentNtv->path() );
		saveDialog.setDirectory( current_file.absolutePath() );
		saveDialog.selectFile( current_file.baseName() );
	}

	if( saveDialog.exec() == QDialog::Accepted )
	{
		currentNtv->setPath( saveDialog.selectedFiles().first() );
		return saveTab();
	}

	return false;
}

void Editor::closeTab( int index )
{
	NumberedTextView *oldNtv = static_cast< NumberedTextView * >( tabWidget->widget( index ) );

	const bool modified = currentNtv->modified();
	const bool emptypath = currentNtv->path().isEmpty();
	const bool emptydoc = currentNtv->textEdit()->toPlainText().isEmpty();

	const bool test1 = modified && ( ! emptypath );
	const bool test2 = modified && ( ! emptydoc ) && emptypath;

	if( test1 || test2 )
	{
		QMessageBox msg
		(
			tr( "Close" ), 
			tr( "The document has been modified. Save changes?" ),
			QMessageBox::Question,
			QMessageBox::Yes, 
			QMessageBox::No,
			QMessageBox::Cancel | QMessageBox::Default,
			this
		);

		switch( msg.exec() )
		{
			case QMessageBox::Yes:
			{
				toolbar_action( actionSave );

				break;
			}
			case QMessageBox::No:
			{
				break;
			}
			default:
			{
				return;
			}
		}
	}

	// Disconnect the signals from the obsolete tab
	disconnectTabSignals( index );

	// If it was the last tab then a new empty tab will be generated
	if( tabWidget->count() == 1 )
	{
		toolbar_action( actionNew );
	}

	tabWidget->removeTab( index );

	delete oldNtv;

	const int actTabIndex = tabWidget->currentIndex();

	tabChanged( actTabIndex );

	updateFileList();
}

void Editor::openFile( const QString &file )
{
	/** Open **/
	QString path;

	if( file.isEmpty() )
	{
		QFileDialog openDialog(this, tr("Open") /*Qt::Dialog*/);

		QStringList filters;
		filters << "Octave (*.m; *.M)" << "Plain text (*.txt)" << "All files (*)";

		openDialog.setAcceptMode( QFileDialog::AcceptOpen );
		openDialog.setDefaultSuffix( "m" );
		openDialog.setFilters( filters );

		//openDialog.setViewMode(QFileDialog::Detail);
		QFileInfo current_file(currentNtv->path());
		openDialog.setDirectory(current_file.absolutePath());
		openDialog.selectFile(current_file.baseName());

		if( openDialog.exec() == QDialog::Accepted )
		{
			path = openDialog.selectedFiles().first();
		}
		else
		{
			return;
		}
	}
	else
	{
		path=file;
	}

	loadFiles( QStringList() << path );
}

void Editor::setProject( const QString &name )
{
	project_name = name;
	loadFiles( Projects::listFiles( project_name ) );
}

const QString Editor::getProject() const
{
	return project_name;
}

void Editor::setSession(Session *session)
{
	BaseWidget::setSession(session);
	setProject(session->getProjectName());
	connect(session, SIGNAL(projectChanged(QString)), this, SLOT(setProject(QString)) );
}

void Editor::search()
{
  QString search, replace;
  QTextCursor cursor;
  QPlainTextEdit *textEdit = currentNtv->textEdit();

  // Strings
  search = search_dialog->searchString();
  replace = search_dialog->replaceString();

  // Flags
  QTextDocument::FindFlags flags;
  if(search_dialog->caseSensitive())
    flags |= QTextDocument::FindCaseSensitively;
  if(search_dialog->wholeWords())
    flags |= QTextDocument::FindWholeWords;

  // Search
  cursor = textEdit->textCursor();
  if(search_dialog->searchStringIsRegExp())
  {
    // Search string is a regular expression
    QRegExp searchReg(search);

    cursor = textEdit->document()->find(searchReg, cursor, flags);
    //cursor = textEdit->document()->find(search, cursor, flags);
  }else{
    // Search string is not a regular expression
    cursor = textEdit->document()->find(search, cursor, flags);
  }

  textEdit->setTextCursor(cursor);
}

void Editor::replace()
{
	QTextCursor cursor = currentNtv->textEdit()->textCursor();

	if( ! cursor.selectedText().isEmpty() )
	{
		int pos = cursor.position();
		cursor.insertText( search_dialog->replaceString() );
		cursor.setPosition( pos);
		currentNtv->textEdit()->setTextCursor( cursor );
	}

	//Next line is commented because editor loose cursor
	//search();
}

void Editor::toggleBreakpoint( int lineno )
{
	currentNtv->toggleBreakpoint( lineno );
}

void Editor::debug( int lineno , int /*colno*/)
{
	currentNtv->setCurrentLine( lineno );
	actionStep->setEnabled( true );  
}

void Editor::endDebug()
{
	currentNtv->setCurrentLine( -1 );
	actionStep->setEnabled( false );

	//Clean break points
	QFileInfo finfo( currentNtv->path() );

	octave_connection->command_enter
	(
		"while (  length (dbstatus('" + finfo.baseName() + "')) >0  )"
		"dbclear('" + finfo.baseName() + "', dbstatus('" + finfo.baseName() + "')(1).line);"
		"endwhile"
	);
}

/**
*	This method will be called through 
*	a signal if the active tab is changed.
*/
void Editor::tabChanged( int tabindex )
{
	if( tabindex >= 0 )
	{
		currentNtv = static_cast< NumberedTextView * >( tabWidget->widget( tabindex ) );

		updateFileList();
		list_files->setCurrentIndex( static_cast<ListModel*>( list_files->model() )->position_index( tabindex ) );
	}
}

/* 
*	This method connects all the necessary
*	tabWidget signals to the right slots 
*/
void Editor::connectTabSignals( int tabindex )
{
	NumberedTextView *tab = static_cast< NumberedTextView * >( tabWidget->widget( tabindex ) );

	connect( tab			 , SIGNAL( textModified()								 ) , this , SLOT( textModified()								) );
	connect( tab->textEdit() , SIGNAL( dynamic_help_required( const QString & )		 ) , this , SLOT( emit_dynamic_help_required( const QString & ) ) );
	connect( tab->textEdit() , SIGNAL( customContextMenuRequested ( const QPoint & ) ) , this , SLOT( customContextMenuPopUp( const QPoint & )      ) );
}


// disconnect( oldNtv->textEdit()->document() , SIGNAL( modificationChanged( int , int , int )        ) , this , SLOT( textModified( int , int , int )               ) );

/* 
*	This method disconnects all the necessary
*	tabWidget signals to the right slots 
*/
void Editor::disconnectTabSignals( int tabindex )
{
	NumberedTextView *tab = static_cast< NumberedTextView * >( tabWidget->widget( tabindex ) );

	disconnect( tab				, SIGNAL( textModified()								) , this , SLOT( textModified()								   ) );
	disconnect( tab->textEdit() , SIGNAL( dynamic_help_required( const QString & )		) , this , SLOT( emit_dynamic_help_required( const QString & ) ) );
	disconnect( tab->textEdit() , SIGNAL( customContextMenuRequested ( const QPoint & ) ) , this , SLOT( customContextMenuPopUp( const QPoint & )      ) );
}

void Editor::textModified()
{
	for( int i = 0 ; i < tabWidget->count() ; i++ )
	{
		NumberedTextView *ntv = static_cast<NumberedTextView*>( tabWidget->widget( i ) );

		if( ( ! ntv->modified() ) || ntv == NULL )
		{
			continue;
		}

		const QString tmptext = ( ntv->path().isEmpty() ) ? tr("New") : ntv->path().split("/").last();
		setTabText( i , tmptext + "*" );
	}
}

void Editor::closeEvent( QCloseEvent * event )
{
	for( int i = 0 ; i < tabWidget->count() ; i++ )
	{
		if( static_cast<NumberedTextView*>( tabWidget->widget( i ) )->modified() )
		{
			const QString caption = tr( "Close this window?" );
			const QString message = tr( "You are going to close Editor. Are you sure?" );

			if( QMessageBox::warning (this , caption , message , QMessageBox::Ok , QMessageBox::Cancel ) != QMessageBox::Ok )
			{
				event->ignore();
				return;
			}
		}
	}

	event->accept();
}

void Editor::closeTabs( bool close_all_tabs )
{
	while( tabWidget->count() > 0 )
	{
		if( currentNtv == NULL )
		{
			printf("[Editor::closeTabs] currentNtv==NULL\n");
			break;
		}

		const bool modified = currentNtv->modified();
		const bool emptypath = currentNtv->path().isEmpty();

		const bool test1 = modified && ! currentNtv->textEdit()->toPlainText().isEmpty() && emptypath;
		const bool test2 = modified && ! emptypath;

		if( test1 || test2 )
		{
			QMessageBox msg
			(
				tr( "Close" ), 
				tr( "The file has been modified. Save changes?" ),
				QMessageBox::Question,
				QMessageBox::Yes, 
				QMessageBox::No,
				QMessageBox::Cancel | QMessageBox::Default,
				this
			);

			switch(msg.exec())
			{
				case QMessageBox::Yes:
					toolbar_action(actionSave);
					break;
				case QMessageBox::No:
					// No hacer nada
					break;
				default:
					return;
			}
		}

		// Borrar
		//tabWidget->removeTab(tabWidget->currentIndex());
		disconnect( currentNtv , SIGNAL( textModified() ) , this , SLOT( textModified() ) );
		NumberedTextView *ntv=currentNtv;
		currentNtv=NULL;
		//TODO: Check if another view (clone view) is using the same document object
		printf("Padre %d ; textEdit %d\n", ntv->textEdit()->document()->parent(), ntv->textEdit() );
		//if(ntv->textEdit()->document()->parent()==ntv->textEdit())
		/*
		{
			for(int i=0;i<tabWidget->count();i++)
			{
				NumberedTextView *w=(NumberedTextView *)tabWidget->widget(i);
				if(
					w!=ntv 
					&& 
					ntv->textEdit()->document()==w->textEdit()->document()
				)
				{
					printf("Padre %d ; textEdit %d\n", w->textEdit()->document()->parent(), w->textEdit() );
					ntv->textEdit()->document()->setParent(w->textEdit());
					w->textEdit()->setDocument(ntv->textEdit()->document());
					break;
				}
			}
		}
		*/
		delete ntv;

		if( ! close_all_tabs )
		{
			break;
		}
	}

	// Add the first tab at start
	if( tabWidget->count() == 0 )
	{
		toolbar_action( actionNew );
	}
	else
	{
		tabChanged( tabWidget->currentIndex() );
	}

	updateFileList();
}

void Editor::newEditorTab()
{
	toolbar_action( actionNew );
	updateFileList();
}

void Editor::loadFiles( const QStringList &files )
{
	QString path;

	foreach( path , files )
	{
		if( path.isEmpty() ) continue;
		
		try
		{
			// Si la pestaña activa no contiene ningún texto ni es un archivo,
			// abrir en ella
			
			if(!currentNtv->path().isEmpty() || !currentNtv->textEdit()->document()->isEmpty())
			{
				newEditorTab();
			}
			
			SimpleEditor *codeEdit = static_cast<SimpleEditor*>(currentNtv->textEdit());
			disconnect(codeEdit->document(), SIGNAL(modificationChanged (bool)), this, SLOT(textModified(bool)));
			//disconnect(currentNtv->textEdit()->document(), SIGNAL(modificationChanged (bool)), this, SLOT(textModified(bool)));

			
			currentNtv->open(path);
				

			//connect(currentNtv->textEdit()->document(), SIGNAL(modificationChanged (bool)), this, SLOT(textModified(bool)));
			connect(codeEdit->document(), SIGNAL(modificationChanged (bool)), this, SLOT(textModified(bool)));
			
			setTabText(tabWidget->currentIndex(), path.split("/").last());
		}
		catch( ... )
		{
			QMessageBox::critical(NULL, tr("Error"), path + " can not be opened");
		}
	}
}

void Editor::close_editor()
{
	if( parent() != NULL )
	{
		static_cast<QWidget*>( parent() )->close();
	}
	else
	{
		close();
	}
}

void Editor::emit_dynamic_help_required( const QString &text )
{
	//printf("%s\n", text.toLocal8Bit().data());
	emit dynamic_help_required(text);
}

BaseWidget *Editor::copyBaseWidget( QWidget * parent )
{
	saveProject();

	Editor *bw = new Editor( parent );

	bw->setSession( session );
	bw->octave_connection = octave_connection;

	for( int i = 0 ; i < tabWidget->count() ; i++ )
	{
		NumberedTextView *code = static_cast< NumberedTextView * >( tabWidget->widget( i ) );

		if( i )
		{
			bw->toolbar_action( bw->actionNew );
		}

		bw->currentNtv->setPath( code->path() );
		bw->currentNtv->setModified( code->modified() );
		bw->currentNtv->textEdit()->setPlainText( code->textEdit()->toPlainText() );

		code->setModified( false );

		if( ! code->path().isEmpty() )
		{
			bw->setTabText( bw->tabWidget->currentIndex() , code->path().split("/").last() );
		}
	}

	return bw;
}

void Editor::setTabText( int index , const QString & label )
{
	tabWidget->setTabText( index , label );

	updateFileList();
}

void Editor::updateFileList()
{
	ListModel *model = static_cast<ListModel*>( list_files->model() );
	model->clear();

	for( int i=0 ; i < tabWidget->count() ; i++ )
	{
		model->append( tabWidget->tabText( i ) , i );
	}

	model->update();
}

void Editor::file_selected( const QModelIndex & index )
{
	ListModel *model = static_cast<ListModel*>( list_files->model() );
	tabWidget->setCurrentIndex(model->position(index));
}


void Editor::indent_callback()
{
	currentNtv->indent();
}

void Editor::unindent_callback()
{
	currentNtv->unindent();
}

void Editor::comment_callback()
{
	currentNtv->comment();
}

void Editor::uncomment_callback()
{
	currentNtv->uncomment();
}

void Editor::simple_rcs_callback()
{
	QString path=currentNtv->path();

	QString repository=path+"~~";
	QString command("simplercs \""+repository+"\" ");
	QProcess::startDetached(command);
	//QProcess::execute(command);
	printf("[NumberedTextView::save] Comando: %s\n", command.toLocal8Bit().data() );
}


void Editor::print_callback()
{
	QPrinter printer;

	QPrintDialog *dialog = new QPrintDialog(&printer, this);
	dialog->setWindowTitle(tr("Print Document"));

	if (currentNtv->textEdit()->textCursor().hasSelection())
		dialog->addEnabledOption(QAbstractPrintDialog::PrintSelection);

	if (dialog->exec() == QDialog::Accepted)
	{
		currentNtv->textEdit()->print(&printer);
	}
}


void Editor::clone_callback()
{
	QTextDocument *document=currentNtv->textEdit()->document();
	QString path=currentNtv->path();
	
	newEditorTab();
	
	currentNtv->textEdit()->setDocument(document);

	if( ! path.isEmpty() )
	{
		setTabText(tabWidget->currentIndex(), path.split("/").last());
		currentNtv->setPath(path);
	}
}

void Editor::toggleBreakPoint_callback()
{
	const int lineno = currentNtv->textEdit()->textCursor().blockNumber()+1;
	currentNtv->toggleBreakpoint( lineno );
}

void Editor::clipboard_double_clicked(const QModelIndex &index)
{
	QString text=index.data().toString();
	currentNtv->textEdit()->textCursor().insertText(text);
}

ListModel::ListModel( QObject *parent , Editor *newEditor ) : 
QAbstractListModel(parent),
editor(newEditor)
{

}

int ListModel::rowCount(const QModelIndex & /* ATM unused */ ) const
{
	return list.size();
}

QVariant ListModel::data( const QModelIndex &index, int role ) const
{
	if ( ! index.isValid() || index.row() >= list.size() )
	{
		return QVariant();
	}

	if ( role == Qt::DisplayRole )
	{
		return list.at( index.row() ).name;
	}

	return QVariant();
}

void ListModel::clear()
{
	list.clear();
}

void ListModel::append( const QString &name , int position )
{
	ListItem item;
	item.name=name;
	item.position=position;
	list.append(item);
}

int ListModel::position( const QModelIndex &index )
{
	return list.at( index.row() ).position;
}

void ListModel::update()
{
	//printf("[ListModel::update] %d Inicio\n",list.size());
	QModelIndex index0 = index(0);
	beginInsertRows(index0, 0, list.size()-1);
	//printf("[ListModel::update] %d Proceso\n",list.size());
	endInsertRows();
	//printf("[ListModel::update] %d Fin\n",list.size());
}

QModelIndex ListModel::position_index( int position )
{
	for( int i=0 ; i < list.size() ; i++ )
	{
		if( list[i].position == position )
		{
			return index( i , 0 );
		}
	}

	return QModelIndex();
}

Qt::DropActions ListModel::supportedDropActions() const
{
	return Qt::CopyAction;
}

bool ListModel::dropMimeData(const QMimeData *data, Qt::DropAction /* ATM unused */ , int /* ATM unused */ , int /* ATM unused */ , const QModelIndex & /* ATM unused */ )
{
	if (data->hasFormat("text/plain"))
	{
		QString path=data->text();
		editor->openFile(path);

		return true;
	}

	return false;
}

Qt::ItemFlags ListModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags defaultFlags = QAbstractListModel::flags(index);

	return Qt::ItemIsDropEnabled | defaultFlags;
}


QStringList ListModel::mimeTypes() const
{
	QStringList types;
	types << "text/plain";
	return types;
}

ClipboardListView::ClipboardListView( QWidget *parent ):
QListView(parent),
_stringModel( new QStringListModel( parent ) ),
popup( new QMenu( parent ) )
{
	setModel(_stringModel);

	QAction *edit=popup->addAction(tr("Edit"));
	popup->addSeparator();
	QAction *remove=popup->addAction(tr("Delete entry"));
	QAction *remove_all=popup->addAction(tr("Delete all"));
	popup->addSeparator();
	QAction *sort_ascending=popup->addAction(tr("Sort ascending"));
	QAction *sort_descending=popup->addAction(tr("Sort descending"));
	popup->addSeparator();
	QAction *up=popup->addAction(tr("Up"));
	QAction *down=popup->addAction(tr("Down"));
	QAction *top=popup->addAction(tr("Top"));
	QAction *bottom=popup->addAction(tr("Bottom"));
	popup->addSeparator();
	stopAction=popup->addAction(tr("Stop append"));
	stopAction->setCheckable(true);
	stopAction->setChecked(false);
	stopAction->setToolTip(tr("Stops append text from clipboard"));

	connect(edit, SIGNAL(triggered()),this, SLOT(edit_callback()));
	connect(remove, SIGNAL(triggered()),this, SLOT(remove_callback()));
	connect(remove_all, SIGNAL(triggered()),this, SLOT(remove_all_callback()));
	connect(sort_ascending, SIGNAL(triggered()),this, SLOT(sort_ascending_callback()));
	connect(sort_descending, SIGNAL(triggered()),this, SLOT(sort_descending_callback()));
	connect(up, SIGNAL(triggered()),this, SLOT(up_callback()));
	connect(down, SIGNAL(triggered()),this, SLOT(down_callback()));
	connect(top, SIGNAL(triggered()),this, SLOT(top_callback()));
	connect(bottom, SIGNAL(triggered()),this, SLOT(bottom_callback()));

	connect(this, SIGNAL(clicked (const QModelIndex &)), this, SLOT(clipboard_selected(const QModelIndex &)) );
	connect(QApplication::clipboard(),SIGNAL(dataChanged()), this, SLOT(clipboard_new_data_callback()) );

	//Load last state SmallClipboard
	QFile inFile(configPath()+"SmallClipboard.xml");
	
	if( inFile.exists() )
	{
		inFile.open(QIODevice::ReadOnly);
		QXmlStreamReader in(&inFile);
		loadStateXML(in);
		inFile.close();
	}

	setToolTip
	(
		"<b>Small Clipboard:</b>"
		"<p>Small Clipboard is a list of clipboard texts. When you copy some text using ctrl+C, this text is copied to system clipboard. If text is small, it will be listed in Small Clipboard. Whichever the text selected from Small Clipboard list, you can paste it pressing ctrl+V. You can paste text with double click, also.</p>"
		"Use right click to see popup menu."
	);
}

ClipboardListView::~ClipboardListView()
{
	QFile outFile( configPath() + "SmallClipboard.xml" );
	
	if( outFile.open( QIODevice::WriteOnly ) )
	{
		saveStateXML( QXmlStreamWriter( &outFile ) );

		outFile.close();
	}
	else
	{
		BOOST_ASSERT_MSG( false , "Could not open file to write xml settings." );
	}
}

void ClipboardListView::contextMenuEvent( QContextMenuEvent * event )
{
	popup->popup( event->globalPos() );
}

QStringListModel *ClipboardListView::stringModel()
{
	return _stringModel;
}

void ClipboardListView::edit_callback()
{
	QModelIndexList indexes=selectedIndexes();
	
	for(int i=0; i<indexes.size(); i++)
	{
		QModelIndex index=indexes[i];
		edit(index);
	}
}

void ClipboardListView::remove_callback()
{
	QModelIndexList indexes=selectedIndexes();
	
	QStringList list=_stringModel->stringList();
	
	for(int i=0; i<indexes.size(); i++)
	{
		QModelIndex index=indexes[i];
		QString text=index.data().toString();
		int k=list.indexOf(text);
		list.removeAt(k);
	}
	
	_stringModel->setStringList(list);
}

void ClipboardListView::remove_all_callback()
{
	QStringList list;
	_stringModel->setStringList(list);
}

void ClipboardListView::sort_ascending_callback()
{
	QStringList list=_stringModel->stringList();
	list.sort();
	_stringModel->setStringList(list);
}

void ClipboardListView::sort_descending_callback()
{
	QStringList list=_stringModel->stringList();
	list.sort();
	for(int i=0; i<list.size()/2; i++)
	{
		list.swap(i, list.size()-1-i);
	}
	_stringModel->setStringList(list);
}

void ClipboardListView::up_callback()
{
	QModelIndexList indexes=selectedIndexes();
	QStringList list=_stringModel->stringList();
	for(int i=0; i<indexes.size(); i++)
	{
		QModelIndex index=indexes[i];
		QString text=index.data().toString();
		int k=list.indexOf(text);
		if(k>0) list.swap(k, k-1);
	}
	_stringModel->setStringList(list);
}

void ClipboardListView::down_callback()
{
	QModelIndexList indexes=selectedIndexes();
	QStringList list=_stringModel->stringList();
	for(int i=0; i<indexes.size(); i++)
	{
		QModelIndex index=indexes[i];
		QString text=index.data().toString();
		int k=list.indexOf(text);
		if(k<list.size()-1) list.swap(k, k+1);
	}
	_stringModel->setStringList(list);
}

void ClipboardListView::top_callback()
{
	QModelIndexList indexes=selectedIndexes();
	QStringList list=_stringModel->stringList();
	for(int i=0; i<indexes.size(); i++)
	{
		QModelIndex index=indexes[i];
		QString text=index.data().toString();
		int k=list.indexOf(text);
		list.removeAt(k);
		list.prepend(text);
	}
	_stringModel->setStringList(list);
}

void ClipboardListView::bottom_callback()
{
	QModelIndexList indexes=selectedIndexes();
	QStringList list=_stringModel->stringList();
	for(int i=0; i<indexes.size(); i++)
	{
		QModelIndex index=indexes[i];
		QString text=index.data().toString();
		int k=list.indexOf(text);
		list.removeAt(k);
		list << text;
	}
	_stringModel->setStringList(list);
}

void ClipboardListView::clipboard_new_data_callback()
{
	if( stopAction->isChecked() ) return;

	QClipboard *clipboard = QApplication::clipboard();

	const QMimeData *mimedata = clipboard->mimeData();
	if(mimedata->hasText())
	{
		QStringList list=_stringModel->stringList();
		while(list.size()>100)
		{
			list.removeFirst();
		}

		QString text=clipboard->text();
		if( text.indexOf('\n')<0 && text.size()<256 && !list.contains(text) )
		{
			list << text;
			_stringModel->setStringList(list);
		}
	}
}

void ClipboardListView::clipboard_selected(const QModelIndex &index)
{
	QString text=index.data().toString();
	QClipboard *clipboard = QApplication::clipboard();
	clipboard->setText(text);
}

void ClipboardListView::saveStateXML(QXmlStreamWriter &out, bool partOk)
{
	if( ! partOk )
	{
		out.writeStartDocument();
	}

	out.writeStartElement("ClipboardListView");
	out.writeStartElement("stopAction");

	if( stopAction->isChecked() )
	{
		out.writeAttribute("value", "true");
	}
	else
	{
		out.writeAttribute("value", "false");
	}

	out.writeEndElement();

	QStringList list=_stringModel->stringList();

	for(int i=0;i<list.size();i++)
	{
		out.writeStartElement("entry");
		out.writeCharacters(list[i]);
		out.writeEndElement();
	}

	out.writeEndElement();

	if( ! partOk )
	{
		out.writeEndDocument();
	}
}

void ClipboardListView::loadStateXML(QXmlStreamReader &in, bool /* ATM unused */ )
{
	bool insideOk=false;
	QStringList list;

	while( !in.atEnd() )
	{
		in.readNext();
		if( in.isStartElement() )
		{
			QStringRef name=in.qualifiedName();
			if(insideOk && name=="entry")
			{
				QString entry=in.readElementText();
				list << entry;
			}
			else if(insideOk && name=="stopAction")
			{
				if( "true"==in.attributes().value("value") )
					stopAction->setChecked(true);
				else
					stopAction->setChecked(false);
			}
			else if(name=="ClipboardListView")
				insideOk=true;
		}
		else if( in.isEndElement() )
		{
			QStringRef name=in.qualifiedName();
			if( name=="ClipboardListView" && insideOk )
				break;
		}
	}

	_stringModel->setStringList(list);
}
