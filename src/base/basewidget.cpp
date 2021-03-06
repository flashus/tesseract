/* Copyright (C) 2011 Tesseract Project
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

#include <boost/assert.hpp>

#include <QMenuBar>
#include <QMdiSubWindow>

#include "basewidget.hpp"
#include "mainwindow.hpp"


BaseWidget::baseProperty::baseProperty( const string &newName , const string &newProp, const string &newType , const string &newActVal , const string &newMinVal , const string &newMaxVal ) : 
name( newName ) ,
prop( newProp ) ,
type( newType ) ,
actVal( newActVal ) ,
minVal( newMinVal ) ,
maxVal( newMaxVal )
{
	BOOST_ASSERT_MSG( name.size() , "Empty string as name passed" );
	BOOST_ASSERT_MSG( type.size() , "Empty string as type passed" );
	BOOST_ASSERT_MSG( prop.size() , "Empty string as property passed" );
	BOOST_ASSERT_MSG( actVal.size() , "Empty string as default value passed" );
}

const string& BaseWidget::baseProperty::getName() const
{
	return name;
}

void BaseWidget::baseProperty::setName( const string &newName )
{
	BOOST_ASSERT_MSG( newName.size() , "Empty string as name passed" );
	name = newName;
}

const string& BaseWidget::baseProperty::getProp() const
{
	return prop;
}

void BaseWidget::baseProperty::setProp( const string &newProp )
{
	BOOST_ASSERT_MSG( newProp.size() , "Empty string as property passed" );
	prop = newProp;
}

const string& BaseWidget::baseProperty::getType() const
{
	return type;
}

void BaseWidget::baseProperty::setType( const string &newType )
{
	BOOST_ASSERT_MSG( newType.size() , "Empty string as type passed" );
	type = newType;
}

const string& BaseWidget::baseProperty::getActVal() const
{
	return actVal;
}

void BaseWidget::baseProperty::setActVal( const string &newActVal )
{
	BOOST_ASSERT_MSG( newActVal.size() , "Empty string as default value passed" );
	actVal = newActVal;
}

const string& BaseWidget::baseProperty::getMinVal() const
{
	return minVal;
}

void BaseWidget::baseProperty::setMinVal( const string &newMinVal )
{
	BOOST_ASSERT_MSG( newMinVal.size() , "Empty string as minimal value passed" );
	minVal = newMinVal;
}

const string& BaseWidget::baseProperty::getMaxVal() const
{
	return maxVal;
}

void BaseWidget::baseProperty::setMaxVal( const string &newMaxVal )
{
	BOOST_ASSERT_MSG( newMaxVal.size() , "Empty string as maximal value passed" );
	maxVal = newMaxVal;
}

BaseWidget::BaseWidget( QWidget *parent ) : 
QMainWindow( parent ),
session( NULL )
{
	setAttribute( Qt::WA_DeleteOnClose );
	setCentralWidget( new QWidget( this ) );
}

void BaseWidget::createMenuView()
{
	QMenu *menuView = menuBar()->addMenu( tr( "View" ) );

	dockMenu = menuView->addMenu( tr( "Add to..." ) );

	QAction *tmpAct01 = menuView->addAction( tr( "Show outside of main window" ) );
	QAction *tmpAct02 = menuView->addAction( tr( "Show inside of main window" ) );

	connect( tmpAct01 , SIGNAL( triggered()           ) , this , SLOT( show_out_main_window_callback() ) );
	connect( tmpAct02 , SIGNAL( triggered()           ) , this , SLOT( show_in_main_window_callback()  ) );
	connect( menuView , SIGNAL( aboutToShow()         ) , this , SLOT( showDockableObjects()           ) );
	connect( menuView , SIGNAL( aboutToHide()         ) , this , SLOT( hideDockableObjects()           ) );
	connect( dockMenu , SIGNAL( triggered( QAction* ) ) , this , SLOT( dockObject(QAction*)            ) );
}

BaseWidget::~BaseWidget()
{
	if( session != NULL )
	{
		session->removeTool( this );
	}
}

WidgetType BaseWidget::widgetType()
{
	return widget_type;
}

// void BaseWidget::focusInEvent(QFocusEvent * event)
// {
// 	//emit widget_activated(this);
// 	//menuBar()->show();
// 	//layout()->update();
// 	//update();
// 	//printf("Foco ganado\n");
// 	QWidget::focusInEvent(event);
// }
// 
// void BaseWidget::focusOutEvent(QFocusEvent * event)
// {
// 	//menuBar()->hide();
// 	//layout()->update();
// 	//update();
// 	//printf("Foco perdido\n");
// 	QWidget::focusOutEvent(event);
// }

void BaseWidget::setSession(Session *session)
{
	this->session=session;
	session->addTool(widget_type, this);
}

Session *BaseWidget::getSession()
{
	return session;
}

QMenu *BaseWidget::getMenu()
{
	return NULL;
}

void BaseWidget::show_out_main_window_callback()
{
	BaseWidget *w=copyBaseWidget();
	addAllDocksTo(w);
	w->show();
	
	QWidget *padre=(QWidget *)parent();
	
	QWidget *main_window=(QWidget *)(session->getFirstTool(MAINWINDOW));
	
	if(main_window!=padre && padre!=NULL && padre!=0)
	{
		padre->setAttribute(Qt::WA_DeleteOnClose);
		padre->close();
	}
}

void BaseWidget::show_in_main_window_callback()
{
	MainWindow *main_window=(MainWindow *)session->getFirstTool(MAINWINDOW);
	
	QList<QMdiSubWindow *> list=main_window->work_space->subWindowList();
	
	for(int i=0;i<list.size();i++)
	{
		if(list[i]->widget()==this)
		{
			return;
		}
	}
	
	BaseWidget *w=copyBaseWidget(main_window->work_space);
	addAllDocksTo(w);
	main_window->work_space->addSubWindow(w);
	w->show();
	
	if((QObject*)main_window!=parent())
	{
		if( parent()!=NULL && parent()!=0)
		{
			QWidget *w=(QWidget*)parent();
			w->setAttribute(Qt::WA_DeleteOnClose);
			w->close();
			return;
		}
		else close();
	}
}

void BaseWidget::showDockableObjects()
{
	dockMenu->clear();
	
	if(session==NULL) return;
	
	QVector<QObject*> tools=session->getTools();
	QVector<QObject*> main_tools=session->getTools(MAIN);
	
	
	BaseWidget *w, *w1=NULL;
	for(int i=0;i<tools.size();i++)
	{
		if( tools[i]==NULL || main_tools.contains(tools[i]) ) continue;
		w=(BaseWidget *)(tools[i]);
		if( w->containsBaseWidget(this) ) continue;
		
		for(int j=0;j<tools.size();j++)
		{
			if( tools[j]==NULL || tools[j]==w || main_tools.contains(tools[j]) ) continue;
			w1=(BaseWidget *)(tools[j]);
			if( w1->containsBaseWidget(w) && w1->containsBaseWidget(this) ) break;
			w1=NULL;
		}
		
		if(w1!=NULL) continue;
		
		
		QAction *_action=dockMenu->addAction(w->windowIcon(), w->windowTitle());
		_action->setData(QVariant::fromValue((QWidget*)w));
	}
}

void BaseWidget::hideDockableObjects()
{
	//dockMenu->clear();
}

void BaseWidget::dockObject(QAction *_action)
{
	BaseWidget *w=(BaseWidget *)(_action->data().value<QWidget*>());
	
	BaseWidget *child=copyBaseWidget(w);
	addAllDocksTo(child);
	w->addDock(child);
	
	QObject *main_window=(session->getFirstTool(MAINWINDOW));
	
	if(main_window!=parent())
	{
		if( parent()!=NULL && parent()!=0)
		{
			QWidget *w=(QWidget*)parent();
			w->setAttribute(Qt::WA_DeleteOnClose);
			w->close();
			return;
		}
		else close();
	}
	
	//close();
}

void BaseWidget::addDock(QWidget *w)
{
	QDockWidget *dock = new QDockWidget(w->windowTitle(),this);
	dock->setObjectName(w->windowTitle());
	addDockWidget(Qt::LeftDockWidgetArea, dock);
	
	dock->setWidget(w);
	dock->setAttribute(Qt::WA_DeleteOnClose);
	dock->show();
	
	docks.append(dock);
	
	connect(dock, SIGNAL(destroyed(QObject*)), this, SLOT(dock_destroyed_cb(QObject *)));
}

QVector<QObject*> BaseWidget::getDocks()
{
	return docks;
}

void BaseWidget::dock_destroyed_cb (QObject *obj)
{
	int i=docks.indexOf(obj);
	
	if(i<0) return;
	
	docks.remove(i);
}

void BaseWidget::addAllDocksTo(BaseWidget *w)
{
	for(int i=0;i<docks.size();i++)
	{
		QDockWidget *d=(QDockWidget *)(docks[i]);
		BaseWidget *bw=(BaseWidget *)(d->widget());
		BaseWidget *child=bw->copyBaseWidget(w);
		bw->addAllDocksTo(child);
		w->addDock(child);
	}
	
	QByteArray state=saveState();
	
	w->restoreState( state );
	w->update();
}

bool BaseWidget::containsBaseWidget( BaseWidget *w )
{
	if( w == this ) 
	{
		return true;
	}
	
	for( int i = 0 ; i < docks.size() ; i++ )
	{
		QDockWidget *d = static_cast<QDockWidget *>( docks[i] );
		BaseWidget *bw = static_cast<BaseWidget *>( d->widget() );

		if( bw->containsBaseWidget( w ) )
		{
			return true;
		}
	}
	
	return false;
}

void BaseWidget::toXML(QXmlStreamWriter & /* ATM unused */ )
{
	
}



