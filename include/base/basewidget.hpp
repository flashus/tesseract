/* Copyright (C) 2006 P.L. Lucas
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

#pragma once
#ifndef TESSERACT_BASE_BASEWIDGET_HPP
#define TESSERACT_BASE_BASEWIDGET_HPP

#include <string>
using std::string;

#include <memory>
using std::tr1::shared_ptr;

#include <QMenu>
#include <QWidget>
#include <QMainWindow>
#include <QDockWidget>
#include <QXmlStreamWriter>

#ifdef TESSERACT_USE_VLD
#	include <vld.h>
#endif

#include "session.hpp"

namespace tesseract{
class config;
}

/** This class is base class for windows of Tesseract.
 */
class BaseWidget:public QMainWindow
{
	Q_OBJECT

	QMenu *dockMenu;
	QVector< QObject * > docks;
	shared_ptr<tesseract::config> conf;
	void addAllDocksTo( BaseWidget *w );


	protected:

		class baseProperty
		{
			string name;
			string prop;
			string type;
			string actVal;
			string minVal;
			string maxVal;


			public:

				baseProperty
				(
					const string &name ,
					const string &prop ,
					const string &type , 
					const string &actVal , 
					const string &minVal = string() , 
					const string &maxVal = string() 
				);

				const string& getName() const;
				void setName( const string &name );

				const string& getProp() const;
				void setProp( const string &prop );

				const string& getType() const;
				void setType( const string &type );

				const string& getActVal() const;
				void setActVal( const string &actVal );

				const string& getMinVal() const;
				void setMinVal( const string &minVal );

				const string& getMaxVal() const;
				void setMaxVal( const string &actVal );
		};

		Session *session;
		WidgetType widget_type;


	public:

		BaseWidget( QWidget *parent = NULL );
		~BaseWidget();

		/**WidgetType of base widget.
		 * @return Type of widget.
		*/
		WidgetType widgetType();
		
		/**Sets session.*/
		void setSession( Session *session );
		/**Gets session.*/
		Session *getSession();
		
		/**Gets menu of this BaseWidget.*/
		virtual QMenu *getMenu();
		
		/**Gets a copy of this BaseWidget.*/
		virtual BaseWidget *copyBaseWidget( QWidget * parent = 0 ) = 0;
		
		/**Tool properties in xml.*/
		virtual void toXML( QXmlStreamWriter &xml );
		
		void addDock( QWidget *w );
		QVector<QObject*> getDocks();
		bool containsBaseWidget( BaseWidget *w );

		/** 
		*	To avoid that menu "View" is generated as
		*	the first most left menu, every inherited
		*	class should call this method at the right
		*	position where the menu should be created.
		*/

		void createMenuView();
		//void focusInEvent(QFocusEvent * event);
		//void focusOutEvent ( QFocusEvent * event );


	public slots:

		void showDockableObjects();
		void hideDockableObjects();
		void dockObject( QAction *action );
		void show_in_main_window_callback();
		void show_out_main_window_callback();
		void dock_destroyed_cb( QObject *obj );


	signals:

		/** This signal is emitted when widget is activated.*/
		void widget_activated( BaseWidget *w );
		
		/**Dynamic help required.*/
		void dynamic_help_required( const QString &text );
};

#endif
