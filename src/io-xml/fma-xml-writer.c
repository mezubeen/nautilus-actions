/*
 * FileManager-Actions
 * A file-manager extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009-2015 Pierre Wieser and others (see AUTHORS)
 *
 * FileManager-Actions is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * FileManager-Actions is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FileManager-Actions; see the file COPYING. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Frederic Ruaudel <grumz@grumz.net>
 *   Rodrigo Moya <rodrigo@gnome-db.org>
 *   Pierre Wieser <pwieser@trychlos.org>
 *   ... and many others (see AUTHORS)
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gio/gio.h>
#include <libintl.h>
#include <libxml/tree.h>
#include <string.h>

#include <api/fma-core-utils.h>
#include <api/fma-data-types.h>
#include <api/fma-object-api.h>
#include <api/fma-ifactory-provider.h>
#include <api/fma-iio-provider.h>

#include <io-gconf/fma-gconf-keys.h>

#include "fma-xml-formats.h"
#include "fma-xml-keys.h"
#include "fma-xml-writer.h"

typedef struct ExportFormatFn ExportFormatFn;

/* private class data
 */
struct _FMAXMLWriterClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct _FMAXMLWriterPrivate {
	gboolean         dispose_has_run;
	FMAIExporter    *provider;
	FMAObjectItem   *exported;
	GSList          *messages;

	/* positionning these at document level
	 */
	xmlDocPtr        doc;
	ExportFormatFn  *fn_str;
	gchar           *buffer;

	xmlNodePtr       root_node;
	xmlNodePtr       list_node;

	/* nodes created in write_data_schema_v2(), used in write_data_schema_v1()
	 */
	xmlNodePtr       schema_node;
	xmlNodePtr       locale_node;
};

/* the association between an export format and the functions
 */
struct ExportFormatFn {
	gchar  *format;
	gchar  *root_node;
	gchar  *list_node;
	void ( *write_list_attribs_fn )( FMAXMLWriter *, const FMAObjectItem * );
	gchar  *element_node;
	void ( *write_data_fn )( FMAXMLWriter *, const FMAObjectId *, const FMADataBoxed *, const FMADataDef * );
	void ( *write_type_fn )( FMAXMLWriter *, const FMAObjectItem *, const FMADataDef *, const gchar * );
};

static GObjectClass *st_parent_class = NULL;

static GType           register_type( void );
static void            class_init( FMAXMLWriterClass *klass );
static void            instance_init( GTypeInstance *instance, gpointer klass );
static void            instance_dispose( GObject *object );
static void            instance_finalize( GObject *object );

static void            write_start_write_type( FMAXMLWriter *writer, FMAObjectItem *object, const FMADataGroup *groups );
static void            write_start_write_version( FMAXMLWriter *writer, FMAObjectItem *object, const FMADataGroup *groups );

static void            write_data_schema_v1( FMAXMLWriter *writer, const FMAObjectId *object, const FMADataBoxed *boxed, const FMADataDef *def );
static void            write_data_schema_v1_element( FMAXMLWriter *writer, const FMADataDef *def );
static void            write_type_schema_v1( FMAXMLWriter *writer, const FMAObjectItem *object, const FMADataDef *def, const gchar *value );
static void            write_data_schema_v2( FMAXMLWriter *writer, const FMAObjectId *object, const FMADataBoxed *boxed, const FMADataDef *def );
static void            write_data_schema_v2_element( FMAXMLWriter *writer, const FMADataDef *def, const gchar *object_id, const gchar *value_str );
static void            write_type_schema_v2( FMAXMLWriter *writer, const FMAObjectItem *object, const FMADataDef *def, const gchar *value );
static void            write_list_attribs_dump( FMAXMLWriter *writer, const FMAObjectItem *object );
static void            write_data_dump( FMAXMLWriter *writer, const FMAObjectId *object, const FMADataBoxed *boxed, const FMADataDef *def );
static void            write_data_dump_element( FMAXMLWriter *writer, const FMADataDef *def, const FMADataBoxed *boxed, const gchar *entry, const gchar *value_str );
static void            write_type_dump( FMAXMLWriter *writer, const FMAObjectItem *object, const FMADataDef *def, const gchar *value );

static xmlDocPtr       build_xml_doc( FMAXMLWriter *writer );
static gchar          *convert_to_gconf_slist( const gchar *str );
static ExportFormatFn *find_export_format_fn( const gchar *format );

#ifdef FMA_ENABLE_DEPRECATED
static ExportFormatFn *find_export_format_fn_from_quark( GQuark format );
#endif

static gchar          *get_output_fname( const FMAObjectItem *item, const gchar *folder, const gchar *format );
static void            output_xml_to_file( const gchar *xml, const gchar *filename, GSList **msg );
static guint           writer_to_buffer( FMAXMLWriter *writer );

static ExportFormatFn st_export_format_fn[] = {

	{ FMA_XML_FORMAT_GCONF_SCHEMA_V1,
					FMA_XML_KEY_SCHEMA_ROOT,
					FMA_XML_KEY_SCHEMA_LIST,
					NULL,
					FMA_XML_KEY_SCHEMA_NODE,
					write_data_schema_v1,
					write_type_schema_v1 },

	{ FMA_XML_FORMAT_GCONF_SCHEMA_V2,
					FMA_XML_KEY_SCHEMA_ROOT,
					FMA_XML_KEY_SCHEMA_LIST,
					NULL,
					FMA_XML_KEY_SCHEMA_NODE,
					write_data_schema_v2,
					write_type_schema_v2 },

	{ FMA_XML_FORMAT_GCONF_ENTRY,
					FMA_XML_KEY_DUMP_ROOT,
					FMA_XML_KEY_DUMP_LIST,
					write_list_attribs_dump,
					FMA_XML_KEY_DUMP_NODE,
					write_data_dump,
					write_type_dump },

	{ NULL }
};

GType
fma_xml_writer_get_type( void )
{
	static GType object_type = 0;

	if( !object_type ){
		object_type = register_type();
	}

	return( object_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "fma_xml_writer_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( FMAXMLWriterClass ),
		NULL,
		NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( FMAXMLWriter ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_OBJECT, "FMAXMLWriter", &info, 0 );

	return( type );
}

static void
class_init( FMAXMLWriterClass *klass )
{
	static const gchar *thisfn = "fma_xml_writer_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( FMAXMLWriterClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "fma_xml_writer_instance_init";
	FMAXMLWriter *self;

	g_debug( "%s: instance=%p, klass=%p", thisfn, ( void * ) instance, ( void * ) klass );
	g_return_if_fail( FMA_IS_XML_WRITER( instance ));
	self = FMA_XML_WRITER( instance );

	self->private = g_new0( FMAXMLWriterPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "fma_xml_writer_instance_dispose";
	FMAXMLWriter *self;

	g_debug( "%s: object=%p", thisfn, ( void * ) object );
	g_return_if_fail( FMA_IS_XML_WRITER( object ));
	self = FMA_XML_WRITER( object );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( object );
		}
	}
}

static void
instance_finalize( GObject *object )
{
	FMAXMLWriter *self;

	g_return_if_fail( FMA_IS_XML_WRITER( object ));
	self = FMA_XML_WRITER( object );

	/* do not release self->private->buffer as the pointer has been
	 * returned to the caller
	 */

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/**
 * fma_xml_writer_export_to_buffer:
 * @instance: this #FMAIExporter instance.
 * @parms: a #FMAIExporterBufferParmsv2 structure.
 *
 * Export the specified 'item' to a newly allocated buffer.
 */
guint
fma_xml_writer_export_to_buffer( const FMAIExporter *instance, FMAIExporterBufferParmsv2 *parms )
{
	static const gchar *thisfn = "fma_xml_writer_export_to_buffer";
	FMAXMLWriter *writer;
	guint code;

	g_debug( "%s: instance=%p, parms=%p", thisfn, ( void * ) instance, ( void * ) parms );

	code = FMA_IEXPORTER_CODE_OK;

	if( !parms->exported || !FMA_IS_OBJECT_ITEM( parms->exported )){
		code = FMA_IEXPORTER_CODE_INVALID_ITEM;
	}

	if( code == FMA_IEXPORTER_CODE_OK ){
		writer = FMA_XML_WRITER( g_object_new( FMA_XML_WRITER_TYPE, NULL ));

		writer->private->provider = ( FMAIExporter * ) instance;
		writer->private->exported = parms->exported;
		writer->private->messages = parms->messages;
#ifdef FMA_ENABLE_DEPRECATED
		if( parms->version == 1 ){
			writer->private->fn_str = find_export_format_fn_from_quark((( FMAIExporterBufferParms * ) parms )->format );
		} else {
			writer->private->fn_str = find_export_format_fn( parms->format );
		}
#else
		writer->private->fn_str = find_export_format_fn( parms->format );
#endif
		writer->private->buffer = NULL;

		if( !writer->private->fn_str ){
			code = FMA_IEXPORTER_CODE_INVALID_FORMAT;

		} else {
			code = writer_to_buffer( writer );
			if( code == FMA_IEXPORTER_CODE_OK ){
				parms->buffer = writer->private->buffer;
			}
		}

		g_object_unref( writer );
	}

	g_debug( "%s: returning code=%u", thisfn, code );
	return( code );
}

/**
 * fma_xml_writer_export_to_file:
 * @instance: this #FMAIExporter instance.
 * @parms: a #FMAIExporterFileParmsv2 structure.
 *
 * Export the specified 'item' to a newly created file.
 */
guint
fma_xml_writer_export_to_file( const FMAIExporter *instance, FMAIExporterFileParmsv2 *parms )
{
	static const gchar *thisfn = "fma_xml_writer_export_to_file";
	FMAXMLWriter *writer;
	gchar *filename;
	guint code;
	const gchar *format2;

	g_debug( "%s: instance=%p, parms=%p", thisfn, ( void * ) instance, ( void * ) parms );

	code = FMA_IEXPORTER_CODE_OK;

	if( !parms->exported || !FMA_IS_OBJECT_ITEM( parms->exported )){
		code = FMA_IEXPORTER_CODE_INVALID_ITEM;
	}

	if( code == FMA_IEXPORTER_CODE_OK ){
		writer = FMA_XML_WRITER( g_object_new( FMA_XML_WRITER_TYPE, NULL ));

		writer->private->provider = ( FMAIExporter * ) instance;
		writer->private->exported = parms->exported;
		writer->private->messages = parms->messages;
#ifdef FMA_ENABLE_DEPRECATED
		if( parms->version == 1 ){
			writer->private->fn_str = find_export_format_fn_from_quark((( FMAIExporterFileParms * ) parms )->format );
			format2 = g_quark_to_string((( FMAIExporterFileParms * ) parms )->format );
		} else {
			writer->private->fn_str = find_export_format_fn( parms->format );
			format2 = parms->format;
		}
#else
		writer->private->fn_str = find_export_format_fn( parms->format );
		format2 = parms->format;
#endif
		writer->private->buffer = NULL;

		if( !writer->private->fn_str ){
			code = FMA_IEXPORTER_CODE_INVALID_FORMAT;

		} else {
			code = writer_to_buffer( writer );

			if( code == FMA_IEXPORTER_CODE_OK ){
				filename = get_output_fname( parms->exported, parms->folder, format2 );

				if( filename ){
					parms->basename = g_path_get_basename( filename );
					output_xml_to_file(
							writer->private->buffer, filename, parms->messages ? &writer->private->messages : NULL );
					g_free( filename );
				}
			}

			g_free( writer->private->buffer );
		}

		g_object_unref( writer );
	}

	g_debug( "%s: returning code=%u", thisfn, code );
	return( code );
}

static xmlDocPtr
build_xml_doc( FMAXMLWriter *writer )
{
	writer->private->doc = xmlNewDoc( BAD_CAST( "1.0" ));

	writer->private->root_node = xmlNewNode( NULL, BAD_CAST( writer->private->fn_str->root_node ));
	xmlDocSetRootElement( writer->private->doc, writer->private->root_node );

	fma_ifactory_provider_write_item(
			FMA_IFACTORY_PROVIDER( writer->private->provider ),
			writer,
			FMA_IFACTORY_OBJECT( writer->private->exported ),
			writer->private->messages ? & writer->private->messages : NULL );

	return( writer->private->doc );
}

guint
fma_xml_writer_write_start( const FMAIFactoryProvider *provider, void *writer_data, const FMAIFactoryObject *object, GSList **messages  )
{
	FMAXMLWriter *writer;
	FMADataGroup *groups;

	g_debug( "fma_xml_writer_write_start: object=%p (%s)", ( void * ) object, G_OBJECT_TYPE_NAME( object ));

	if( FMA_IS_OBJECT_ITEM( object )){
		fma_object_dump( object );

		writer = FMA_XML_WRITER( writer_data );

		writer->private->list_node = xmlNewChild( writer->private->root_node, NULL, BAD_CAST( writer->private->fn_str->list_node ), NULL );

		if( writer->private->fn_str->write_list_attribs_fn ){
			( *writer->private->fn_str->write_list_attribs_fn )( writer, FMA_OBJECT_ITEM( object ));
		}

		groups = fma_ifactory_object_get_data_groups( object );
		write_start_write_type( writer, FMA_OBJECT_ITEM( object ), groups );
		write_start_write_version( writer, FMA_OBJECT_ITEM( object ), groups );
	}

	return( IIO_PROVIDER_CODE_OK );
}

/* at end of write_start (list_node already created)
 * explicitly write the 'Type' node
 */
static void
write_start_write_type( FMAXMLWriter *writer, FMAObjectItem *object, const FMADataGroup *groups )
{
	const FMADataDef *def;
	const gchar *svalue;

	writer->private->schema_node = NULL;
	writer->private->locale_node = NULL;
	def = fma_data_def_get_data_def( groups, FMA_FACTORY_OBJECT_ITEM_GROUP, FMAFO_DATA_TYPE );
	svalue = FMA_IS_OBJECT_ACTION( object ) ? FMA_GCONF_VALUE_TYPE_ACTION : FMA_GCONF_VALUE_TYPE_MENU;

	( *writer->private->fn_str->write_type_fn )( writer, object, def, svalue );
}

static void
write_start_write_version( FMAXMLWriter *writer, FMAObjectItem *object, const FMADataGroup *groups )
{
	const FMADataDef *def;
	guint iversion;
	gchar *svalue;

	writer->private->schema_node = NULL;
	writer->private->locale_node = NULL;
	def = fma_data_def_get_data_def( groups, FMA_FACTORY_OBJECT_ITEM_GROUP, FMAFO_DATA_IVERSION );
	iversion = fma_object_get_iversion( object );
	svalue = g_strdup_printf( "%d", iversion );

	( *writer->private->fn_str->write_type_fn )( writer, object, def, svalue );

	g_free( svalue );
}

guint
fma_xml_writer_write_data( const FMAIFactoryProvider *provider, void *writer_data, const FMAIFactoryObject *object, const FMADataBoxed *boxed, GSList **messages )
{
	FMAXMLWriter *writer;
	const FMADataDef *def;

	/*FMADataDef *def = fma_data_boxed_get_data_def( boxed );
	g_debug( "fma_xml_writer_write_data: def=%s", def->name );*/

	def = fma_data_boxed_get_data_def( boxed );

	/* do no export empty values
	 */
	if( !fma_data_boxed_is_default( boxed ) || def->write_if_default ){

		writer = FMA_XML_WRITER( writer_data );

		writer->private->schema_node = NULL;
		writer->private->locale_node = NULL;

		( *writer->private->fn_str->write_data_fn )( writer, FMA_OBJECT_ID( object ), boxed, def );
	}

	return( IIO_PROVIDER_CODE_OK );
}

guint
fma_xml_writer_write_done( const FMAIFactoryProvider *provider, void *writer_data, const FMAIFactoryObject *object, GSList **messages  )
{
	return( IIO_PROVIDER_CODE_OK );
}

static void
write_data_schema_v1( FMAXMLWriter *writer, const FMAObjectId *object, const FMADataBoxed *boxed, const FMADataDef *def )
{
	write_data_schema_v2( writer, object, boxed, def );

	write_data_schema_v1_element( writer, def );
}

static void
write_data_schema_v1_element( FMAXMLWriter *writer, const FMADataDef *def )
{
	if( !writer->private->locale_node ){
		writer->private->locale_node = xmlNewChild( writer->private->schema_node, NULL, BAD_CAST( FMA_XML_KEY_SCHEMA_NODE_LOCALE ), NULL );
		xmlNewProp( writer->private->locale_node, BAD_CAST( "name" ), BAD_CAST( "C" ));
	}

	xmlNewChild( writer->private->schema_node, NULL, BAD_CAST( FMA_XML_KEY_SCHEMA_NODE_OWNER ), BAD_CAST( PACKAGE_TARNAME ));
	xmlNewChild( writer->private->locale_node, NULL, BAD_CAST( FMA_XML_KEY_SCHEMA_NODE_LOCALE_SHORT ), BAD_CAST( gettext( def->short_label )));
	xmlNewChild( writer->private->locale_node, NULL, BAD_CAST( FMA_XML_KEY_SCHEMA_NODE_LOCALE_LONG ), BAD_CAST( gettext( def->long_label )));
}

static void
write_type_schema_v1( FMAXMLWriter *writer, const FMAObjectItem *object, const FMADataDef *def, const gchar *value )
{
	gchar *object_id;

	object_id = fma_object_get_id( object );
	write_data_schema_v2_element( writer, def, object_id, value );
	write_data_schema_v1_element( writer, def );

	g_free( object_id );
}

/*
 * <schema>
 *  <key>/schemas/apps/filemanager-actions/configurations/entry</key>
 *  <applyto>/apps/filemanager-actions/configurations/item_id/profile_id/entry</applyto>
 */
static void
write_data_schema_v2( FMAXMLWriter *writer, const FMAObjectId *object, const FMADataBoxed *boxed, const FMADataDef *def )
{
	gchar *object_id;
	gchar *value_str;
	gchar *tmp;

	value_str = fma_boxed_get_string( FMA_BOXED( boxed ));

	/* boolean value must be lowercase
	 */
	if( def->type == FMA_DATA_TYPE_BOOLEAN ){
		tmp = g_ascii_strdown( value_str, -1 );
		g_free( value_str );
		value_str = tmp;
	}

	/* string or uint list value must be converted to gconf format
	 * comma-separated and enclosed within square brackets
	 */
	if( def->type == FMA_DATA_TYPE_STRING_LIST || def->type == FMA_DATA_TYPE_UINT_LIST ){
		tmp = convert_to_gconf_slist( value_str );
		g_free( value_str );
		value_str = tmp;
	}

	object_id = fma_object_get_id( object );

	if( FMA_IS_OBJECT_PROFILE( object )){
		FMAObjectItem *action = fma_object_get_parent( object );
		gchar *id = fma_object_get_id( action );
		gchar *tmp = g_strdup_printf( "%s/%s", id, object_id );
		g_free( id );
		g_free( object_id );
		object_id = tmp;
	}

	write_data_schema_v2_element( writer, def, object_id, value_str );

	g_free( value_str );
	g_free( object_id );
}

/*
 * <schema>
 *  <key>/schemas/apps/filemanager-actions/configurations/entry</key>
 *  <applyto>/apps/filemanager-actions/configurations/item_id/profile_id/entry</applyto>
 */
static void
write_data_schema_v2_element( FMAXMLWriter *writer, const FMADataDef *def, const gchar *object_id, const gchar *value_str )
{
	xmlChar *content;
	xmlNodePtr parent_value_node;

	writer->private->schema_node = xmlNewChild( writer->private->list_node, NULL, BAD_CAST( FMA_XML_KEY_SCHEMA_NODE ), NULL );

	content = BAD_CAST( g_build_path( "/", FMA_GCONF_SCHEMAS_PATH, def->gconf_entry, NULL ));
	xmlNewChild( writer->private->schema_node, NULL, BAD_CAST( FMA_XML_KEY_SCHEMA_NODE_KEY ), content );
	xmlFree( content );

	content = BAD_CAST( g_build_path( "/", FMA_GCONF_CONFIGURATIONS_PATH, object_id, def->gconf_entry, NULL ));
	xmlNewChild( writer->private->schema_node, NULL, BAD_CAST( FMA_XML_KEY_SCHEMA_NODE_APPLYTO ), content );
	xmlFree( content );

	xmlNewChild( writer->private->schema_node, NULL, BAD_CAST( FMA_XML_KEY_SCHEMA_NODE_TYPE ), BAD_CAST( fma_data_types_get_gconf_dump_key( def->type )));
	if( def->type == FMA_DATA_TYPE_STRING_LIST ){
		xmlNewChild( writer->private->schema_node, NULL, BAD_CAST( FMA_XML_KEY_SCHEMA_NODE_LISTTYPE ), BAD_CAST( "string" ));
	}

	parent_value_node = writer->private->schema_node;

	if( def->localizable ){
		writer->private->locale_node = xmlNewChild( writer->private->schema_node, NULL, BAD_CAST( FMA_XML_KEY_SCHEMA_NODE_LOCALE ), NULL );
		xmlNewProp( writer->private->locale_node, BAD_CAST( "name" ), BAD_CAST( "C" ));
		parent_value_node = writer->private->locale_node;
	}

	content = xmlEncodeSpecialChars( writer->private->doc, BAD_CAST( value_str ));
	xmlNewChild( parent_value_node, NULL, BAD_CAST( FMA_XML_KEY_SCHEMA_NODE_DEFAULT ), content );
	xmlFree( content );
}

/*
 * <schema>
 *  <key>/schemas/apps/filemanager-actions/configurations/entry</key>
 *  <applyto>/apps/filemanager-actions/configurations/item_id/profile_id/entry</applyto>
 */
static void
write_type_schema_v2( FMAXMLWriter *writer, const FMAObjectItem *object, const FMADataDef *def, const gchar *value )
{
	gchar *object_id;

	object_id = fma_object_get_id( object );
	write_data_schema_v2_element( writer, def, object_id, value );

	g_free( object_id );
}

static void
write_list_attribs_dump( FMAXMLWriter *writer, const FMAObjectItem *object )
{
	gchar *id;
	gchar *path;

	id = fma_object_get_id( object );
	path = g_build_path( "/", FMA_GCONF_CONFIGURATIONS_PATH, id, NULL );
	xmlNewProp( writer->private->list_node, BAD_CAST( FMA_XML_KEY_DUMP_LIST_PARM_BASE ), BAD_CAST( path ));

	g_free( path );
	g_free( id );
}

static void
write_data_dump( FMAXMLWriter *writer, const FMAObjectId *object, const FMADataBoxed *boxed, const FMADataDef *def )
{
	gchar *entry;
	gchar *value_str;
	gchar *tmp;

	value_str = fma_boxed_get_string( FMA_BOXED( boxed ));

	/* boolean value must be lowercase
	 */
	if( def->type == FMA_DATA_TYPE_BOOLEAN ){
		tmp = g_ascii_strdown( value_str, -1 );
		g_free( value_str );
		value_str = tmp;
	}

	/* string or uint list value must be converted to gconf format
	 * comma-separated and enclosed within square brackets
	 */
	if( def->type == FMA_DATA_TYPE_STRING_LIST || def->type == FMA_DATA_TYPE_UINT_LIST ){
		tmp = convert_to_gconf_slist( value_str );
		g_free( value_str );
		value_str = tmp;
	}

	if( FMA_IS_OBJECT_PROFILE( object )){
		gchar *id = fma_object_get_id( object );
		entry = g_strdup_printf( "%s/%s", id, def->gconf_entry );
		g_free( id );
	} else {
		entry = g_strdup( def->gconf_entry );
	}

	write_data_dump_element( writer, def, boxed, entry, value_str );

	g_free( entry );
	g_free( value_str );
}

static void
write_data_dump_element( FMAXMLWriter *writer, const FMADataDef *def, const FMADataBoxed *boxed, const gchar *entry, const gchar *value_str )
{
	xmlNodePtr entry_node;
	xmlNodePtr value_node;
	xmlNodePtr value_list_node, value_list_value_node;
	GSList *list, *is;
	xmlChar *encoded_content;

	entry_node = xmlNewChild( writer->private->list_node, NULL, BAD_CAST( writer->private->fn_str->element_node ), NULL );

	xmlNewChild( entry_node, NULL, BAD_CAST( FMA_XML_KEY_DUMP_NODE_KEY ), BAD_CAST( entry ));

	value_node = xmlNewChild( entry_node, NULL, BAD_CAST( FMA_XML_KEY_DUMP_NODE_VALUE ), NULL );

	if( def->type == FMA_DATA_TYPE_STRING_LIST ){
		value_list_node = xmlNewChild( value_node, NULL, BAD_CAST( FMA_XML_KEY_DUMP_NODE_VALUE_LIST ), NULL );
		xmlNewProp( value_list_node, BAD_CAST( FMA_XML_KEY_DUMP_NODE_VALUE_LIST_PARM_TYPE ), BAD_CAST( FMA_XML_KEY_DUMP_NODE_VALUE_TYPE_STRING ));
		value_list_value_node = xmlNewChild( value_list_node, NULL, BAD_CAST( FMA_XML_KEY_DUMP_NODE_VALUE ), NULL );
		list = ( GSList * ) fma_boxed_get_as_void( FMA_BOXED( boxed ));

		for( is = list ; is ; is = is->next ){
			encoded_content = xmlEncodeSpecialChars( writer->private->doc, BAD_CAST(( gchar * ) is->data ));
			xmlNewChild( value_list_value_node, NULL, BAD_CAST( FMA_XML_KEY_DUMP_NODE_VALUE_TYPE_STRING ), encoded_content );
			xmlFree( encoded_content );
		}

		fma_core_utils_slist_free( list );

	} else {
		encoded_content = xmlEncodeSpecialChars( writer->private->doc, BAD_CAST( value_str ));
		xmlNewChild( value_node, NULL, BAD_CAST( fma_data_types_get_gconf_dump_key( def->type )), encoded_content );
		xmlFree( encoded_content );
	}
}

static void
write_type_dump( FMAXMLWriter *writer, const FMAObjectItem *object, const FMADataDef *def, const gchar *value )
{
	write_data_dump_element( writer, def, NULL, def->gconf_entry, value );
}

/*
 * we have here a string list as "value; value;"
 * we want "[value, value]"
 */
static gchar *
convert_to_gconf_slist( const gchar *slist_str )
{
	GSList *values;
	GSList *is;
	gboolean first;
	GString *str = g_string_new( "[" );

	values = fma_core_utils_slist_from_split( slist_str, ";" );
	first = TRUE;

	for( is = values ; is ; is = is->next ){
		if( !first ){
			str = g_string_append( str, "," );
		}
		str = g_string_append( str, ( const gchar * ) is->data );
		first = FALSE;
	}

	str = g_string_append( str, "]" );

	return( g_string_free( str, FALSE ));
}

static ExportFormatFn *
find_export_format_fn( const gchar *format )
{
	ExportFormatFn *found;
	ExportFormatFn *i;

	found = NULL;
	i = st_export_format_fn;

	while( i->format && !found ){
		if( !strcmp( i->format, format )){
			found = i;
		}
		i++;
	}

	return( found );
}

#ifdef FMA_ENABLE_DEPRECATED
static ExportFormatFn *
find_export_format_fn_from_quark( GQuark format )
{
	ExportFormatFn *found;
	ExportFormatFn *i;

	found = NULL;
	i = st_export_format_fn;

	while( i->format && !found ){
		if( g_quark_from_string( i->format ) == format ){
			found = i;
		}
		i++;
	}

	return( found );
}
#endif

/*
 * get_output_fname:
 * @item: the #FMAObjectItme-derived object to be exported.
 * @folder: the URI of the directoy where to write the output XML file.
 * @format: the export format.
 *
 * Returns: a filename suitable for writing the output XML.
 *
 * As we don't want overwrite already existing files, the candidate
 * filename is incremented until we find an available filename.
 *
 * The returned string should be g_free() by the caller.
 *
 * Note that this function is always subject to race condition, as it
 * is possible, though unlikely, that the given file be created
 * between our test of inexistance and the actual write.
 */
static gchar *
get_output_fname( const FMAObjectItem *item, const gchar *folder, const gchar *format )
{
	static const gchar *thisfn = "fma_xml_writer_get_output_fname";
	gchar *item_id;
	gchar *canonical_fname = NULL;
	gchar *canonical_ext = NULL;
	gchar *candidate_fname;
	gint counter;

	g_return_val_if_fail( FMA_IS_OBJECT_ITEM( item ), NULL );
	g_return_val_if_fail( folder, NULL );
	g_return_val_if_fail( strlen( folder ), NULL );

	item_id = fma_object_get_id( item );

	if( !strcmp( format, FMA_XML_FORMAT_GCONF_SCHEMA_V1 )){
		canonical_fname = g_strdup_printf( "config_%s", item_id );
		canonical_ext = g_strdup( "schemas" );

	} else if( !strcmp( format, FMA_XML_FORMAT_GCONF_SCHEMA_V2 )){
		canonical_fname = g_strdup_printf( "config-%s", item_id );
		canonical_ext = g_strdup( "schema" );

	} else if( !strcmp( format, FMA_XML_FORMAT_GCONF_ENTRY )){
		canonical_fname = g_strdup_printf( "%s-%s", FMA_IS_OBJECT_ACTION( item ) ? "action" : "menu", item_id );
		canonical_ext = g_strdup( "xml" );

	} else {
		g_warning( "%s: unknown format: %s", thisfn, format );
	}

	g_free( item_id );
	g_return_val_if_fail( canonical_fname, NULL );

	candidate_fname = g_strdup_printf( "%s/%s.%s", folder, canonical_fname, canonical_ext );

	if( !fma_core_utils_file_exists( candidate_fname )){
		g_free( canonical_fname );
		g_free( canonical_ext );
		return( candidate_fname );
	}

	for( counter = 0 ; ; ++counter ){
		g_free( candidate_fname );
		candidate_fname = g_strdup_printf( "%s/%s_%d.%s", folder, canonical_fname, counter, canonical_ext );
		if( !fma_core_utils_file_exists( candidate_fname )){
			break;
		}
	}

	g_free( canonical_fname );
	g_free( canonical_ext );

	return( candidate_fname );
}

/*
 * output_xml_to_file:
 * @xml: the xml buffer.
 * @filename: the full path of the output filename as an URI.
 * @msg: a GSList to append messages.
 *
 * Exports an item to the given filename.
 */
static void
output_xml_to_file( const gchar *xml, const gchar *filename, GSList **msg )
{
	static const gchar *thisfn = "fma_xml_writer_output_xml_to_file";
	GFile *file;
	GFileOutputStream *stream;
	GError *error = NULL;
	gchar *errmsg;

	g_return_if_fail( xml );
	g_return_if_fail( filename && g_utf8_strlen( filename, -1 ));

	g_debug( "%s: filename=%s", thisfn, filename );

	file = g_file_new_for_uri( filename );

	stream = g_file_replace( file, NULL, FALSE, G_FILE_CREATE_NONE, NULL, &error );
	if( error ){
		errmsg = g_strdup_printf( "%s: g_file_replace: %s", thisfn, error->message );
		g_warning( "%s", errmsg );
		if( msg ){
			*msg = g_slist_append( *msg, errmsg );
		}
		g_error_free( error );
		if( stream ){
			g_object_unref( stream );
		}
		g_object_unref( file );
		return;
	}

	g_output_stream_write( G_OUTPUT_STREAM( stream ), xml, g_utf8_strlen( xml, -1 ), NULL, &error );
	if( error ){
		errmsg = g_strdup_printf( "%s: g_output_stream_write: %s", thisfn, error->message );
		g_warning( "%s", errmsg );
		if( msg ){
			*msg = g_slist_append( *msg, errmsg );
		}
		g_error_free( error );
		g_object_unref( stream );
		g_object_unref( file );
		return;
	}

	g_output_stream_close( G_OUTPUT_STREAM( stream ), NULL, &error );
	if( error ){
		errmsg = g_strdup_printf( "%s: g_output_stream_close: %s", thisfn, error->message );
		g_warning( "%s", errmsg );
		if( msg ){
			*msg = g_slist_append( *msg, errmsg );
		}
		g_error_free( error );
		g_object_unref( stream );
		g_object_unref( file );
		return;
	}

	g_object_unref( stream );
	g_object_unref( file );
}

static guint
writer_to_buffer( FMAXMLWriter *writer )
{
	guint code;
	xmlDocPtr doc;
	xmlChar *text;
	int textlen;

	code = FMA_IEXPORTER_CODE_OK;
	doc = build_xml_doc( writer );

	xmlDocDumpFormatMemoryEnc( doc, &text, &textlen, "UTF-8", 1 );
	writer->private->buffer = g_strdup(( const gchar * ) text );

	xmlFree( text );
	xmlFreeDoc (doc);
	xmlCleanupParser();

	return( code );
}
