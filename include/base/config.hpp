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

/** @file config.h
 * This file contains application's config parameters. Configuration is automatically load and save from CONFIG_PATH/config.rc and from user home/.qtoctave.rc
 */
#pragma once

#define PKG_ENABLED

#ifndef TESSERACT_BASE_CONFIG_HPP
#define TESSERACT_BASE_CONFIG_HPP

#include <cstdio>

#include <QMap>
#include <QString>
#include <QDomDocument>

#include <map>
#include <string>

#include <boost/lexical_cast.hpp>

using std::map;
using std::string;

#ifdef TESSERACT_USE_VLD
#	include <vld.h>
#endif

#include "session.hpp"

namespace tesseract
{

class settings
{
	int id;

	enum
	{
		ID_SETTINGS_ACTIVE,
		ID_SETTINGS_DEFAULT,
		ID_SETTINGS_LIMITS_MIN,
		ID_SETTINGS_LIMITS_MAX
	};

	explicit settings( int newId ) :
	id(newId) 
	{

	}


	public:

		int get() const;

		static settings actVal()
		{
			return settings( ID_SETTINGS_ACTIVE );
		}

		static settings defVal()
		{
			return settings( ID_SETTINGS_DEFAULT );
		}

		static settings limMin()
		{
			return settings( ID_SETTINGS_LIMITS_MIN );
		}

		static settings limMax()
		{
			return settings( ID_SETTINGS_LIMITS_MAX );
		}
};

class config : public QObject
{
	Q_OBJECT

	// how the xml document root node called
	const string rootname;

	// where is the configuration file located
	const string filename;

	typedef map< settings , QDomDocument > configmap;
	typedef std::pair< settings , QDomDocument > settingspair;

	configmap data;


	public:

		config( string const &rootname , string const &filename );

		/*
			<param name="name">
				Identification for needed parameter. A lookup will be started
				which iterates through the all configurations. The function returns
				on the first match.
			</param>

			<param name="val">
				Reference to the variable which corresponds with the first parameter
				'name'. The passed argument reference will be overwritten with the
				right value if a valid parameter identification 'name' can be found
				in any configurations.
			</param>
		*/
		template< class T > void requestProperty( const string &name , T &val ) const
		{
			//	Iterate through all settings
			for(  configurations::const_iterator j = data.begin() ; j != data.end() ; j++ )
			{
				// try to find the requested parameter key
				configmap::const_iterator i = (*j)->find( key );

				// Enters if block when the parameter was found
				if( i != (*j).end() )
				{
					try
					{
						// try to cast the parameter to the desired value
						val = boost::lexical_cast< T >( *i );
					}
					catch ( bad_lexical_cast )
					{
						BOOST_ASSERT_MSG( false , tr( "Could not cast the value to the passed parameter: " ) + key );						
					}
					finally
					{
						return;
					}
				}
			}

			BOOST_ASSERT_MSG( false , tr( "Could not find the requested parameter: " ) + key );
		}


	public slots:

		// this slot receives new configurations
		void receiveConfiguration
		( 
			const string &node, 
			const string &prop,
			const string &datatype,
			const string &defval,
			const string &minval = "", 
			const string &maxval = "" 
		);

		void requestAttribute( const string &node , string &propVal );
};

}

/**
 * Gets config of parameter.
 * @param parameter Parameter name.
 * @return A QString with parameter value.
 */
const QString getConfig( const QString &parameter );

/**
 * Sets config of parameter.
 * @param configuration Add a parameter with value. QMap key is parameter name. QMap value is parameter value.
 */
void setConfig( const QMap< QString , QString > &configuration );

#endif
