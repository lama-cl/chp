/* Icecast
 *
 * This program is distributed under the GNU General Public License, version 2.
 * A copy of this license is included with this source.
 *
 * Copyright 2000-2004, Jack Moffitt <jack@xiph.org, 
 *                      Michael Smith <msmith@xiph.org>,
 *                      oddsock <oddsock@xiph.org>,
 *                      Karl Heyes <karl@xiph.org>
 *                      and others (see AUTHORS for details).
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "cfgfile.h"
#include "connection.h"
#include "refbuf.h"
#include "client.h"
#include "source.h"
#include "global.h"
#include "event.h"
#include "stats.h"
#include "compat.h"
#include "xslt.h"
#include "fserve.h"
#include "admin.h"

#include "format.h"

#include "logging.h"
#include "auth.h"
#ifdef _WIN32
#define snprintf _snprintf
#endif

#define CATMODULE "admin"

#define COMMAND_ERROR             (-1)

#pragma CHECKED_SCOPE on

/* Mount-specific commands */
#define COMMAND_RAW_FALLBACK        1
#define COMMAND_RAW_METADATA_UPDATE     2
#define COMMAND_RAW_SHOW_LISTENERS  3
#define COMMAND_RAW_MOVE_CLIENTS    4
#define COMMAND_RAW_MANAGEAUTH      5
#define COMMAND_SHOUTCAST_METADATA_UPDATE     6
#define COMMAND_RAW_UPDATEMETADATA      7

#define COMMAND_TRANSFORMED_FALLBACK        50
#define COMMAND_TRANSFORMED_SHOW_LISTENERS  53
#define COMMAND_TRANSFORMED_MOVE_CLIENTS    54
#define COMMAND_TRANSFORMED_MANAGEAUTH      55
#define COMMAND_TRANSFORMED_UPDATEMETADATA  56
#define COMMAND_TRANSFORMED_METADATA_UPDATE 57

/* Global commands */
#define COMMAND_RAW_LIST_MOUNTS             101
#define COMMAND_RAW_STATS                   102
#define COMMAND_RAW_LISTSTREAM              103
#define COMMAND_PLAINTEXT_LISTSTREAM        104
#define COMMAND_TRANSFORMED_LIST_MOUNTS     201
#define COMMAND_TRANSFORMED_STATS           202
#define COMMAND_TRANSFORMED_LISTSTREAM      203

/* Client management commands */
#define COMMAND_RAW_KILL_CLIENT             301
#define COMMAND_RAW_KILL_SOURCE             302
#define COMMAND_TRANSFORMED_KILL_CLIENT     401
#define COMMAND_TRANSFORMED_KILL_SOURCE     402

/* Admin commands requiring no auth */
#define COMMAND_BUILDM3U                    501

#define FALLBACK_RAW_REQUEST "fallbacks"
#define FALLBACK_TRANSFORMED_REQUEST "fallbacks.xsl"
#define SHOUTCAST_METADATA_REQUEST "admin.cgi"
#define METADATA_RAW_REQUEST "metadata"
#define METADATA_TRANSFORMED_REQUEST "metadata.xsl"
#define LISTCLIENTS_RAW_REQUEST "listclients"
#define LISTCLIENTS_TRANSFORMED_REQUEST "listclients.xsl"
#define STATS_RAW_REQUEST "stats"
#define STATS_TRANSFORMED_REQUEST "stats.xsl"
#define LISTMOUNTS_RAW_REQUEST "listmounts"
#define LISTMOUNTS_TRANSFORMED_REQUEST "listmounts.xsl"
#define STREAMLIST_RAW_REQUEST "streamlist"
#define STREAMLIST_TRANSFORMED_REQUEST "streamlist.xsl"
#define STREAMLIST_PLAINTEXT_REQUEST "streamlist.txt"
#define MOVECLIENTS_RAW_REQUEST "moveclients"
#define MOVECLIENTS_TRANSFORMED_REQUEST "moveclients.xsl"
#define KILLCLIENT_RAW_REQUEST "killclient"
#define KILLCLIENT_TRANSFORMED_REQUEST "killclient.xsl"
#define KILLSOURCE_RAW_REQUEST "killsource"
#define KILLSOURCE_TRANSFORMED_REQUEST "killsource.xsl"
#define ADMIN_XSL_RESPONSE "response.xsl"
#define MANAGEAUTH_RAW_REQUEST "manageauth"
#define MANAGEAUTH_TRANSFORMED_REQUEST "manageauth.xsl"
#define UPDATEMETADATA_RAW_REQUEST "updatemetadata"
#define UPDATEMETADATA_TRANSFORMED_REQUEST "updatemetadata.xsl"
#define DEFAULT_RAW_REQUEST ""
#define DEFAULT_TRANSFORMED_REQUEST ""
#define BUILDM3U_RAW_REQUEST "buildm3u"


int admin_get_command(const char *command : itype(_Nt_array_ptr<const char>))
{
    if(!strcmp(command, FALLBACK_RAW_REQUEST))
        return COMMAND_RAW_FALLBACK;
    else if(!strcmp(command, FALLBACK_TRANSFORMED_REQUEST))
        return COMMAND_TRANSFORMED_FALLBACK;
    else if(!strcmp(command, METADATA_RAW_REQUEST))
        return COMMAND_RAW_METADATA_UPDATE;
    else if(!strcmp(command, METADATA_TRANSFORMED_REQUEST))
        return COMMAND_TRANSFORMED_METADATA_UPDATE;
    else if(!strcmp(command, SHOUTCAST_METADATA_REQUEST))
        return COMMAND_SHOUTCAST_METADATA_UPDATE;
    else if(!strcmp(command, LISTCLIENTS_RAW_REQUEST))
        return COMMAND_RAW_SHOW_LISTENERS;
    else if(!strcmp(command, LISTCLIENTS_TRANSFORMED_REQUEST))
        return COMMAND_TRANSFORMED_SHOW_LISTENERS;
    else if(!strcmp(command, STATS_RAW_REQUEST))
        return COMMAND_RAW_STATS;
    else if(!strcmp(command, STATS_TRANSFORMED_REQUEST))
        return COMMAND_TRANSFORMED_STATS;
    else if(!strcmp(command, "stats.xml")) /* The old way */
        return COMMAND_RAW_STATS;
    else if(!strcmp(command, LISTMOUNTS_RAW_REQUEST))
        return COMMAND_RAW_LIST_MOUNTS;
    else if(!strcmp(command, LISTMOUNTS_TRANSFORMED_REQUEST))
        return COMMAND_TRANSFORMED_LIST_MOUNTS;
    else if(!strcmp(command, STREAMLIST_RAW_REQUEST))
        return COMMAND_RAW_LISTSTREAM;
    else if(!strcmp(command, STREAMLIST_PLAINTEXT_REQUEST))
        return COMMAND_PLAINTEXT_LISTSTREAM;
    else if(!strcmp(command, MOVECLIENTS_RAW_REQUEST))
        return COMMAND_RAW_MOVE_CLIENTS;
    else if(!strcmp(command, MOVECLIENTS_TRANSFORMED_REQUEST))
        return COMMAND_TRANSFORMED_MOVE_CLIENTS;
    else if(!strcmp(command, KILLCLIENT_RAW_REQUEST))
        return COMMAND_RAW_KILL_CLIENT;
    else if(!strcmp(command, KILLCLIENT_TRANSFORMED_REQUEST))
        return COMMAND_TRANSFORMED_KILL_CLIENT;
    else if(!strcmp(command, KILLSOURCE_RAW_REQUEST))
        return COMMAND_RAW_KILL_SOURCE;
    else if(!strcmp(command, KILLSOURCE_TRANSFORMED_REQUEST))
        return COMMAND_TRANSFORMED_KILL_SOURCE;
    else if(!strcmp(command, MANAGEAUTH_RAW_REQUEST))
        return COMMAND_RAW_MANAGEAUTH;
    else if(!strcmp(command, MANAGEAUTH_TRANSFORMED_REQUEST))
        return COMMAND_TRANSFORMED_MANAGEAUTH;
    else if(!strcmp(command, UPDATEMETADATA_RAW_REQUEST))
        return COMMAND_RAW_UPDATEMETADATA;
    else if(!strcmp(command, UPDATEMETADATA_TRANSFORMED_REQUEST))
        return COMMAND_TRANSFORMED_UPDATEMETADATA;
    else if(!strcmp(command, BUILDM3U_RAW_REQUEST))
        return COMMAND_BUILDM3U;
    else if(!strcmp(command, DEFAULT_TRANSFORMED_REQUEST))
        return COMMAND_TRANSFORMED_STATS;
    else if(!strcmp(command, DEFAULT_RAW_REQUEST))
        return COMMAND_TRANSFORMED_STATS;
    else
        return COMMAND_ERROR;
}

static void command_fallback(_Ptr<client_t> client, _Ptr<source_t> source, int response);
static void command_metadata(_Ptr<client_t> client, _Ptr<source_t> source, int response);
static void command_shoutcast_metadata(_Ptr<client_t> client, _Ptr<source_t> source);
static void command_show_listeners(_Ptr<client_t> client, _Ptr<source_t> source, int response);
static void command_move_clients(_Ptr<client_t> client, _Ptr<source_t> source, int response);
static void command_stats(_Ptr<client_t> client, _Nt_array_ptr<const char> mount, int response);
static void command_list_mounts(_Ptr<client_t> client, int response);
static void command_kill_client(_Ptr<client_t> client, _Ptr<source_t> source, int response);
static void command_manageauth(_Ptr<client_t> client, _Ptr<source_t> source, int response);
static void command_buildm3u(_Ptr<client_t> client, _Nt_array_ptr<const char> mount);
static void command_kill_source(_Ptr<client_t> client, _Ptr<source_t> source, int response);
static void command_updatemetadata(_Ptr<client_t> client, _Ptr<source_t> source, int response);
static void admin_handle_mount_request(_Ptr<client_t> client, _Ptr<source_t> source, int command);
static void admin_handle_general_request(_Ptr<client_t> client, int command);

/* build an XML doc containing information about currently running sources.
 * If a mountpoint is passed then that source will not be added to the XML
 * doc even if the source is running */
xmlDocPtr admin_build_sourcelist (const char *mount : itype(_Nt_array_ptr<const char>))
{
    _Ptr<avl_node> node = ((void *)0);
    _Ptr<source_t> source = ((void *)0);
    xmlNodePtr xmlnode = NULL, srcnode = NULL;
    xmlDocPtr doc = NULL;
    char buf _Nt_checked[22] = { '\0' };
    time_t now = time(NULL);

    doc = xmlNewDoc (XMLSTR("1.0"));
    xmlnode = xmlNewDocNode (doc, NULL, XMLSTR("icestats"), NULL);
    xmlDocSetRootElement(doc, xmlnode);

    if (mount) {
        xmlNewTextChild (xmlnode, NULL, XMLSTR("current_source"), XMLSTR(mount));
    }

    node = avl_get_first(global.source_tree);
    while(node) {
        source = avl_get<source_t>(node);
        if (mount && strcmp (mount, source->mount) == 0)
        {
            node = avl_get_next (node);
            continue;
        }

        if (source->running || source->on_demand)
        {
            _Ptr<ice_config_t> config = ((void *)0);
            _Ptr<mount_proxy> mountinfo = ((void *)0);

            srcnode = xmlNewChild(xmlnode, NULL, XMLSTR("source"), NULL);
            xmlSetProp(srcnode, XMLSTR("mount"),XMLSTR(source->mount));


            if (source->fallback_mount) 
              xmlNewTextChild(srcnode, NULL, XMLSTR("fallback"), XMLSTR(source->fallback_mount));
            else 
              xmlNewTextChild(srcnode, NULL, XMLSTR("fallback"), NULL);

            snprintf (buf, sizeof(buf), "%lu", source->listeners);
            xmlNewTextChild(srcnode, NULL, XMLSTR("listeners"), XMLSTR(buf));

            config = config_get_config();
            mountinfo = config_find_mount (config, source->mount, MOUNT_TYPE_NORMAL);
            if (mountinfo && mountinfo->auth)
            {
                xmlNewTextChild(srcnode, NULL, XMLSTR("authenticator"),
                        XMLSTR(mountinfo->auth->type));
            }
            config_release_config();

            if (source->running)
            {
                if (source->client) 
                {
                    snprintf (buf, sizeof(buf), "%lu",
                            (unsigned long)(now - source->con->con_time));
                    xmlNewTextChild (srcnode, NULL, XMLSTR("Connected"), XMLSTR(buf));
                }
                xmlNewTextChild (srcnode, NULL, XMLSTR("content-type"), 
                        XMLSTR(source->format->contenttype));
            }
        }
        node = avl_get_next(node);
    }
    return(doc);
}

void admin_send_response (xmlDocPtr doc : itype(_Ptr<xmlDoc>), client_t *client : itype(_Ptr<client_t>), int response, const char *xslt_template : itype(_Nt_array_ptr<const char>) byte_count(8))
{
    if (response == RAW)
    {
        _Ptr<_Nt_array_ptr<xmlChar>> pbuff = malloc<_Nt_array_ptr<xmlChar>>(sizeof *pbuff);
        _Nt_array_ptr<xmlChar> buff = NULL;
        int len = 0;
        size_t buf_len;
        ssize_t ret;

        xmlDocDumpMemory(doc, pbuff, &len);
        buff = *pbuff;
        

        buf_len = len + 1024;
        if (buf_len < 4096)
            buf_len = 4096;

        client_set_queue (client, NULL);
        client->refbuf = refbuf_new (buf_len);

	ret = util_http_build_header(client->refbuf->data, buf_len, 0,
	                             0, 200, NULL,
				     "text/xml", "utf-8",
				     NULL, NULL);
        if (ret == -1) {
            ICECAST_LOG_ERROR("Dropping client as we can not build response headers.");
            client_send_500(client, "Header generation failed.");
            xmlSafeFree((_Nt_array_ptr<char>)buff);
            return;
        } else if (buf_len < (len + ret + 64)) {
            buf_len = ret + len + 64;
            _Nt_array_ptr<char> new_data : count(buf_len) = strrealloc(client->refbuf->data, buf_len);
            if (new_data) {
                ICECAST_LOG_DEBUG("Client buffer reallocation succeeded.");
                client->refbuf->data = new_data, client->refbuf->len = buf_len;
                ret = util_http_build_header(client->refbuf->data, buf_len, 0,
                                             0, 200, NULL,
                                             "text/xml", "utf-8",
                                             NULL, NULL);
                if (ret == -1) {
                    ICECAST_LOG_ERROR("Dropping client as we can not build response headers.");
                    client_send_500(client, "Header generation failed.");
                    xmlSafeFree((_Nt_array_ptr<char>)buff);
                    return;
                }
            } else {
                ICECAST_LOG_ERROR("Client buffer reallocation failed. Dropping client.");
                client_send_500(client, "Buffer reallocation failed.");
                xmlSafeFree((_Nt_array_ptr<char>)buff);
                return;
            } 
        }

        /* FIXME: in this section we hope no function will ever return -1 */
	      ret += snprintf (client->refbuf->data + ret, buf_len - ret, "Content-Length: %d\r\n\r\n%s", xmlStrlen(buff), buff);

        client->refbuf->data = _Dynamic_bounds_cast<_Nt_array_ptr<char>>(client->refbuf->data, count(ret)),  client->refbuf->len = ret;
        
        xmlSafeFree((_Nt_array_ptr<char>)buff);
        client->respcode = 200;
        fserve_add_client (client, NULL);
    }
    if (response == TRANSFORMED)
    {
        //_Nt_array_ptr<char> fullpath_xslt_template = NULL;
        unsigned int fullpath_xslt_template_len;
        _Ptr<ice_config_t> config = config_get_config();

        fullpath_xslt_template_len = strlen (config->adminroot_dir) + 
            strlen (xslt_template) + 2;
        _Nt_array_ptr<char> fullpath_xslt_template : count(fullpath_xslt_template_len) = stralloc(fullpath_xslt_template_len); 
        snprintf(fullpath_xslt_template, fullpath_xslt_template_len, "%s%s%s",
            config->adminroot_dir, PATH_SEPARATOR, xslt_template);
        config_release_config();


        ICECAST_LOG_DEBUG("Sending XSLT (%s)", fullpath_xslt_template);
        xslt_transform(doc, fullpath_xslt_template, client);
        free<char>(fullpath_xslt_template);
    }
}


void admin_handle_request(client_t *client : itype(_Ptr<client_t>), const char *uri : itype(_Nt_array_ptr<const char>) count(0))
{
    _Nt_array_ptr<const char> mount = ((void *)0);
_Nt_array_ptr<const char> command_string = ((void *)0);

    int command;

    ICECAST_LOG_DEBUG("Admin request (%s)", uri);
    if (!((strcmp(uri, "/admin.cgi") == 0) ||
         (strncmp("/admin/", uri, 7) == 0))) {
        ICECAST_LOG_ERROR("Internal error: admin request isn't");
        client_send_401(client);
        return;
    }

    if (strcmp(uri, "/admin.cgi") == 0) {
        command_string = uri + 1;
    }
    else {
        command_string = uri + 7;
    }

    ICECAST_LOG_DEBUG("Got command (%s)", command_string);
    command = admin_get_command(command_string);

    if(command < 0) {
        ICECAST_LOG_ERROR("Error parsing command string or unrecognised command: %s",
                command_string);
        client_send_400(client, "Unrecognised command");
        return;
    }

    if (command == COMMAND_SHOUTCAST_METADATA_UPDATE) {

        _Ptr<ice_config_t> config = ((void *)0);
        _Nt_array_ptr<const char> sc_mount = NULL;
        _Nt_array_ptr<const char> pass = (_Nt_array_ptr<char>) httpp_get_query_param (client->parser, "pass");
        _Ptr<listener_t> listener = ((void *)0);

        if (pass == NULL)
        {
            client_send_400 (client, "missing pass parameter");
            return;
        }
        global_lock();
        config = config_get_config ();
        sc_mount = config->shoutcast_mount;
        listener = config_get_listen_sock (config, client->con);

        if (listener && listener->shoutcast_mount)
            sc_mount = listener->shoutcast_mount;

        httpp_set_query_param (client->parser, "mount", sc_mount);
        httpp_setvar (client->parser, HTTPP_VAR_PROTOCOL, "ICY");
        httpp_setvar (client->parser, HTTPP_VAR_ICYPASSWORD, pass);
        config_release_config ();
        global_unlock();
    }

    mount = (_Nt_array_ptr<char>) httpp_get_query_param(client->parser, "mount");

    if(mount != NULL) {
        _Ptr<source_t> source = ((void *)0);

        /* this request does not require auth but can apply to files on webroot */
        if (command == COMMAND_BUILDM3U)
        {
            command_buildm3u (client, mount);
            return;
        }
        /* This is a mount request, handle it as such */
        if (client->authenticated == 0 && !connection_check_admin_pass(client->parser))
        {
            switch (client_check_source_auth (client, mount))
            {
                case 0:
                    break;
                default:
                    ICECAST_LOG_INFO("Bad or missing password on mount modification admin "
                            "request (command: %s)", command_string);
                    client_send_401(client);
                    /* fall through */
                case 1:
                    return;
            }
        }

        avl_tree_rlock(global.source_tree);
        source = source_find_mount_raw(mount);

        if (source == NULL)
        {
            ICECAST_LOG_WARN("Admin command %s on non-existent source %s", 
                    command_string, mount);
            avl_tree_unlock(global.source_tree);
            client_send_400(client, "Source does not exist");
        }
        else
        {
            if (source->running == 0 && source->on_demand == 0)
            {
                avl_tree_unlock (global.source_tree);
                ICECAST_LOG_INFO("Received admin command %s on unavailable mount \"%s\"",
                        command_string, mount);
                client_send_400 (client, "Source is not available");
                return;
            }
            if (command == COMMAND_SHOUTCAST_METADATA_UPDATE &&
                    source->shoutcast_compat == 0)
            {
                avl_tree_unlock (global.source_tree);
                ICECAST_LOG_ERROR("illegal change of metadata on non-shoutcast "
                        "compatible stream");
                client_send_400 (client, "illegal metadata call");
                return;
            }
            ICECAST_LOG_INFO("Received admin command %s on mount \"%s\"", 
                    command_string, mount);
            admin_handle_mount_request(client, source, command);
            avl_tree_unlock(global.source_tree);
        }
    }
    else {

        if (command == COMMAND_PLAINTEXT_LISTSTREAM) {
        /* this request is used by a slave relay to retrieve
           mounts from the master, so handle this request
           validating against the relay password */
            if(!connection_check_relay_pass(client->parser)) {
                ICECAST_LOG_INFO("Bad or missing password on admin command "
                      "request (command: %s)", command_string);
                client_send_401(client);
                return;
            }
        }
        else {
            if(!connection_check_admin_pass(client->parser)) {
                ICECAST_LOG_INFO("Bad or missing password on admin command "
                      "request (command: %s)", command_string);
                client_send_401(client);
                return;
            }
        }
        
        admin_handle_general_request(client, command);
    }
}

static void admin_handle_general_request(_Ptr<client_t> client, int command)
{
    switch(command) {
        case COMMAND_RAW_STATS:
            command_stats(client, NULL, RAW);
            break;
        case COMMAND_RAW_LIST_MOUNTS:
            command_list_mounts(client, RAW);
            break;
        case COMMAND_RAW_LISTSTREAM:
            command_list_mounts(client, RAW);
            break;
        case COMMAND_PLAINTEXT_LISTSTREAM:
            command_list_mounts(client, PLAINTEXT);
            break;
        case COMMAND_TRANSFORMED_STATS:
            command_stats(client, NULL, TRANSFORMED);
            break;
        case COMMAND_TRANSFORMED_LIST_MOUNTS:
            command_list_mounts(client, TRANSFORMED);
            break;
        case COMMAND_TRANSFORMED_LISTSTREAM:
            command_list_mounts(client, TRANSFORMED);
            break;
        case COMMAND_TRANSFORMED_MOVE_CLIENTS:
            command_list_mounts(client, TRANSFORMED);
            break;
        default:
            ICECAST_LOG_WARN("General admin request not recognised");
            client_send_400(client, "Unknown admin request");
            return;
    }
}

static void admin_handle_mount_request(_Ptr<client_t> client, _Ptr<source_t> source, int command)
{
    switch(command) {
        case COMMAND_RAW_STATS:
            command_stats(client, source->mount, RAW);
            break;
        case COMMAND_RAW_FALLBACK:
            command_fallback(client, source, RAW);
            break;
        case COMMAND_RAW_METADATA_UPDATE:
            command_metadata(client, source, RAW);
            break;
        case COMMAND_TRANSFORMED_METADATA_UPDATE:
            command_metadata(client, source, TRANSFORMED);
            break;
        case COMMAND_SHOUTCAST_METADATA_UPDATE:
            command_shoutcast_metadata(client, source);
            break;
        case COMMAND_RAW_SHOW_LISTENERS:
            command_show_listeners(client, source, RAW);
            break;
        case COMMAND_RAW_MOVE_CLIENTS:
            command_move_clients(client, source, RAW);
            break;
        case COMMAND_RAW_KILL_CLIENT:
            command_kill_client(client, source, RAW);
            break;
        case COMMAND_RAW_KILL_SOURCE:
            command_kill_source(client, source, RAW);
            break;
        case COMMAND_TRANSFORMED_STATS:
            command_stats(client, source->mount, TRANSFORMED);
            break;
        case COMMAND_TRANSFORMED_FALLBACK:
            command_fallback(client, source, RAW);
            break;
        case COMMAND_TRANSFORMED_SHOW_LISTENERS:
            command_show_listeners(client, source, TRANSFORMED);
            break;
        case COMMAND_TRANSFORMED_MOVE_CLIENTS:
            command_move_clients(client, source, TRANSFORMED);
            break;
        case COMMAND_TRANSFORMED_KILL_CLIENT:
            command_kill_client(client, source, TRANSFORMED);
            break;
        case COMMAND_TRANSFORMED_KILL_SOURCE:
            command_kill_source(client, source, TRANSFORMED);
            break;
        case COMMAND_TRANSFORMED_MANAGEAUTH:
            command_manageauth(client, source, TRANSFORMED);
            break;
        case COMMAND_RAW_MANAGEAUTH:
            command_manageauth(client, source, RAW);
            break;
        case COMMAND_TRANSFORMED_UPDATEMETADATA:
            command_updatemetadata(client, source, TRANSFORMED);
            break;
        case COMMAND_RAW_UPDATEMETADATA:
            command_updatemetadata(client, source, RAW);
            break;
        default:
            ICECAST_LOG_WARN("Mount request not recognised");
            client_send_400(client, "Mount request unknown");
            break;
    }
}

#define COMMAND_REQUIRE(client,name,var) \
    do { \
        (var) = (_Nt_array_ptr<char>) httpp_get_query_param((client)->parser, (name)); \
        if((var) == NULL) { \
            client_send_400((client), "Missing parameter"); \
            return; \
        } \
    } while(0);
#define COMMAND_OPTIONAL(client,name,var) \
    (var) = (_Nt_array_ptr<char>) httpp_get_query_param((client)->parser, (name))

static void html_success(_Ptr<client_t> client, _Nt_array_ptr<char> message)
{
    ssize_t ret;

    ret = util_http_build_header(client->refbuf->data, PER_CLIENT_REFBUF_SIZE, 0,
                                 0, 200, NULL,
				 "text/html", "utf-8",
				 "", NULL);

    if (ret == -1 || ret >= PER_CLIENT_REFBUF_SIZE) {
        ICECAST_LOG_ERROR("Dropping client as we can not build response headers.");
        client_send_500(client, "Header generation failed.");
        return;
    }

    snprintf(client->refbuf->data + ret, PER_CLIENT_REFBUF_SIZE - ret,
             "<html><head><title>Admin request successful</title></head>"
	     "<body><p>%s</p></body></html>", message);

    client->respcode = 200;
    refbuf_widen(client->refbuf);
    fserve_add_client (client, NULL);
}


static void command_move_clients(_Ptr<client_t> client, _Ptr<source_t> source, int response)
{
    _Nt_array_ptr<const char> dest_source = ((void *)0);
    _Ptr<source_t> dest = ((void *)0);
    xmlDocPtr doc = NULL;
    xmlNodePtr node = NULL;
    char buf _Nt_checked [255] : count(255) = { '\0' };
    int parameters_passed = 0;

    ICECAST_LOG_DEBUG("Doing optional check");
    if((COMMAND_OPTIONAL(client, "destination", dest_source))) {
        parameters_passed = 1;
    }
    ICECAST_LOG_DEBUG("Done optional check (%d)", parameters_passed);
    if (!parameters_passed) {
        doc = admin_build_sourcelist(source->mount);
        admin_send_response(doc, client, response, 
             MOVECLIENTS_TRANSFORMED_REQUEST);
        xmlFreeDoc(doc);
        return;
    }

    dest = source_find_mount (dest_source);

    if (dest == NULL)
    {
        client_send_400 (client, "No such destination");
        return;
    }

    if (strcmp (dest->mount, source->mount) == 0)
    {
        client_send_400 (client, "supplied mountpoints are identical");
        return;
    }

    if (dest->running == 0 && dest->on_demand == 0)
    {
        client_send_400 (client, "Destination not running");
        return;
    }

    ICECAST_LOG_INFO("source is \"%s\", destination is \"%s\"", source->mount, dest->mount);

    doc = xmlNewDoc (XMLSTR("1.0"));
    node = xmlNewDocNode(doc, NULL, XMLSTR("iceresponse"), NULL);
    xmlDocSetRootElement(doc, node);

    source_move_clients (source, dest);

    memset(buf, 0, sizeof(buf));
    snprintf (buf, sizeof(buf), "Clients moved from %s to %s",
            source->mount, dest_source);
    xmlNewTextChild(node, NULL, XMLSTR("message"), XMLSTR(buf));
    xmlNewTextChild(node, NULL, XMLSTR("return"), XMLSTR("1"));

    admin_send_response(doc, client, response, 
        ADMIN_XSL_RESPONSE);
    xmlFreeDoc(doc);
}

static void command_show_listeners(_Ptr<client_t> client, _Ptr<source_t> source, int response)
{
    xmlDocPtr doc = NULL;
    xmlNodePtr node = NULL, srcnode = NULL, listenernode = NULL;
    _Ptr<avl_node> client_node = ((void *)0);
    _Ptr<client_t> current = ((void *)0);
    char buf _Nt_checked[22] : count(22);
    _Nt_array_ptr<const char> userAgent = NULL;
    time_t now = time(NULL);

    doc = xmlNewDoc (XMLSTR("1.0"));
    node = xmlNewDocNode(doc, NULL, XMLSTR("icestats"), NULL);
    srcnode = xmlNewChild(node, NULL, XMLSTR("source"), NULL);
    xmlSetProp(srcnode, XMLSTR("mount"), XMLSTR(source->mount));
    xmlDocSetRootElement(doc, node);

    memset(buf, 0, sizeof(buf));
    snprintf (buf, sizeof(buf), "%lu", source->listeners);
    xmlNewTextChild(srcnode, NULL, XMLSTR("Listeners"), XMLSTR(buf));

    avl_tree_rlock(source->client_tree);

    client_node = avl_get_first(source->client_tree);
    while(client_node) {
        current = avl_get<client_t>(client_node);
        listenernode = xmlNewChild(srcnode, NULL, XMLSTR("listener"), NULL);
        xmlNewTextChild(listenernode, NULL, XMLSTR("IP"),XMLSTR(current->con->ip));
        userAgent = (_Nt_array_ptr<char>) httpp_getvar(current->parser, "user-agent");
        if (userAgent) {
            xmlNewTextChild(listenernode, NULL, XMLSTR("UserAgent"), XMLSTR(userAgent));
        }
        else {
            xmlNewTextChild(listenernode, NULL, XMLSTR("UserAgent"), XMLSTR("Unknown"));
        }
        memset(buf, 0, sizeof(buf));
        snprintf(buf, sizeof(buf), "%lu", (unsigned long)(now - current->con->con_time));
        xmlNewTextChild(listenernode, NULL, XMLSTR("Connected"), XMLSTR(buf));
        memset(buf, 0, sizeof(buf));
        snprintf(buf, sizeof(buf)-1, "%lu", current->con->id);
        xmlNewTextChild(listenernode, NULL, XMLSTR("ID"), XMLSTR(buf));
        if (current->username) {
            xmlNewTextChild(listenernode, NULL, XMLSTR("username"), XMLSTR(current->username));
        }
        client_node = avl_get_next(client_node);
    }

    avl_tree_unlock(source->client_tree);
    admin_send_response(doc, client, response, 
        LISTCLIENTS_TRANSFORMED_REQUEST);
    xmlFreeDoc(doc);
}

static void command_buildm3u(_Ptr<client_t> client, _Nt_array_ptr<const char> mount)
{
    _Nt_array_ptr<const char> username = NULL;
    _Nt_array_ptr<const char> password = NULL;
    _Ptr<ice_config_t> config = ((void *)0);
    ssize_t ret;

    COMMAND_REQUIRE(client, "username", username);
    COMMAND_REQUIRE(client, "password", password);

    ret = util_http_build_header(client->refbuf->data, PER_CLIENT_REFBUF_SIZE, 0,
                                 0, 200, NULL,
				 "audio/x-mpegurl", NULL,
				 NULL, NULL);

    if (ret == -1 || ret >= (PER_CLIENT_REFBUF_SIZE - 512)) { /* we want at least 512 Byte left for data */
        ICECAST_LOG_ERROR("Dropping client as we can not build response headers.");
        client_send_500(client, "Header generation failed.");
        return;
    }


    config = config_get_config();
    snprintf (client->refbuf->data + ret, PER_CLIENT_REFBUF_SIZE - ret,
        "Content-Disposition = attachment; filename=listen.m3u\r\n\r\n" 
        "http://%s:%s@%s:%d%s\r\n",
        username,
        password,
        config->hostname,
        config->port,
        mount
    );
    config_release_config();

    client->respcode = 200;
    refbuf_widen(client->refbuf);
    fserve_add_client (client, NULL);
}


static void command_manageauth(_Ptr<client_t> client, _Ptr<source_t> source, int response)
{
    xmlDocPtr doc = NULL;
    xmlNodePtr node = NULL, srcnode = NULL, msgnode = NULL;
    _Nt_array_ptr<const char> action = NULL;
    _Nt_array_ptr<const char> username = NULL;
    _Nt_array_ptr<char> message = NULL;
    int ret = AUTH_OK;
    _Ptr<ice_config_t> config = config_get_config ();
    _Ptr<mount_proxy> mountinfo = config_find_mount (config, source->mount, MOUNT_TYPE_NORMAL);

    do
    {
        if (mountinfo == NULL || mountinfo->auth == NULL)
        {
            ICECAST_LOG_WARN("manage auth request for %s but no facility available", source->mount);
            break;
        }
        COMMAND_OPTIONAL(client, "action", action);
        COMMAND_OPTIONAL (client, "username", username);

        if (action == NULL)
            action = "list";

        if (!strcmp(action, "add"))
        {
            _Nt_array_ptr<const char> password = NULL;
            COMMAND_OPTIONAL (client, "password", password);

            if (username == NULL || password == NULL)
            {
                ICECAST_LOG_WARN("manage auth request add for %s but no user/pass", source->mount);
                break;
            }
            _Checked {
            ret = mountinfo->auth->adduser(mountinfo->auth, username, password);
            }
            if (ret == AUTH_FAILED) {
                message = (_Nt_array_ptr<char>) strdup("User add failed - check the icecast error log");
            }
            if (ret == AUTH_USERADDED) {
                message = (_Nt_array_ptr<char>)strdup("User added");
            }
            if (ret == AUTH_USEREXISTS) {
                message = (_Nt_array_ptr<char>)strdup("User already exists - not added");
            }
        }
        if (!strcmp(action, "delete"))
        {
            if (username == NULL)
            {
                ICECAST_LOG_WARN("manage auth request delete for %s but no username", source->mount);
                break;
            }
            _Checked {
            ret = mountinfo->auth->deleteuser(mountinfo->auth, username);
            }
            if (ret == AUTH_FAILED) {
                message = (_Nt_array_ptr<char>)strdup("User delete failed - check the icecast error log");
            }
            if (ret == AUTH_USERDELETED) {
                message = (_Nt_array_ptr<char>)strdup("User deleted");
            }
        }

        doc = xmlNewDoc (XMLSTR("1.0"));
        node = xmlNewDocNode(doc, NULL, XMLSTR("icestats"), NULL);
        srcnode = xmlNewChild(node, NULL, XMLSTR("source"), NULL);
        xmlSetProp(srcnode, XMLSTR("mount"), XMLSTR(source->mount));

        if (message) {
            msgnode = xmlNewChild(node, NULL, XMLSTR("iceresponse"), NULL);
            xmlNewTextChild(msgnode, NULL, XMLSTR("message"), XMLSTR(message));
        }

        xmlDocSetRootElement(doc, node);

        if (mountinfo && mountinfo->auth && mountinfo->auth->listuser) _Checked {
            mountinfo->auth->listuser (mountinfo->auth, srcnode);
        }

        config_release_config ();

        admin_send_response(doc, client, response, 
                MANAGEAUTH_TRANSFORMED_REQUEST);
        free<char> (message);
        xmlFreeDoc(doc);
        return;
    } while (0);

    config_release_config ();
    client_send_400 (client, "missing parameter");
}

static void command_kill_source(_Ptr<client_t> client, _Ptr<source_t> source, int response)
{
    xmlDocPtr doc =NULL;
    xmlNodePtr node = NULL;

    doc = xmlNewDoc (XMLSTR("1.0"));
    node = xmlNewDocNode(doc, NULL, XMLSTR("iceresponse"), NULL);
    xmlNewTextChild(node, NULL, XMLSTR("message"), XMLSTR("Source Removed"));
    xmlNewTextChild(node, NULL, XMLSTR("return"), XMLSTR("1"));
    xmlDocSetRootElement(doc, node);

    source->running = 0;

    admin_send_response(doc, client, response, 
        ADMIN_XSL_RESPONSE);
    xmlFreeDoc(doc);
}

static void command_kill_client(_Ptr<client_t> client, _Ptr<source_t> source, int response)
{
    _Nt_array_ptr<const char> idtext = ((void *)0);
    int id;
    _Ptr<client_t> listener = ((void *)0);
    xmlDocPtr doc = NULL;
    xmlNodePtr node = NULL;
    char buf _Nt_checked[50] : count(50) = "";

    COMMAND_REQUIRE(client, "id", idtext);

    id = atoi(idtext);

    listener = source_find_client(source, id);

    doc = xmlNewDoc (XMLSTR("1.0"));
    node = xmlNewDocNode(doc, NULL, XMLSTR("iceresponse"), NULL);
    xmlDocSetRootElement(doc, node);
    ICECAST_LOG_DEBUG("Response is %d", response);

    if(listener != NULL) {
        ICECAST_LOG_INFO("Admin request: client %d removed", id);

        /* This tags it for removal on the next iteration of the main source
         * loop
         */
        listener->con->error = 1;
        memset(buf, 0, sizeof(buf));
        snprintf(buf, sizeof(buf)-1, "Client %d removed", id);
        xmlNewTextChild(node, NULL, XMLSTR("message"), XMLSTR(buf));
        xmlNewTextChild(node, NULL, XMLSTR("return"), XMLSTR("1"));
    }
    else {
        memset(buf, 0, sizeof(buf));
        snprintf(buf, sizeof(buf)-1, "Client %d not found", id);
        xmlNewTextChild(node, NULL, XMLSTR("message"), XMLSTR(buf));
        xmlNewTextChild(node, NULL, XMLSTR("return"), XMLSTR("0"));
    }
    admin_send_response(doc, client, response, 
        ADMIN_XSL_RESPONSE);
    xmlFreeDoc(doc);
}

static void command_fallback(_Ptr<client_t> client, _Ptr<source_t> source, int response)
{
    _Nt_array_ptr<const char> fallback = ((void *)0);
    _Nt_array_ptr<char> old = NULL;

    ICECAST_LOG_DEBUG("Got fallback request");

    COMMAND_REQUIRE(client, "fallback", fallback);

    old = source->fallback_mount;
    source->fallback_mount = strdup(fallback);
    free<char>(old);

    html_success(client, "Fallback configured");
}

static void command_metadata(_Ptr<client_t> client, _Ptr<source_t> source, int response)
{
    _Nt_array_ptr<const char> action = ((void *)0);
    _Nt_array_ptr<const char> song = ((void *)0);
_Nt_array_ptr<const char> title = ((void *)0);
_Nt_array_ptr<const char> artist = ((void *)0);
_Nt_array_ptr<const char> charset = ((void *)0);

    _Ptr<format_plugin_t> plugin = ((void *)0);
    xmlDocPtr doc = NULL;
    xmlNodePtr node = NULL;
    int same_ip = 1;

    doc = xmlNewDoc (XMLSTR("1.0"));
    node = xmlNewDocNode (doc, NULL, XMLSTR("iceresponse"), NULL);
    xmlDocSetRootElement(doc, node);

    ICECAST_LOG_DEBUG("Got metadata update request");

    COMMAND_REQUIRE(client, "mode", action);
    COMMAND_OPTIONAL(client, "song", song);
    COMMAND_OPTIONAL(client, "title", title);
    COMMAND_OPTIONAL(client, "artist", artist);
    COMMAND_OPTIONAL(client, "charset", charset);

    if (strcmp (action, "updinfo") != 0)
    {
        xmlNewTextChild(node, NULL, XMLSTR("message"), XMLSTR("No such action"));
        xmlNewTextChild(node, NULL, XMLSTR("return"), XMLSTR("0"));
        admin_send_response(doc, client, response, 
            ADMIN_XSL_RESPONSE);
        xmlFreeDoc(doc);
        return;
    }

    plugin = source->format;
    if (source->client && strcmp (client->con->ip, source->client->con->ip) != 0)
        if (response == RAW && connection_check_admin_pass (client->parser) == 0)
            same_ip = 0;

    if (same_ip && plugin && plugin->set_tag)
    {
        if (song)
        {
          _Checked { 
            plugin->set_tag (plugin, "song", song, charset);
          }
            ICECAST_LOG_INFO("Metadata on mountpoint %s changed to \"%s\"", source->mount, song);
        }
        else
        {
            if (artist && title)
            {
              _Checked {
                plugin->set_tag (plugin, "title", title, charset);
                plugin->set_tag (plugin, "artist", artist, charset);
              }
                ICECAST_LOG_INFO("Metadata on mountpoint %s changed to \"%s - %s\"",
                        source->mount, artist, title);
            }
        }
        /* updates are now done, let them be pushed into the stream */
        _Checked {
        plugin->set_tag (plugin, NULL, NULL, NULL);
        }
    }
    else
    {
        xmlNewTextChild(node, NULL, XMLSTR("message"), 
            XMLSTR("Mountpoint will not accept URL updates"));
        xmlNewTextChild(node, NULL, XMLSTR("return"), XMLSTR("1"));
        admin_send_response(doc, client, response, 
            ADMIN_XSL_RESPONSE);
        xmlFreeDoc(doc);
        return;
    }

    xmlNewTextChild(node, NULL, XMLSTR("message"), XMLSTR("Metadata update successful"));
    xmlNewTextChild(node, NULL, XMLSTR("return"), XMLSTR("1"));
    admin_send_response(doc, client, response, 
        ADMIN_XSL_RESPONSE);
    xmlFreeDoc(doc);
}

static void command_shoutcast_metadata(_Ptr<client_t> client, _Ptr<source_t> source)
{
    _Nt_array_ptr<const char> action = ((void *)0);
    _Nt_array_ptr<const char> value = ((void *)0);
    int same_ip = 1;

    ICECAST_LOG_DEBUG("Got shoutcast metadata update request");

    COMMAND_REQUIRE(client, "mode", action);
    COMMAND_REQUIRE(client, "song", value);

    if (strcmp (action, "updinfo") != 0)
    {
        client_send_400 (client, "No such action");
        return;
    }
    if (source->client && strcmp (client->con->ip, source->client->con->ip) != 0)
        if (connection_check_admin_pass (client->parser) == 0)
            same_ip = 0;

    if (same_ip && source->format && source->format->set_tag)
    {
      _Checked {
        source->format->set_tag (source->format, "title", value, NULL);
        source->format->set_tag (source->format, NULL, NULL, NULL);
      }

        ICECAST_LOG_DEBUG("Metadata on mountpoint %s changed to \"%s\"", 
                source->mount, value);
        html_success(client, "Metadata update successful");
    }
    else
    {
        client_send_400 (client, "mountpoint will not accept URL updates");
    }
}

static void command_stats(_Ptr<client_t> client, _Nt_array_ptr<const char> mount, int response) {
    xmlDocPtr doc = NULL;

    ICECAST_LOG_DEBUG("Stats request, sending xml stats");

    doc = stats_get_xml(1, mount);
    admin_send_response(doc, client, response, STATS_TRANSFORMED_REQUEST);
    xmlFreeDoc(doc);
    return;
}

static void command_list_mounts(_Ptr<client_t> client, int response)
{
    ICECAST_LOG_DEBUG("List mounts request");

    if (response == PLAINTEXT)
    {
        ssize_t ret = util_http_build_header(client->refbuf->data, PER_CLIENT_REFBUF_SIZE, 0,
	                       0, 200, NULL,
			       "text/plain", "utf-8",
			       "", NULL);

        if (ret == -1 || ret >= PER_CLIENT_REFBUF_SIZE) {
            ICECAST_LOG_ERROR("Dropping client as we can not build response headers.");
            client_send_500(client, "Header generation failed.");
            return;
        }

        refbuf_widen(client->refbuf);
        client->respcode = 200;

        client->refbuf->next = stats_get_streams ();
        fserve_add_client (client, NULL);
    }
    else
    {
        xmlDocPtr doc = NULL;
        avl_tree_rlock (global.source_tree);
        doc = admin_build_sourcelist(NULL);
        avl_tree_unlock (global.source_tree);

        admin_send_response(doc, client, response, 
            LISTMOUNTS_TRANSFORMED_REQUEST);
        xmlFreeDoc(doc);
    }
}

static void command_updatemetadata(_Ptr<client_t> client, _Ptr<source_t> source, int response)
{
    xmlDocPtr doc = NULL;
    xmlNodePtr node = NULL, srcnode = NULL;

    doc = xmlNewDoc (XMLSTR("1.0"));
    node = xmlNewDocNode (doc, NULL, XMLSTR("icestats"), NULL);
    srcnode = xmlNewChild (node, NULL, XMLSTR("source"), NULL);
    xmlSetProp (srcnode, XMLSTR("mount"), XMLSTR(source->mount));
    xmlDocSetRootElement(doc, node);

    admin_send_response(doc, client, response, 
        UPDATEMETADATA_TRANSFORMED_REQUEST);
    xmlFreeDoc(doc);
}
