#include <cstdio>
#include <cstring>
#include <cstdlib>

#include <set>
#include <fstream>
#include <iostream>

using std::set;
using std::ifstream;

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

using boost::property_tree::ptree;
using boost::property_tree::ptree_bad_path;
using boost::property_tree::xml_parser_error;


#include <QMap>
#include <QDir>
#include <QString>
#include <QTranslator>

#include "config.hpp"
#include "projects.hpp"

namespace tesseract
{

bool operator < ( settings const & lhs , settings const & rhs )
{
	return lhs.get() < rhs.get();
}

config::config( string const &newRootName , string const &newConfigPath ) : 
rootname( newRootName ),
filename( newConfigPath )
{

}

void config::initActiveSettings()
{
	try
	{
		ptree pt;
		read_xml( filename , pt );
		
		vector<string> limitValues;
		vector<string> defaultKeys;
		vector<string> defaultValues;

		//for( configmap::const_iterator i = rmap.begin() ; i != rmap.end() ; i++ )
		//{
		//	defaultKeys.push_back( i->first );
		//	defaultValues.push_back( i->second );

		//	try
		//	{
		//		string activeValue = pt.get<string>( rootname + "." + node + "." + i->first );
		//	}
		//	catch ( boost::exception * e )
		//	{
		//		int shit = 0;
		//	}
		//}

//		data.insert( configpair( settings::active()  , configmap() ) );
	}
	catch( ptree_bad_path * e )
	{
		int i = 0;
	}
	catch (	xml_parser_error * e )
	{
		string errormsg( "Error while parsing xml file (" + e->filename() + " )" );
//		errormsg.append( "in line ( " + e->line() + ")." );
		errormsg.append( "Message: ( " + e->message() );
		errormsg.append( boost::lexical_cast<string,int>( e->line() ) );
		errormsg.append( ". ( " + e->message() + ")." );
		
		BOOST_ASSERT_MSG( false , errormsg.c_str() );
	}
}

// this slot receives new configurations
void config::receiveConfiguration( string const &node , shared_ptr<map< string , string >> const &defaults , shared_ptr<map< string , string >> const &limits )
{
	configmap limmap;
	configmap defmap;

	limmap.insert( settingspair( settings::limit()   , limits   ) );
	defmap.insert( settingspair( settings::default() , defaults ) );

	data.insert( configpair( node , limmap ) );
	data.insert( configpair( node , defmap ) );
}

int settings::get() const
{
	return id;
}

}

static QMap<QString, QString> config;

static QMap<QString, QString> load_config( const QString &file )
{
	QMap<QString, QString> config;
	
	FILE *in = fopen( file.toLocal8Bit().data() , "r" );
	
	if( in == NULL ) 
	{
		std::cout << "The config file :" << file.toStdString() 
				  << "does not exists" << std::endl; 

		return config;
	}
	
	char buf[1024];
	char *p, *parameter;
	char *value;
		
	while( ! feof( in ) )
	{
		if( fgets( buf , 1024 , in ) == NULL ) 
		{
			break;
		}

		if( feof( in ) )
		{
			break;
		}

		if( buf[0] == '#' ) 
		{
			continue;
		}

		p = strchr( buf , '=' );

		if( p != NULL )
		{
			*p = '\0';

			parameter=buf;
			value = p + 1;

			QString qparameter( QString( parameter ).trimmed() );
			QString qvalue( QString( value ).trimmed() );
			
			if( config.value( parameter ).isEmpty() )
			{
				config[qparameter] = qvalue;
			}
			else
			{
				config[qparameter] = config[qparameter] + '\n' + qvalue;
			}
		}
	}
	
	fclose( in );
	
	return config;
}

const QString getConfig( const QString &parameter )
{
	if ( config.isEmpty() )
	{	
		config = load_config( configPath() + "config.rc" );
	}
	
	return config.value( parameter );
}


void setConfig( const QMap<QString,QString> &configuration )
{
	QMapIterator<QString, QString> k( configuration );
	
	while ( k.hasNext() )
	{
		k.next();
		
		if( ! k.key().isEmpty() )
		{
			config[k.key().trimmed()] = k.value().trimmed();
		}
	}
	
	QString home_config = configPath() + "config.rc";
	
	FILE *out = fopen( home_config.toLocal8Bit().data() , "w" );

	if( ! out )
	{
		exit(-1234567890);
	}
	
	fprintf( out , "#QtOctave local configuration.\n" );
	
	QMapIterator<QString, QString> i( config );

	while ( i.hasNext() )
	{
		i.next();

		if( i.key().size() && i.value().size() )
		{
			QStringList values = i.value().split( '\n' );
			QString item;

			foreach( item , values )
			{
				fprintf( out , "%s=%s\n" , i.key().toLocal8Bit().data() , item.toLocal8Bit().data() );
			}
		}
	}
	
	fclose( out );
}

