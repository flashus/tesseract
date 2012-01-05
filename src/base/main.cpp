/* Copyright (C) 2006,2007,2008 P.L. Lucas
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

#include <iostream>

#include <QMdiArea>
#include <QTextCodec>
#include <QFileDialog>
#include <QTranslator>
#include <QInputDialog>
#include <QLibraryInfo>
#include <QMdiSubWindow>

#include "main.hpp"
#include "help.hpp"
#include "table.hpp"
#include "splash.hpp"
#include "editor.hpp"
#include "terminal.hpp"
#include "projects.hpp"
#include "svgcanvas.hpp"
#include "basewidget.hpp"
#include "mainwindow.hpp"
#include "operations.hpp"
#include "window_list.hpp"
#include "generate_menu.hpp"
#include "variables_list.hpp"
#include "octave_connection.hpp"

extern QString syntaxPath();

Main::Main( QObject * parent ) : 
QObject( parent ),
oc( shared_ptr<OctaveConnection>( new OctaveConnection() ) )
{
	//Build Octave commands list
	{
		QString oc;

		if( getConfig( "octave_path" ).isEmpty() )
		{
			oc = "octave";
		}
		else
		{
			oc = getConfig( "octave_path" );
		}

		QString command =

		"l=completion_matches('');"
		"[nrows,ncols]=size(l);"

		"out=fopen('"+syntaxPath()+"commands.txt"+"','w');"

		"for k=1:nrows;"
		"fprintf(out,'%s\\n',l(k,:));"
		"endfor;"

		"fclose(out);";

		std::cout << "[Main::Main] Building commands list.\n";

		system( QString( oc + " --no-history -q  --eval \"" + command + "\"").toLocal8Bit().data() );

		std::cout << "[Main::Main] Commands list builded.\n";
	}

	QString session_name = getConfig( "session_name" );

	if( ! session_name.isEmpty() && session_name != "Empty" )
	{
		session.setProjectName( session_name );
	}

	session.addTool( MAIN , this );

	main_window = new MainWindow( oc.get() , &session );

	work_space = main_window->work_space;

	window_list = NULL;

	if( getConfig( "octave_path" ).isEmpty() )
	{
		oc->setOctavePath( "octave" );
	}
	else
	{
		oc->setOctavePath( getConfig( "octave_path" ) );
	}

	terminal = static_cast< Terminal * >( createTool( TERMINAL , work_space ) );
	terminal->work_space = work_space;
	terminal->setOctaveConnection( oc.get() );

	oc->startOctave();

	terminal->setProject();

	//Show list of windows in workspace
	window_list = new WindowList( work_space , main_window->toolBarDocks );
	main_window->toolBarDocks->addWidget( window_list );
	window_list->setSizePolicy( QSizePolicy( QSizePolicy::Expanding , QSizePolicy::Preferred ) );
	window_list->show();
	main_window->showMaximized();

	{//Open tools from config
		QString modeWorkArea=getConfig("mode_work_area");

		QFile file(projectsPath()+"last_windows_layout.xml");
		QFile wl_file(configPath()+"windows_layout.xml");

		QXmlStreamReader xml;

		if( modeWorkArea=="last" && file.exists() )
		{
			file.open(QIODevice::ReadOnly | QIODevice::Text);
			xml.setDevice(&file);
			modeWorkArea.clear();
		}
		else if( ! modeWorkArea.isEmpty() && wl_file.exists() )
		{
			wl_file.open(QIODevice::ReadOnly | QIODevice::Text);
			xml.setDevice(&wl_file);
		}
		else
		{
			QString xmlConfig=
			"<tools_config>"
				"<tool type='main_window'>"
					"<tool type='terminal' place='workspace'>"
						//"<tool type='table'>"
						//	"<matrix value='a'/>"
						//"</tool>"
					"</tool>"
					"<tool type='variables_list'/>"
					"<tool type='command_list'/>"
					"<tool type='navigator'/>"
				"</tool>"
				"<tool type='editor' place='window'>"
					//"<tool type='help' title='Octave Help'/>"
					//"<tool type='dynamic_help'/>"
				"</tool>"
			"</tools_config>";

			xml.addData( xmlConfig );
			modeWorkArea.clear();
		}

		openTools( xml , modeWorkArea );

		file.close();
		wl_file.close();

		QMap<QString, QString> windowSettings;
		windowSettings["mode_work_area"]="last";
		setConfig( windowSettings );
	}

	connect(oc.get(), SIGNAL(clearScreen()), this, SLOT(clear_terminal()));


	connect(main_window->actionCompletionMatches, SIGNAL(triggered()), terminal , SLOT(completion_matches_callback()));
	connect(main_window->actionDynamicHelp, SIGNAL(triggered()), this, SLOT(dynamic_help()));
	connect(main_window->actionStopProcess, SIGNAL(triggered()), terminal , SLOT(stop_process_callback()));
	connect(main_window->actionClearTerminal, SIGNAL(triggered()), terminal , SLOT(clear_callback()));

	operations = new Operations( this , &active_widget , main_window );
	operations->setOctaveConnection( oc.get() );
	operations->setSession( &session );

	//Build menus from files

	GenerateMenu generate_menu( main_window , operations );
	generate_menu.setPath( QApplication::applicationDirPath() + QDir::separator() + "menus" );
	generate_menu.load_menu();

	generate_menu.setPath( QString( projectsPath() + "menus" ) );
	generate_menu.load_menu();

	//generate_menu.setPath("./menus");
	//generate_menu.load_menu();

	main_window->show_config_help_menus();

	connect(main_window->actionOctave_help, SIGNAL(triggered()), this, SLOT(help()));
	connect(main_window->actions.value("qtoctave_help"),  SIGNAL(triggered()), this, SLOT(help_qtoctave()));
	connect(main_window->actions.value("qtoctave_about"),  SIGNAL(triggered()), this, SLOT(about()));
	connect(main_window->actionTable, SIGNAL(triggered()), this, SLOT(table()));

	connect(main_window->actionVariableList, SIGNAL(triggered()), this, SLOT(variable_list()));

	//connect the Navigator
	connect(main_window->actionNavigator, SIGNAL(triggered()), this, SLOT(setVisibleNavigator()));

	connect(main_window->actionRunFile, SIGNAL(triggered()), this, SLOT(run_file()));

	connect(main_window->actionEditor, SIGNAL(triggered()), this, SLOT(editor_callback()));

	connect(main_window->actions["actionCommandList"], SIGNAL(triggered()), this, SLOT(commands_list()));

	connect(main_window->actionSvgCanvas, SIGNAL(triggered()), this, SLOT(svgcanvas_callback()));

	if( oc != NULL )
	{
		connect(oc.get(), SIGNAL(line_ready(QString)), this, SLOT(line_ready(QString)));
	}

	//main_window->showMaximized();
}

Terminal * Main::getTerminal()
{
	return terminal;
}

void Main::widget_activated( BaseWidget *w )
{
	active_widget = w;
}

void Main::line_ready( const QString &line )
{
	//Builds SvgCanvas if it's needed.
	QRegExp re("~~svgcanvas: *(\\d+) +(.+)\n");

	if( re.exactMatch( line ) )
	{
		const int number = re.cap(1).toInt();

		QVector<QObject*> tools=session.getTools(SVGCANVAS);

		for(int i=0;i<tools.size();i++)
		{
			if( static_cast<SvgCanvas*>( tools[i] )->getCanvasNumber() == number ) 
			{
				return;
			}
		}

		//SvgCanvas needed
		SvgCanvas *svgcanvas = static_cast<SvgCanvas*>( createTool( SVGCANVAS , work_space ) );

		svgcanvas->show();
		svgcanvas->setCanvasNumber( number );
		svgcanvas->line_ready( line );
	}
}

void Main::help()
{
	if( getConfig( "qtinfo_ok" ).isEmpty() || getConfig( "qtinfo_ok" ) == "false" )
	{
		 oc->command_enter( "qtinfo" );
	}
	else
	{
		Help *help = static_cast<Help*>( createTool( HELP , work_space ) );
	
		if( getConfig( "help_path" ).isEmpty() )
		{
			help->setSource( helpSource() );
		}
		else 
		{
			help->setSource( getConfig( "help_path" ) );
		}

		help->show();
	}
}

void Main::about()
{
	Help *help = static_cast< Help * >( createTool( HELP , work_space ) );

	QFileInfo path( helpPath() );

	if( getConfig( "qtoctave_help_path" ).isEmpty() )
	{
		help->setSource( helpPath() + "about.html" );
	}
	else
	{
		help->setSource( getConfig( "qtoctave_help_path" ) , "about" );
	}

	help->setWindowTitle( "Tesseract About" );
	help->show();
}

void Main::table( QString text )
{
	bool ok = true;

	if( text.isEmpty() )
	{
		text = QInputDialog::getText( main_window , tr( "Select table" ) , tr( "Matrix name:" ) , QLineEdit::Normal, "", &ok );
	}

	if ( ok && ( ! text.isEmpty() ) )
	{
		Table *table = static_cast<Table*>( createTool( TABLE , work_space ) );

		table->setMatrix( text );

		table->show();
		table->windowActivated();
	}
}

void Main::run_file()
{
	QFileDialog openDialog( NULL , tr( "Open" ) , "." );
	QStringList filters;

	filters << "Octave (*.m; *.M)";

	openDialog.setFilters( filters );
	openDialog.setAcceptMode( QFileDialog::AcceptOpen );
	openDialog.setDefaultSuffix( "m" );

	if( openDialog.exec() == QDialog::Accepted )
	{
		QFileInfo fileInfo( openDialog.selectedFiles().first() );
		QString cmd;
		//OctaveConnection *oc = terminal->getOctaveConnection();

		// Change dir
		cmd = QString( "cd \"" ) + fileInfo.dir().absolutePath() + QString( "\"" );
		oc->command_enter( cmd );

		// Execute file
		cmd = fileInfo.baseName();
		oc->command_enter( cmd );
	}
}

void Main::variable_list()
{
	VariableList *variableList = static_cast<VariableList*>( createTool( VARIABLESLIST , work_space ) );
	variableList->show();
}

void Main::dynamic_help()
{
	DynamicHelp *dynamic_help = static_cast<DynamicHelp*>( createTool( DYNAMIC_HELP , work_space ) );
	dynamic_help->show();
}

void Main::commands_list()
{
	CommandList *command_list = static_cast<CommandList*>( createTool( COMMANDLIST , work_space) );
	command_list->show();
}

void Main::editor_callback()
{
	if( getConfig( "external_editor" ) != "true" )
	{
		Editor *editor = static_cast<Editor*>( createTool( EDITOR , work_space ) );
		editor->show();
	}
	else
	{
		const QString editor = getConfig( "editor" );

		if( editor.isEmpty() )
		{
			return;
		}

		QString ed( editor );

		QProcess::startDetached( ed );
	}
}

void Main::svgcanvas_callback()
{
	SvgCanvas *svgcanvas = static_cast<SvgCanvas*>( createTool( SVGCANVAS , work_space ) );
	svgcanvas->show();
}

/** set visible or not the dock of var list*/
void Main::setVisibleVarList()
{
	main_window->dockListVar->setVisible( ! main_window->dockListVar->isVisible() );
}

/**this function show or not the Navigator dock*/
void Main::setVisibleNavigator()
{
	Navigator *nav = static_cast<Navigator*>( createTool( NAVIGATOR , work_space ) );
	nav->show();
}

QWidget *Main::mainWindowWidget()
{
	return main_window;
}

void Main::clear_terminal()
{
	Terminal *terminal = static_cast<Terminal*>( session.getFirstTool( TERMINAL ) );
	terminal->clear_callback();
}

BaseWidget *Main::createTool( WidgetType type , QWidget *parent )
{
	BaseWidget *w;

	switch( type )
	{
		case TERMINAL:
		{
			Terminal *terminal = new Terminal();
			terminal->setSession( &session );
			w = terminal;
			break;
		}

		case HELP:
		{
			Help *help = new Help( parent );
			help->setSession( &session );
			w = help;
			break;
		}

		case TABLE:
		{
			Table *table = new Table( parent );
			table->setOctaveConnection( oc.get() );
			table->setSession( &session );
			w = table;
			break;
		}

		case VARIABLESLIST:
		{
			VariableList *variableList = new VariableList( parent );
			variableList->setOctaveConnection( oc.get() );
			connect( variableList , SIGNAL( open_table( QString ) ) , this , SLOT( table( QString ) ) );
			variableList->setSession( &session );
			variableList->send_whos_command_to_octave();
			w = variableList;
			break;
		}

		case DYNAMIC_HELP:
		{
			DynamicHelp *dynamic_help = new DynamicHelp( oc->getOctavePath() , parent );
			dynamic_help->setSession( &session );
			w = dynamic_help;
			break;
		}

		case EDITOR:
		{
			//if( get_config("external_editor") != "true")
			//{
				Editor *editor = new Editor( parent );
				editor->setSession( &session );
				editor->setOctaveConnection( oc.get() );
				w = editor;
			//}
			break;
		}

		case NAVIGATOR:
		{
			Navigator *nav = new Navigator( parent );
			nav->setOctaveConnection( oc.get() );
			nav->setSession( &session );
			w = nav;
			break;
		}

		case COMMANDLIST:
		{
			CommandList *command_list = new CommandList( parent );
			Autocomplete *line = static_cast<Terminal*>( session.getFirstTool( TERMINAL ) )->getAutocomplete();
			
			connect( line , SIGNAL( new_command_entered( QStringList ) ) , command_list , SLOT( set_list( QStringList ) ) );

			command_list->setLineEdit( line );
			command_list->setSession( &session );
			command_list->set_list( line->commands() );

			w = command_list;

			break;
		}
		case SVGCANVAS:
		{
			SvgCanvas *svgcanvas = new SvgCanvas( parent );

			svgcanvas->setSession( &session );
			svgcanvas->setOctaveConnection( oc.get() );

			QVector< QObject *> tools = session.getTools( SVGCANVAS );
			
			int min = 1;

			for(int i = 0 ; i < tools.size() ; i++ )
			{
				for( int j = 0 ; j < tools.size() ; j++ )
				{
					if( static_cast< SvgCanvas *>( tools[j] )->getCanvasNumber() == min )
					{
						min++;
					}
				}
			}

			svgcanvas->setCanvasNumber( min );
			w = svgcanvas;

			break;
		}
		default:
		{
			return NULL;
		}
	}

	if( parent == work_space )
	{//Add window in workspace
		QMdiSubWindow *mdi = work_space->addSubWindow( w );
		mdi->setAttribute( Qt::WA_DeleteOnClose );
		mdi->setWindowIcon( w->windowIcon() );
	}

	return w;
}

void Main::openTools( QXmlStreamReader &xml , const QString &config_name )
{
	BaseWidget *bw;
	QWidget *parent;
	QList<BaseWidget*> tools;
	QHash<QString, WidgetType> tools_type;

	tools_type["help"] = HELP;
	tools_type["table"] = TABLE;
	tools_type["editor"] = EDITOR;
	tools_type["terminal"] = TERMINAL;
	tools_type["svgcanvas"] = SVGCANVAS;
	tools_type["navigator"] = NAVIGATOR;
	tools_type["main_window"] = MAINWINDOW;
	tools_type["command_list"] = COMMANDLIST;
	tools_type["dynamic_help"] = DYNAMIC_HELP;
	tools_type["variables_list"] = VARIABLESLIST;

	while ( ! xml.atEnd() )
	{
		xml.readNext();

		if( xml.isStartElement() )
		{
			if( xml.name() == "tool" )
			{
				QXmlStreamAttributes attr = xml.attributes();

				QString type = attr.value( "type" ).toString();

				if( ! tools_type.contains( type ) )
				{
					printf("Type %s isn't in database\n", type.toLocal8Bit().data());
					return;
				}

				QString place=attr.value("place").toString();

				if( place == "window" ) 
				{
					parent = NULL;
				}
				else
				{
					parent = work_space; // place=="workspace"
				}

				if( type == "terminal" )
				{
					bw = static_cast<BaseWidget*>( session.getFirstTool( TERMINAL ) );
				}
				else if( type == "main_window" )
				{
					bw = static_cast<BaseWidget*>( session.getFirstTool( MAINWINDOW ) );
				}
				else if( tools.isEmpty() )
				{
					bw = createTool( tools_type[type] , parent );
				}
				else
				{
					bw = createTool( tools_type[type] , tools.last() );
				}

				QString title = attr.value( "title" ).toString();
				
				if( ! title.isEmpty() )
				{
					bw->setWindowTitle( title );
				}
				
				if( place != "window" ) //place=="workspace"
				{
					std::size_t y=0;
					std::size_t x=0;
					std::size_t width=0;
					std::size_t height=0;

					if( attr.hasAttribute("xPosition") )
					{
						x=attr.value("xPosition").toString().toInt();
					}

					if( attr.hasAttribute("yPosition") )
					{
						y=attr.value("yPosition").toString().toInt();
					}

					if( attr.hasAttribute("width") )
					{
						width=attr.value("width").toString().toInt();
					}

					if( attr.hasAttribute("height") )
					{
						height=attr.value("height").toString().toInt();
					}

					bool maximized=false;

					if( attr.hasAttribute("maximized") )
					{
						maximized=(attr.value("maximized").toString()=="true");
					}

					bool minimized=false;
					
					if( attr.hasAttribute("minimized") )
					{
						minimized=(attr.value("minimized").toString()=="true");
					}

					if(bw!=NULL)
					{
						InitialPosition initPos;
						initPos.x=x;
						initPos.y=y;

						if(width<=0)
						{
							//width=100;
							maximized = true;
						}

						if(height<=0) 
						{
							//height=100;
							maximized = true;
						}

						initPos.width=width;
						initPos.height=height;

						initPos.maximized=maximized;
						initPos.minimized=minimized;

						initPos.widget=bw->parentWidget();

						if(initPos.widget!=NULL)
						{
							initialPositionList.append(initPos);
						}
					}
				}

				tools.append( bw );

				switch( bw->widgetType() )
				{
					case HELP:
					{
						Help *help = static_cast< Help * >( bw );
						if(getConfig("help_path").isEmpty()) help->setSource( helpSource() );
						else help->setSource(getConfig("help_path"));

						break;
					}
					default:
					{
						break;
					}
				}
			}
			else if( ! tools.isEmpty() )
			{
				QXmlStreamAttributes attr=xml.attributes();
				switch( tools.last()->widgetType() )
				{
					case TABLE:
					{
						Table* table=(Table*)tools.last();
						if(xml.name()=="matrix")
						{
							QString value=attr.value("value").toString();
							table->setMatrix(value);
							table->windowActivated();
						}
						break;
					}
					default:
					;
				}

				if(xml.name()=="state")
				{
					QString state=xml.readElementText();
					tools.last()->restoreState( QByteArray::fromHex(state.toAscii()) );
				}
			}
			else if(xml.name()=="tools_config" && !config_name.isEmpty() )
			{
				QXmlStreamAttributes attr=xml.attributes();

				QString name=attr.value("name").toString();

				if(name!=config_name)
				{
					while (!xml.atEnd())
					{
						xml.readNext();
						if( xml.isStartElement() )
						{
							if(xml.name()=="tools_config")
							{
								QXmlStreamAttributes attr=xml.attributes();

								QString name=attr.value("name").toString();

								if(name==config_name) break;
							}
						}
					}
				}
			}
		}
		else if( xml.isEndElement() )
		{
			if( xml.name()=="tool" /*&& !tools.isEmpty()*/ )
			{
				BaseWidget *bw=tools.last();
				tools.removeLast();
				if(!tools.isEmpty() && bw->widgetType()!=TERMINAL && bw->widgetType()!=MAINWINDOW)
					tools.last()->addDock(bw);
				bw->show();
			}
		}
	}

	connect(&timer, SIGNAL(timeout()), this, SLOT(initialPosition_callback()));
	timer.setSingleShot(true);
	timer.start(5000);
}

void Main::initialPosition_callback()
{
	foreach( InitialPosition i , initialPositionList )
	{
		/*i.widget->move(i.x,i.y);
		i.widget->resize(i.width, i.height);*/
		if(i.maximized)
		{
			i.widget->showMaximized();
		}
		else if(i.minimized) 
		{
			i.widget->showMinimized();
		}
	}
}

int main( int argn , char **argv )
{
	QApplication a( argn , argv );

	//Se inicializa la configuración
	getConfig("");

#	ifndef DEBUG

	{
		class SleeperThread : public QThread
		{
		public:
			static void msleep( unsigned long msecs ) 
			{
				QThread::msleep( msecs );
			}
		};

		QPixmap aSplashImage
		(
			QApplication::applicationDirPath() + "/styles/default/images/splash.png" 
		);

		CSplashScreen aSplashScreen( aSplashImage );
		aSplashScreen.show();

		for ( int i = 1 ; i <= 5 ; i++ )
		{
			aSplashScreen.showMessage
			(
				QString( "Processing %1..." ).arg(i) , 
				Qt::AlignTop | Qt::AlignLeft , 
				Qt::white 
			);

			SleeperThread::msleep( 500 );
		}
	}

#	endif

	// Translations
	QString transFile;
	QTranslator qtTranslator, qtoctaveTranslator;

	QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));

	// Qt translations
	QString locales=QLocale::system().name();
	//if(locales.length()>2)
	//{
	//	locales.resize(2);
	//}
		
	if(qtTranslator.load("qt_" + QLocale::system().name(),QLibraryInfo::location(QLibraryInfo::TranslationsPath) ) )
	{
	  a.installTranslator(&qtTranslator);
	  printf("[main()] Loaded QT Translation file for locale '%s'.\n",QLocale::system().name().toLocal8Bit().data());
	}
	else
	{
	  printf("[main()] Error loading the QT Translation file for locale '%s'.\n", QLocale::system().name().toLocal8Bit().data());
	}

	// QtOctave translations
	if( qtoctaveTranslator.load( "qtoctave_" + locales , langPath() ) )
	{
	  a.installTranslator(&qtoctaveTranslator);
	  printf("[main()] Loaded translation file for locale '%s'.\n",
		 QLocale::system().name().toLocal8Bit().data());
	}
	else
	{
	  std::cerr << "[main()] Error loading the translation file for locale" 
				<< locales.toLocal8Bit().data() << "not found in" << langPath().toStdString()
				<< std::endl;
	}

	// Create Terminal

	// Load
	a.processEvents();

	Main m;
	tesseract::config conf( "tesseract" , configPath().toStdString() + "config.xml" );

	// Connect configuration
	QObject::connect
	( 
		m.getTerminal()	, SIGNAL( sendConfiguration( const string & , const string &, const string &, const string &, const string &, const string & ) ), 
		&conf		  , SLOT    ( receiveConfiguration( const string & , const string &, const string &, const string &, const string &, const string & ) )
	);

	QObject::connect
	( 
		m.getTerminal()	, SIGNAL( receiveAttribute( const string & , string & ) ), 
		&conf		    , SLOT  ( requestAttribute( const string & , string & ) )
	);

	m.getTerminal()->initConfig();

	return a.exec();
}