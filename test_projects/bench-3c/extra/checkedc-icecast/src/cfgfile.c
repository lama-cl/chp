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
 * Copyright 2011,      Dave 'justdave' Miller <justdave@mozilla.com>.
 * Copyright 2011-2014, Thomas B. "dm8tbr" Ruecker <thomas@ruecker.fi>,
 * Copyright 2011-2014, Philipp "ph3-der-loewe" Schafft <lion@lion.leolix.org>,
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <fnmatch.h>
#endif
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "thread/thread.h"
#include "cfgfile.h"
#include "refbuf.h"
#include "client.h"
#include "logging.h" 
#include "util.h"

#define CATMODULE "CONFIG"
#define CONFIG_DEFAULT_LOCATION "Earth"
#define CONFIG_DEFAULT_ADMIN "icemaster@localhost"
#define CONFIG_DEFAULT_CLIENT_LIMIT 256
#define CONFIG_DEFAULT_SOURCE_LIMIT 16
#define CONFIG_DEFAULT_QUEUE_SIZE_LIMIT (500*1024)
#define CONFIG_DEFAULT_BURST_SIZE (64*1024)
#define CONFIG_DEFAULT_THREADPOOL_SIZE 4
#define CONFIG_DEFAULT_CLIENT_TIMEOUT 30
#define CONFIG_DEFAULT_HEADER_TIMEOUT 15
#define CONFIG_DEFAULT_SOURCE_TIMEOUT 10
#define CONFIG_DEFAULT_MASTER_USERNAME "relay"
#define CONFIG_DEFAULT_SHOUTCAST_MOUNT "/stream"
#define CONFIG_DEFAULT_ICE_LOGIN 0
#define CONFIG_DEFAULT_FILESERVE 1
#define CONFIG_DEFAULT_TOUCH_FREQ 5
#define CONFIG_DEFAULT_HOSTNAME "localhost"
#define CONFIG_DEFAULT_PLAYLIST_LOG NULL
#define CONFIG_DEFAULT_ACCESS_LOG "access.log"
#define CONFIG_DEFAULT_ERROR_LOG "error.log"
#define CONFIG_DEFAULT_LOG_LEVEL 3
#define CONFIG_DEFAULT_CHROOT 0
#define CONFIG_DEFAULT_CHUID 0
#define CONFIG_DEFAULT_USER NULL
#define CONFIG_DEFAULT_GROUP NULL
#define CONFIG_MASTER_UPDATE_INTERVAL 120
#define CONFIG_YP_URL_TIMEOUT 10
#define CONFIG_DEFAULT_CIPHER_LIST "ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:DHE-DSS-AES128-GCM-SHA256:kEDH+AESGCM:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA:ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA:ECDHE-ECDSA-AES256-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA:DHE-DSS-AES128-SHA256:DHE-RSA-AES256-SHA256:DHE-DSS-AES256-SHA:DHE-RSA-AES256-SHA:ECDHE-RSA-DES-CBC3-SHA:ECDHE-ECDSA-DES-CBC3-SHA:AES128-GCM-SHA256:AES256-GCM-SHA384:AES128-SHA256:AES256-SHA256:AES128-SHA:AES256-SHA:AES:DES-CBC3-SHA:HIGH:!aNULL:!eNULL:!EXPORT:!DES:!RC4:!MD5:!PSK:!aECDH:!EDH-DSS-DES-CBC3-SHA:!EDH-RSA-DES-CBC3-SHA:!KRB5-DES-CBC3-SHA"

#ifndef _WIN32
#define CONFIG_DEFAULT_BASE_DIR "/usr/local/icecast"
#define CONFIG_DEFAULT_LOG_DIR "/usr/local/icecast/logs"
#define CONFIG_DEFAULT_WEBROOT_DIR "/usr/local/icecast/webroot"
#define CONFIG_DEFAULT_ADMINROOT_DIR "/usr/local/icecast/admin"
#define MIMETYPESFILE "/etc/mime.types"
#else
#define CONFIG_DEFAULT_BASE_DIR ".\\"
#define CONFIG_DEFAULT_LOG_DIR ".\\logs"
#define CONFIG_DEFAULT_WEBROOT_DIR ".\\webroot"
#define CONFIG_DEFAULT_ADMINROOT_DIR ".\\admin"
#define MIMETYPESFILE ".\\mime.types"
#endif

#pragma CHECKED_SCOPE on

static ice_config_t _current_configuration;
static ice_config_locks _locks;

static void _set_defaults(_Ptr<ice_config_t> c);
static void _parse_root(xmlDocPtr doc, xmlNodePtr node, _Ptr<ice_config_t> c);
static void _parse_limits(xmlDocPtr doc, xmlNodePtr node, _Ptr<ice_config_t> c);
static void _parse_directory(xmlDocPtr doc, xmlNodePtr node, _Ptr<ice_config_t> c);
static void _parse_paths(xmlDocPtr doc, xmlNodePtr node, _Ptr<ice_config_t> c);
static void _parse_logging(xmlDocPtr doc, xmlNodePtr node, _Ptr<ice_config_t> c);
static void _parse_security(xmlDocPtr doc, xmlNodePtr node, _Ptr<ice_config_t> c);
static void _parse_authentication(xmlDocPtr doc, xmlNodePtr node, _Ptr<ice_config_t> c);
static void _parse_http_headers(xmlDocPtr doc, xmlNodePtr node, _Ptr<_Ptr<ice_config_http_header_t>> http_headers);
static void _parse_relay(xmlDocPtr doc, xmlNodePtr node, _Ptr<ice_config_t> c);
static void _parse_mount(xmlDocPtr doc, xmlNodePtr node, _Ptr<ice_config_t> c);
static void _parse_listen_socket(xmlDocPtr doc, xmlNodePtr node, _Ptr<ice_config_t> c);
static void _add_server(xmlDocPtr doc, xmlNodePtr node, _Ptr<ice_config_t> c);

static void merge_mounts(_Ptr<mount_proxy> dst, _Ptr<mount_proxy> src);
static inline void _merge_mounts_all(_Ptr<ice_config_t> c);

static void create_locks(void) {
    thread_mutex_create(&_locks.relay_lock);
    thread_rwlock_create(&_locks.config_lock);
}

static void release_locks(void) {
    thread_mutex_destroy(&_locks.relay_lock);
    thread_rwlock_destroy(&_locks.config_lock);
}

void config_initialize(void) {
    create_locks();
}

void config_shutdown(void) {
    config_get_config();
    config_clear(&_current_configuration);
    config_release_config();
    release_locks();
}

void config_init_configuration(ice_config_t *configuration : itype(_Ptr<ice_config_t>))
{
  _Array_ptr<void> buffer : byte_count(sizeof(ice_config_t)) = (_Array_ptr<void>) configuration;
    memset(buffer, 0, sizeof(ice_config_t));
    _set_defaults(configuration);
}

static void config_clear_http_header(_Ptr<ice_config_http_header_t> header) {
 _Ptr<ice_config_http_header_t> old = ((void *)0);

 while (header) {
  xmlSafeFree(header->name);
  xmlSafeFree(header->value);
  old = header;
  header = header->next;
  free<ice_config_http_header_t>(old);
 }
}

static _Ptr<ice_config_http_header_t> config_copy_http_header(_Ptr<ice_config_http_header_t> header) {
    _Ptr<ice_config_http_header_t> ret = NULL;
    _Ptr<ice_config_http_header_t> cur = NULL;
    _Ptr<ice_config_http_header_t> old = NULL;

    while (header) {
        if (cur) {
            cur->next = calloc<struct ice_config_http_header_tag>(1, sizeof(ice_config_http_header_t));
            old = cur;
            cur = cur->next;
        } else {
            ret = calloc<ice_config_http_header_t>(1, sizeof(ice_config_http_header_t));
            cur = ret;
        }

        if (!cur) return ret; /* TODO: do better error handling */

        cur->type   = header->type;
        cur->name   = (_Nt_array_ptr<char>) xmlCharStrdup(header->name);
        cur->value  = (_Nt_array_ptr<char>) xmlCharStrdup(header->value);
        cur->status = header->status;

        if (!cur->name || !cur->value) {
            if (cur->name) xmlSafeFree(cur->name);
            if (cur->value) xmlSafeFree(cur->value);
            if (old) {
                old->next = NULL;
            } else {
                ret = NULL;
            }
            free<ice_config_http_header_t>(cur);
            return ret;
        }

        header = header->next;
    }

    return ret;
}

static void config_clear_mount (_Ptr<mount_proxy> mount)
{
    _Ptr<config_options_t> option = ((void *)0);

    if (mount->mountname)       xmlSafeFree (mount->mountname);
    if (mount->username)        xmlSafeFree (mount->username);
    if (mount->password)        xmlSafeFree (mount->password);
    if (mount->dumpfile)        xmlSafeFree (mount->dumpfile);
    if (mount->intro_filename)  xmlSafeFree (mount->intro_filename);
    if (mount->on_connect)      xmlSafeFree (mount->on_connect);
    if (mount->on_disconnect)   xmlSafeFree (mount->on_disconnect);
    if (mount->fallback_mount)  xmlSafeFree (mount->fallback_mount);
    if (mount->stream_name)     xmlSafeFree (mount->stream_name);
    if (mount->stream_description)  xmlSafeFree (mount->stream_description);
    if (mount->stream_url)      xmlSafeFree (mount->stream_url);
    if (mount->stream_genre)    xmlSafeFree (mount->stream_genre);
    if (mount->bitrate)         xmlSafeFree (mount->bitrate);
    if (mount->type)            xmlSafeFree (mount->type);
    if (mount->charset)         xmlSafeFree (mount->charset);
    if (mount->cluster_password)    xmlSafeFree (mount->cluster_password);

    if (mount->auth_type)       xmlSafeFree (mount->auth_type);
    option = mount->auth_options;
    while (option)
    {
        _Ptr<config_options_t> nextopt = option->next;
        if (option->name)   xmlSafeFree (option->name);
        if (option->value)  xmlSafeFree (option->value);
        free<config_options_t> (option);
        option = nextopt;
    }
    auth_release (mount->auth);
    config_clear_http_header(mount->http_headers);
    free<mount_proxy> (mount);
}

listener_t *config_clear_listener(listener_t *listener : itype(_Ptr<listener_t>)) : itype(_Ptr<listener_t>)
{
    _Ptr<listener_t> next = NULL;
    if (listener)
    {
        next = listener->next;
        if (listener->bind_address)     xmlSafeFree (listener->bind_address);
        if (listener->shoutcast_mount)  xmlSafeFree (listener->shoutcast_mount);
        free<listener_t> (listener);
    }
    return next;
}

void config_clear(ice_config_t *c : itype(_Ptr<ice_config_t>))
{
    _Ptr<ice_config_dir_t> dirnode = ((void *)0);
_Ptr<ice_config_dir_t> nextdirnode = ((void *)0);

    _Ptr<relay_server> relay = ((void *)0);
_Ptr<relay_server> nextrelay = ((void *)0);

    _Ptr<mount_proxy> mount = ((void *)0);
_Ptr<mount_proxy> nextmount = ((void *)0);

    _Ptr<aliases> alias = ((void *)0);
_Ptr<aliases> nextalias = ((void *)0);

#ifdef USE_YP
    int i;
#endif

    free<char>(c->config_filename);

    xmlSafeFree (c->server_id);
    if (c->location) xmlSafeFree(c->location);
    if (c->admin) xmlSafeFree(c->admin);
    if (c->source_password) xmlSafeFree(c->source_password);
    if (c->admin_username)
        xmlSafeFree(c->admin_username);
    if (c->admin_password)
        xmlSafeFree(c->admin_password);
    if (c->relay_username)
        xmlSafeFree(c->relay_username);
    if (c->relay_password)
        xmlSafeFree(c->relay_password);
    if (c->hostname) xmlSafeFree(c->hostname);
    if (c->base_dir) xmlSafeFree(c->base_dir);
    if (c->log_dir) xmlSafeFree(c->log_dir);
    if (c->webroot_dir) xmlSafeFree(c->webroot_dir);
    if (c->adminroot_dir) xmlSafeFree(c->adminroot_dir);
    if (c->cert_file) xmlSafeFree(c->cert_file);
    if (c->cipher_list) xmlSafeFree(c->cipher_list);
    if (c->pidfile)
        xmlSafeFree(c->pidfile);
    if (c->banfile) xmlSafeFree(c->banfile);
    if (c->allowfile) xmlSafeFree(c->allowfile);
    if (c->playlist_log) xmlSafeFree(c->playlist_log);
    if (c->access_log) xmlSafeFree(c->access_log);
    if (c->error_log) xmlSafeFree(c->error_log);
    if (c->shoutcast_mount) xmlSafeFree(c->shoutcast_mount);
    if (c->master_server) xmlSafeFree(c->master_server);
    if (c->master_username) xmlSafeFree(c->master_username);
    if (c->master_password) xmlSafeFree(c->master_password);
    if (c->user) xmlSafeFree(c->user);
    if (c->group) xmlSafeFree(c->group);
    if (c->mimetypes_fn) xmlSafeFree (c->mimetypes_fn);

    while ((c->listen_sock = config_clear_listener (c->listen_sock)))
        ;

    thread_mutex_lock(&(_locks.relay_lock));
    relay = c->relay;
    while(relay) {
        nextrelay = relay->next;
        xmlSafeFree(relay->server);
        xmlSafeFree(relay->mount);
        xmlSafeFree(relay->localmount);
        free<relay_server>(relay);
        relay = nextrelay;
    }
    thread_mutex_unlock(&(_locks.relay_lock));

    mount = c->mounts;
    while(mount) {
        nextmount = mount->next;
        config_clear_mount (mount);
        mount = nextmount;
    }

    alias = c->aliases;
    while(alias) {
        nextalias = alias->next;
        xmlSafeFree(alias->source);
        xmlSafeFree(alias->destination);
        xmlSafeFree(alias->bind_address);
        free<aliases>(alias);
        alias = nextalias;
    }

    dirnode = c->dir_list;
    while(dirnode) {
        nextdirnode = dirnode->next;
        xmlSafeFree(dirnode->host);
        free<ice_config_dir_t>(dirnode);
        dirnode = nextdirnode;
    }
#ifdef USE_YP
    i = 0;
    while (i < c->num_yp_directories)
    {
        xmlFree (c->yp_url[i]);
        i++;
    }
#endif

    config_clear_http_header(c->http_headers);

    _Array_ptr<void> buffer : byte_count(sizeof(ice_config_t)) = (_Array_ptr<void>) c;
    memset(buffer, 0, sizeof(ice_config_t));
}

int config_initial_parse_file(const char *filename : itype(_Nt_array_ptr<const char>))
{
    /* Since we're already pointing at it, we don't need to copy it in place */
    return config_parse_file(filename, &_current_configuration);
}

int config_parse_file(const char *filename : itype(_Nt_array_ptr<const char>), ice_config_t *configuration : itype(_Ptr<ice_config_t>))
{
    xmlDocPtr doc = NULL;
    xmlNodePtr node = NULL;

    if (filename == NULL || strcmp(filename, "") == 0) return CONFIG_EINSANE;
    
    doc = xmlParseFile(filename);
    if (doc == NULL) {
        return CONFIG_EPARSE;
    }

    node = xmlDocGetRootElement(doc);
    if (node == NULL) {
        xmlFreeDoc(doc);
        return CONFIG_ENOROOT;
    }

    if (xmlStrcmp (node->name, (_Nt_array_ptr<unsigned char>) "icecast") != 0) {
        xmlFreeDoc(doc);
        return CONFIG_EBADROOT;
    }

    config_init_configuration(configuration);

    configuration->config_filename = ((_Nt_array_ptr<char> )strdup (filename));

    _parse_root(doc, node->xmlChildrenNode, configuration);

    xmlFreeDoc(doc);

    _merge_mounts_all(configuration);

    return 0;
}

int config_parse_cmdline(int arg, char **argv : itype(_Ptr<_Nt_array_ptr<char>>))
{
    return 0;
}

ice_config_locks *config_locks(void) : itype(_Ptr<ice_config_locks>)
{
    return &_locks;
}

void config_release_config(void)
{
    thread_rwlock_unlock(&(_locks.config_lock));
}

ice_config_t *config_get_config(void) : itype(_Ptr<ice_config_t>)
{
    thread_rwlock_rlock(&(_locks.config_lock));
    return &_current_configuration;
}

ice_config_t *config_grab_config(void) : itype(_Ptr<ice_config_t>)
{
    thread_rwlock_wlock(&(_locks.config_lock));
    return &_current_configuration;
}

/* MUST be called with the lock held! */
void config_set_config(ice_config_t *config : itype(_Ptr<ice_config_t>)) {
    memcpy<ice_config_t>(&_current_configuration, config, sizeof(ice_config_t));
}

ice_config_t *config_get_config_unlocked(void) : itype(_Ptr<ice_config_t>)
{
    return &_current_configuration;
}

static void _set_defaults(_Ptr<ice_config_t> configuration)
{
    configuration->location = (_Nt_array_ptr<char>)xmlCharStrdup (CONFIG_DEFAULT_LOCATION);
    configuration->server_id = (_Nt_array_ptr<char>)xmlCharStrdup (ICECAST_VERSION_STRING);
    configuration->admin = (_Nt_array_ptr<char>)xmlCharStrdup (CONFIG_DEFAULT_ADMIN);
    configuration->client_limit = CONFIG_DEFAULT_CLIENT_LIMIT;
    configuration->source_limit = CONFIG_DEFAULT_SOURCE_LIMIT;
    configuration->queue_size_limit = CONFIG_DEFAULT_QUEUE_SIZE_LIMIT;
    configuration->threadpool_size = CONFIG_DEFAULT_THREADPOOL_SIZE;
    configuration->client_timeout = CONFIG_DEFAULT_CLIENT_TIMEOUT;
    configuration->header_timeout = CONFIG_DEFAULT_HEADER_TIMEOUT;
    configuration->source_timeout = CONFIG_DEFAULT_SOURCE_TIMEOUT;
    configuration->source_password = NULL;
    configuration->shoutcast_mount = (_Nt_array_ptr<char>)xmlCharStrdup (CONFIG_DEFAULT_SHOUTCAST_MOUNT);
    configuration->ice_login = CONFIG_DEFAULT_ICE_LOGIN;
    configuration->fileserve = CONFIG_DEFAULT_FILESERVE;
    configuration->touch_interval = CONFIG_DEFAULT_TOUCH_FREQ;
    configuration->on_demand = 0;
    configuration->dir_list = NULL;
    configuration->hostname = (_Nt_array_ptr<char>)xmlCharStrdup (CONFIG_DEFAULT_HOSTNAME);
    configuration->mimetypes_fn = (_Nt_array_ptr<char>)xmlCharStrdup (MIMETYPESFILE);
    configuration->master_server = NULL;
    configuration->master_server_port = 0;
    configuration->master_update_interval = CONFIG_MASTER_UPDATE_INTERVAL;
    configuration->master_username = (_Nt_array_ptr<char>)xmlCharStrdup (CONFIG_DEFAULT_MASTER_USERNAME);
    configuration->master_password = NULL;
    configuration->base_dir = (_Nt_array_ptr<char>)xmlCharStrdup (CONFIG_DEFAULT_BASE_DIR);
    configuration->log_dir = (_Nt_array_ptr<char>)xmlCharStrdup (CONFIG_DEFAULT_LOG_DIR);
    configuration->cipher_list = (_Nt_array_ptr<char>)xmlCharStrdup (CONFIG_DEFAULT_CIPHER_LIST);
    configuration->webroot_dir = (_Nt_array_ptr<char>)xmlCharStrdup (CONFIG_DEFAULT_WEBROOT_DIR);
    configuration->adminroot_dir = (_Nt_array_ptr<char>)xmlCharStrdup (CONFIG_DEFAULT_ADMINROOT_DIR);
    configuration->playlist_log = (_Nt_array_ptr<char>)xmlCharStrdup (CONFIG_DEFAULT_PLAYLIST_LOG);
    configuration->access_log = (_Nt_array_ptr<char>)xmlCharStrdup (CONFIG_DEFAULT_ACCESS_LOG);
    configuration->error_log = (_Nt_array_ptr<char>)xmlCharStrdup (CONFIG_DEFAULT_ERROR_LOG);
    configuration->loglevel = CONFIG_DEFAULT_LOG_LEVEL;
    configuration->chroot = CONFIG_DEFAULT_CHROOT;
    configuration->chuid = CONFIG_DEFAULT_CHUID;
    configuration->user = NULL;
    configuration->group = NULL;
    configuration->num_yp_directories = 0;
    configuration->relay_username = (_Nt_array_ptr<char>)xmlCharStrdup (CONFIG_DEFAULT_MASTER_USERNAME);
    configuration->relay_password = NULL;
    /* default to a typical prebuffer size used by clients */
    configuration->burst_size = CONFIG_DEFAULT_BURST_SIZE;
}

static void _parse_root(xmlDocPtr doc, xmlNodePtr node, _Ptr<ice_config_t> configuration)
{
  _Nt_array_ptr<char> tmp = NULL;

    configuration->listen_sock = calloc<listener_t> (1, sizeof(listener_t));
    configuration->listen_sock->port = 8000;
    configuration->listen_sock_count = 1;

    do {
        if (node == NULL) break;
        if (xmlIsBlankNode(node)) continue;

        if (xmlStrcmp (node->name, XMLSTR("location")) == 0) {
            if (configuration->location) xmlSafeFree(configuration->location);
            configuration->location = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
        } else if (xmlStrcmp (node->name, XMLSTR("admin")) == 0) {
            if (configuration->admin) xmlSafeFree(configuration->admin);
            configuration->admin = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
        } else if (xmlStrcmp (node->name, XMLSTR("server-id")) == 0) {
            xmlSafeFree (configuration->server_id);
            configuration->server_id = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
            ICECAST_LOG_WARN("Warning, server version string override detected. This may lead to unexpected client software behavior.");
        } else if(xmlStrcmp (node->name, XMLSTR("authentication")) == 0) {
            _parse_authentication(doc, node->xmlChildrenNode, configuration);
        } else if (xmlStrcmp (node->name, XMLSTR("source-password")) == 0) {
            /* TODO: This is the backwards-compatibility location */
            ICECAST_LOG_WARN("<source-password> defined outside <authentication>. This is deprecated.");
            if (configuration->source_password) xmlSafeFree(configuration->source_password);
            configuration->source_password = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
        } else if (xmlStrcmp (node->name, XMLSTR("icelogin")) == 0) {
            tmp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
            configuration->ice_login = atoi(tmp);
            if (tmp) xmlSafeFree(tmp);
        } else if (xmlStrcmp (node->name, XMLSTR("fileserve")) == 0) {
            tmp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
            configuration->fileserve = atoi(tmp);
            if (tmp) xmlSafeFree(tmp);
        } else if (xmlStrcmp (node->name, XMLSTR("relays-on-demand")) == 0) {
            tmp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
            configuration->on_demand = atoi(tmp);
            if (tmp) xmlSafeFree(tmp);
        } else if (xmlStrcmp (node->name, XMLSTR("hostname")) == 0) {
            if (configuration->hostname) xmlSafeFree(configuration->hostname);
            configuration->hostname = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
        } else if (xmlStrcmp (node->name, XMLSTR("mime-types")) == 0) {
            if (configuration->mimetypes_fn) xmlSafeFree(configuration->mimetypes_fn);
            configuration->mimetypes_fn = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
        } else if (xmlStrcmp (node->name, XMLSTR("listen-socket")) == 0) {
            _parse_listen_socket(doc, node->xmlChildrenNode, configuration);
        } else if (xmlStrcmp (node->name, XMLSTR("port")) == 0) {
            tmp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
            if (tmp) {
                configuration->port = atoi(tmp);
                configuration->listen_sock->port = atoi(tmp);
                xmlSafeFree(tmp);
            } else {
                ICECAST_LOG_WARN("<port> must not be empty.");
            }
        } else if (xmlStrcmp (node->name, XMLSTR("bind-address")) == 0) {
            if (configuration->listen_sock->bind_address) 
                xmlSafeFree(configuration->listen_sock->bind_address);
            configuration->listen_sock->bind_address = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
        } else if (xmlStrcmp (node->name, XMLSTR("master-server")) == 0) {
            if (configuration->master_server) xmlSafeFree(configuration->master_server);
            configuration->master_server = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
        } else if (xmlStrcmp (node->name, XMLSTR("master-username")) == 0) {
            if (configuration->master_username) xmlSafeFree(configuration->master_username);
            configuration->master_username = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
        } else if (xmlStrcmp (node->name, XMLSTR("master-password")) == 0) {
            if (configuration->master_password) xmlSafeFree(configuration->master_password);
            configuration->master_password = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
        } else if (xmlStrcmp (node->name, XMLSTR("master-server-port")) == 0) {
            tmp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
            configuration->master_server_port = atoi(tmp);
            xmlSafeFree (tmp);
        } else if (xmlStrcmp (node->name, XMLSTR("master-update-interval")) == 0) {
            tmp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
            configuration->master_update_interval = atoi(tmp);
            xmlSafeFree (tmp);
        } else if (xmlStrcmp (node->name, XMLSTR("shoutcast-mount")) == 0) {
            if (configuration->shoutcast_mount) xmlSafeFree(configuration->shoutcast_mount);
            configuration->shoutcast_mount = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
        } else if (xmlStrcmp (node->name, XMLSTR("limits")) == 0) {
            _parse_limits(doc, node->xmlChildrenNode, configuration);
        } else if (xmlStrcmp (node->name, XMLSTR("http-headers")) == 0) _Checked {
            _parse_http_headers(doc, node->xmlChildrenNode, &(configuration->http_headers));
        } else if (xmlStrcmp (node->name, XMLSTR("relay")) == 0) {
            _parse_relay(doc, node->xmlChildrenNode, configuration);
        } else if (xmlStrcmp (node->name, XMLSTR("mount")) == 0) {
            _parse_mount(doc, node, configuration);
        } else if (xmlStrcmp (node->name, XMLSTR("directory")) == 0) {
            _parse_directory(doc, node->xmlChildrenNode, configuration);
        } else if (xmlStrcmp (node->name, XMLSTR("paths")) == 0) {
            _parse_paths(doc, node->xmlChildrenNode, configuration);
        } else if (xmlStrcmp (node->name, XMLSTR("logging")) == 0) {
            _parse_logging(doc, node->xmlChildrenNode, configuration);
        } else if (xmlStrcmp (node->name, XMLSTR("security")) == 0) {
            _parse_security(doc, node->xmlChildrenNode, configuration);
        }
    } while ((node = node->next));

    /* drop the first listening socket details if more than one is defined, as we only
     * have port or listen-socket not both */
    if (configuration->listen_sock_count > 1)
    {
        configuration->listen_sock = config_clear_listener (configuration->listen_sock);
        configuration->listen_sock_count--;
    }
    if (configuration->port == 0)
        configuration->port = 8000;

   /* issue some warnings on bad configurations */
   if (!configuration->fileserve)
       ICECAST_LOG_WARN("Warning, serving of static files has been disabled in the config, this will also affect files used by the web interface (stylesheets, images).");

  if (!configuration->hostname || strcmp(configuration->hostname, CONFIG_DEFAULT_HOSTNAME) == 0) {
      ICECAST_LOG_WARN("Warning, <hostname> not configured, using default value \"%s\". This will cause problems, e.g. with YP directory listings.", CONFIG_DEFAULT_HOSTNAME);
      if (!configuration->hostname)
          configuration->hostname = (_Nt_array_ptr<char>)xmlCharStrdup (CONFIG_DEFAULT_HOSTNAME);
  }

  if (!configuration->location || strcmp(configuration->location, CONFIG_DEFAULT_LOCATION) == 0) {
      ICECAST_LOG_WARN("Warning, <location> not configured, using default value \"%s\".", CONFIG_DEFAULT_LOCATION);
      if (!configuration->location)
          configuration->location = (_Nt_array_ptr<char>)xmlCharStrdup (CONFIG_DEFAULT_LOCATION);
  }

  if (!configuration->admin || strcmp(configuration->admin, CONFIG_DEFAULT_ADMIN) == 0) {
      ICECAST_LOG_WARN("Warning, <admin> contact not configured, using default value \"%s\".", CONFIG_DEFAULT_ADMIN);
      if (!configuration->admin)
          configuration->admin = (_Nt_array_ptr<char>)xmlCharStrdup (CONFIG_DEFAULT_ADMIN);
  }
}

static void _parse_limits(xmlDocPtr doc, xmlNodePtr node, _Ptr<ice_config_t> configuration)
{
  _Nt_array_ptr<char> tmp = NULL;

    do {
        if (node == NULL) break;
        if (xmlIsBlankNode(node)) continue;

        if (xmlStrcmp (node->name, XMLSTR("clients")) == 0) {
            tmp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
            configuration->client_limit = atoi(tmp);
            if (tmp) xmlSafeFree(tmp);
        } else if (xmlStrcmp (node->name, XMLSTR("sources")) == 0) {
            tmp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
            configuration->source_limit = atoi(tmp);
            if (tmp) xmlSafeFree(tmp);
        } else if (xmlStrcmp (node->name, XMLSTR("queue-size")) == 0) {
            tmp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
            configuration->queue_size_limit = atoi(tmp);
            if (tmp) xmlSafeFree(tmp);
        } else if (xmlStrcmp (node->name, XMLSTR("threadpool")) == 0) {
            tmp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
            configuration->threadpool_size = atoi(tmp);
            if (tmp) xmlSafeFree(tmp);
        } else if (xmlStrcmp (node->name, XMLSTR("client-timeout")) == 0) {
            tmp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
            configuration->client_timeout = atoi(tmp);
            if (tmp) xmlSafeFree(tmp);
        } else if (xmlStrcmp (node->name, XMLSTR("header-timeout")) == 0) {
            tmp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
            configuration->header_timeout = atoi(tmp);
            if (tmp) xmlSafeFree(tmp);
        } else if (xmlStrcmp (node->name, XMLSTR("source-timeout")) == 0) {
            tmp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
            configuration->source_timeout = atoi(tmp);
            if (tmp) xmlSafeFree(tmp);
        } else if (xmlStrcmp (node->name, XMLSTR("burst-on-connect")) == 0) {
            tmp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
            if (atoi(tmp) == 0)
                configuration->burst_size = 0;
            if (tmp) xmlSafeFree(tmp);
        } else if (xmlStrcmp (node->name, XMLSTR("burst-size")) == 0) {
            tmp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
            configuration->burst_size = atoi(tmp);
            if (tmp) xmlSafeFree(tmp);
        }
    } while ((node = node->next));
}

static void _parse_mount(xmlDocPtr doc, xmlNodePtr node, _Ptr<ice_config_t> configuration)
{
    _Nt_array_ptr<char> tmp = NULL;
    _Ptr<mount_proxy> mount = calloc<mount_proxy>(1, sizeof(mount_proxy));
    _Ptr<mount_proxy> current = configuration->mounts;
    _Ptr<mount_proxy> last =NULL;
    
    /* default <mount> settings */
    mount->mounttype = MOUNT_TYPE_NORMAL;
    mount->max_listeners = -1;
    mount->burst_size = -1;
    mount->mp3_meta_interval = -1;
    mount->yp_public = -1;
    mount->next = NULL;

    tmp = (_Nt_array_ptr<char>) xmlGetProp(node, XMLSTR("type"));
    if (tmp) {
        if (strcmp(tmp, "normal") == 0) {
	    mount->mounttype = MOUNT_TYPE_NORMAL;
	}
	else if (strcmp(tmp, "default") == 0) {
	    mount->mounttype = MOUNT_TYPE_DEFAULT;
	}
	else {
	    ICECAST_LOG_WARN("Unknown mountpoint type: %s", tmp);
            config_clear_mount (mount);
            return;
	}
	xmlSafeFree(tmp);
    }

    node = node->xmlChildrenNode;

    do {
        if (node == NULL) break;
        if (xmlIsBlankNode(node)) continue;

        if (xmlStrcmp (node->name, XMLSTR("mount-name")) == 0) {
            mount->mountname = (_Nt_array_ptr<char>)xmlNodeListGetString (doc, node->xmlChildrenNode, 1);
        }
        else if (xmlStrcmp (node->name, XMLSTR("username")) == 0) {
            mount->username = (_Nt_array_ptr<char>)xmlNodeListGetString(
                    doc, node->xmlChildrenNode, 1);
        }
        else if (xmlStrcmp (node->name, XMLSTR("password")) == 0) {
            mount->password = (_Nt_array_ptr<char>)xmlNodeListGetString(
                    doc, node->xmlChildrenNode, 1);
        }
        else if (xmlStrcmp (node->name, XMLSTR("dump-file")) == 0) {
            mount->dumpfile = (_Nt_array_ptr<char>)xmlNodeListGetString(
                    doc, node->xmlChildrenNode, 1);
        }
        else if (xmlStrcmp (node->name, XMLSTR("intro")) == 0) {
            mount->intro_filename = (_Nt_array_ptr<char>)xmlNodeListGetString(
                    doc, node->xmlChildrenNode, 1);
        }
        else if (xmlStrcmp (node->name, XMLSTR("fallback-mount")) == 0) {
            mount->fallback_mount = (_Nt_array_ptr<char>)xmlNodeListGetString(
                    doc, node->xmlChildrenNode, 1);
        }
        else if (xmlStrcmp (node->name, XMLSTR("fallback-when-full")) == 0) {
            tmp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
            mount->fallback_when_full = atoi(tmp);
            if(tmp) xmlSafeFree(tmp);
        }
        else if (xmlStrcmp (node->name, XMLSTR("max-listeners")) == 0) {
            tmp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
            mount->max_listeners = atoi(tmp);
            if(tmp) xmlSafeFree(tmp);
        }
        else if (xmlStrcmp (node->name, XMLSTR("charset")) == 0) {
            mount->charset = (_Nt_array_ptr<char>)xmlNodeListGetString(doc,
                    node->xmlChildrenNode, 1);
        }
        else if (xmlStrcmp (node->name, XMLSTR("mp3-metadata-interval")) == 0) {
            tmp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
            mount->mp3_meta_interval = atoi(tmp);
            if(tmp) xmlSafeFree(tmp);
        }
        else if (xmlStrcmp (node->name, XMLSTR("fallback-override")) == 0) {
            tmp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
            mount->fallback_override = atoi(tmp);
            if(tmp) xmlSafeFree(tmp);
        }
        else if (xmlStrcmp (node->name, XMLSTR("no-mount")) == 0) {
            tmp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
            mount->no_mount = atoi(tmp);
            if(tmp) xmlSafeFree(tmp);
        }
        else if (xmlStrcmp (node->name, XMLSTR("no-yp")) == 0) {
            tmp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
            mount->yp_public = atoi(tmp) == 0 ? -1 : 0;
            if(tmp) xmlSafeFree(tmp);
        }
        else if (xmlStrcmp (node->name, XMLSTR("hidden")) == 0) {
            tmp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
            mount->hidden = atoi(tmp);
            if(tmp) xmlSafeFree(tmp);
        }
        else if (xmlStrcmp (node->name, XMLSTR("authentication")) == 0) {
            mount->auth = auth_get_authenticator (node);
        }
        else if (xmlStrcmp (node->name, XMLSTR("on-connect")) == 0) {
            mount->on_connect = (_Nt_array_ptr<char>)xmlNodeListGetString(
                    doc, node->xmlChildrenNode, 1);
        }
        else if (xmlStrcmp (node->name, XMLSTR("on-disconnect")) == 0) {
            mount->on_disconnect = (_Nt_array_ptr<char>)xmlNodeListGetString(
                    doc, node->xmlChildrenNode, 1);
        }
        else if (xmlStrcmp (node->name, XMLSTR("max-listener-duration")) == 0) {
            tmp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
            mount->max_listener_duration = atoi(tmp);
            if(tmp) xmlSafeFree(tmp);
        }
        else if (xmlStrcmp (node->name, XMLSTR("queue-size")) == 0) {
            tmp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
            mount->queue_size_limit = atoi (tmp);
            if(tmp) xmlSafeFree(tmp);
        }
        else if (xmlStrcmp (node->name, XMLSTR("source-timeout")) == 0) {
            tmp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
            if (tmp)
            {
                mount->source_timeout = atoi (tmp);
                xmlSafeFree(tmp);
            }
        } else if (xmlStrcmp (node->name, XMLSTR("burst-size")) == 0) {
            tmp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
            mount->burst_size = atoi(tmp);
            if (tmp) xmlSafeFree(tmp);
        } else if (xmlStrcmp (node->name, XMLSTR("cluster-password")) == 0) {
            mount->cluster_password = (_Nt_array_ptr<char>)xmlNodeListGetString(
                    doc, node->xmlChildrenNode, 1);
        } else if (xmlStrcmp (node->name, XMLSTR("stream-name")) == 0) {
            mount->stream_name = (_Nt_array_ptr<char>)xmlNodeListGetString(
                    doc, node->xmlChildrenNode, 1);
        } else if (xmlStrcmp (node->name, XMLSTR("stream-description")) == 0) {
            mount->stream_description = (_Nt_array_ptr<char>)xmlNodeListGetString(
                    doc, node->xmlChildrenNode, 1);
        } else if (xmlStrcmp (node->name, XMLSTR("stream-url")) == 0) {
            mount->stream_url = (_Nt_array_ptr<char>)xmlNodeListGetString(
                    doc, node->xmlChildrenNode, 1);
        } else if (xmlStrcmp (node->name, XMLSTR("genre")) == 0) {
            mount->stream_genre = (_Nt_array_ptr<char>)xmlNodeListGetString(
                    doc, node->xmlChildrenNode, 1);
        } else if (xmlStrcmp (node->name, XMLSTR("bitrate")) == 0) {
            mount->bitrate = (_Nt_array_ptr<char>)xmlNodeListGetString(
                    doc, node->xmlChildrenNode, 1);
        } else if (xmlStrcmp (node->name, XMLSTR("public")) == 0) {
            tmp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
            mount->yp_public = atoi (tmp);
            if(tmp) xmlSafeFree(tmp);
        } else if (xmlStrcmp (node->name, XMLSTR("type")) == 0) {
            mount->type = (_Nt_array_ptr<char>)xmlNodeListGetString(
                    doc, node->xmlChildrenNode, 1);
        } else if (xmlStrcmp (node->name, XMLSTR("subtype")) == 0) {
            mount->subtype = (_Nt_array_ptr<char>)xmlNodeListGetString(
                    doc, node->xmlChildrenNode, 1);
        } else if (xmlStrcmp (node->name, XMLSTR("http-headers")) == 0) _Checked {
            _parse_http_headers(doc, node->xmlChildrenNode, &(mount->http_headers));
        }
    } while ((node = node->next));

    /* make sure we have at least the mountpoint name */
    if (mount->mountname == NULL && mount->mounttype != MOUNT_TYPE_DEFAULT)
    {
        config_clear_mount (mount);
        return;
    }
    else if (mount->mountname != NULL && mount->mounttype == MOUNT_TYPE_DEFAULT)
    {
    	ICECAST_LOG_WARN("Default mount %s has mount-name set. This is not supported. Behavior may not be consistent.", mount->mountname);
    }
    if (mount->auth && mount->mountname) {
        mount->auth->mount = strdup ((_Nt_array_ptr<char>)mount->mountname);
    } else if (mount->auth && mount->mounttype == MOUNT_TYPE_DEFAULT ) {
        mount->auth->mount = strdup ("(default mount)");
    }
    while(current) {
        last = current;
        current = current->next;
    }

    if (!mount->fallback_mount && (mount->fallback_when_full || mount->fallback_override))
    {
        ICECAST_LOG_WARN("Config for mount %s contains fallback options but no fallback mount.", mount->mountname);
    }

    if(last)
        last->next = mount;
    else
        configuration->mounts = mount;
}

static void _parse_http_headers(xmlDocPtr doc, xmlNodePtr node, _Ptr<_Ptr<ice_config_http_header_t>> http_headers) {
    _Ptr<ice_config_http_header_t> header = ((void *)0);
    _Ptr<ice_config_http_header_t> next = ((void *)0);
    _Nt_array_ptr<char> name = NULL;
    _Nt_array_ptr<char> value = NULL;
    _Nt_array_ptr<char> tmp = NULL;
    int status;
    http_header_type type;

    do {
        if (node == NULL) break;
        if (xmlIsBlankNode(node)) continue;
        if (xmlStrcmp (node->name, XMLSTR("header")) != 0) break;
        if (!(name = (_Nt_array_ptr<char>)xmlGetProp(node, XMLSTR("name")))) break;
        if (!(value = (_Nt_array_ptr<char>)xmlGetProp(node, XMLSTR("value")))) break;

        type = HTTP_HEADER_TYPE_STATIC; /* default */
        if ((tmp = (_Nt_array_ptr<char>)xmlGetProp(node, XMLSTR("type")))) {
            if (strcmp(tmp, "static") == 0) {
                type = HTTP_HEADER_TYPE_STATIC;
            } else {
                ICECAST_LOG_WARN("Unknown type %s for HTTP Header %s", tmp, name);
                xmlSafeFree(tmp);
                break;
            }
            xmlSafeFree(tmp);
        }

        status = 0; /* default: any */
        if ((tmp = (_Nt_array_ptr<char>)xmlGetProp(node, XMLSTR("status")))) {
            status = atoi(tmp);
            xmlSafeFree(tmp);
        }

        header = calloc<ice_config_http_header_t>(1, sizeof(ice_config_http_header_t));
        if (!header) break;
        header->type = type;
        header->name = name;
        header->value = value;
        header->status = status;
        name = NULL;
        value = NULL;

        if (!*http_headers) {
            *http_headers = header;
            continue;
        }
        next = *http_headers;
        while (next->next) next = next->next;
        next->next = header;
    } while ((node = node->next));
    /* in case we used break we may need to clean those up */
    if (name)
	xmlSafeFree(name);
    if (value)
	xmlSafeFree(value);
}

static void _parse_relay(xmlDocPtr doc, xmlNodePtr node, _Ptr<ice_config_t> configuration)
{
    _Nt_array_ptr<char> tmp = NULL;
    _Ptr<relay_server> relay = calloc<relay_server>(1, sizeof(relay_server));
    _Ptr<relay_server> current = configuration->relay;
    _Ptr<relay_server> last =NULL;

    while(current) {
        last = current;
        current = current->next;
    }

    if(last)
        last->next = relay;
    else
        configuration->relay = relay;

    relay->next = NULL;
    relay->mp3metadata = 1;
    relay->on_demand = configuration->on_demand;
    relay->server = (_Nt_array_ptr<char>)xmlCharStrdup ("127.0.0.1");
    relay->mount = (_Nt_array_ptr<char>)xmlCharStrdup ("/");

    do {
        if (node == NULL) break;
        if (xmlIsBlankNode(node)) continue;

        if (xmlStrcmp (node->name, XMLSTR("server")) == 0) {
            if (relay->server) xmlSafeFree (relay->server);
            relay->server = (_Nt_array_ptr<char>)xmlNodeListGetString(
                    doc, node->xmlChildrenNode, 1);
        }
        else if (xmlStrcmp (node->name, XMLSTR("port")) == 0) {
            tmp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
            if (tmp) {
                relay->port = atoi(tmp);
                xmlSafeFree(tmp);
            } else {
                ICECAST_LOG_WARN("<port> must not be empty.");
            }
        }
        else if (xmlStrcmp (node->name, XMLSTR("mount")) == 0) {
            if (relay->mount) xmlSafeFree (relay->mount);
            relay->mount = (_Nt_array_ptr<char>)xmlNodeListGetString(
                    doc, node->xmlChildrenNode, 1);
        }
        else if (xmlStrcmp (node->name, XMLSTR("local-mount")) == 0) {
            if (relay->localmount) xmlSafeFree (relay->localmount);
            relay->localmount = (_Nt_array_ptr<char>)xmlNodeListGetString(
                    doc, node->xmlChildrenNode, 1);
        }
        else if (xmlStrcmp (node->name, XMLSTR("relay-shoutcast-metadata")) == 0) {
            tmp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
            relay->mp3metadata = atoi(tmp);
            if(tmp) xmlSafeFree(tmp);
        }
        else if (xmlStrcmp (node->name, XMLSTR("username")) == 0) {
            if (relay->username) xmlSafeFree (relay->username);
            relay->username = (_Nt_array_ptr<char>)xmlNodeListGetString(doc,
                    node->xmlChildrenNode, 1);
        }
        else if (xmlStrcmp (node->name, XMLSTR("password")) == 0) {
            if (relay->password) xmlSafeFree (relay->password);
            relay->password = (_Nt_array_ptr<char>)xmlNodeListGetString(doc,
                    node->xmlChildrenNode, 1);
        }
        else if (xmlStrcmp (node->name, XMLSTR("on-demand")) == 0) {
            tmp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
            relay->on_demand = atoi(tmp);
            if (tmp) xmlSafeFree(tmp);
        }
        else if (xmlStrcmp (node->name, XMLSTR("bind")) == 0) {
            if (relay->bind) xmlSafeFree (relay->bind);
            relay->bind = (_Nt_array_ptr<char>)xmlNodeListGetString (doc, node->xmlChildrenNode, 1);
        }
    } while ((node = node->next));
    if (relay->localmount == NULL)
        relay->localmount = (_Nt_array_ptr<char>)xmlStrdup ((_Nt_array_ptr<xmlChar>) relay->mount);
}

static void _parse_listen_socket(xmlDocPtr doc, xmlNodePtr node, _Ptr<ice_config_t> configuration)
{
    _Nt_array_ptr<char> tmp = NULL;
    _Ptr<listener_t> listener = calloc<listener_t> (1, sizeof(listener_t));

    if (listener == NULL)
        return;
    listener->port = 8000;

    do {
        if (node == NULL) break;
        if (xmlIsBlankNode(node)) continue;

        if (xmlStrcmp (node->name, XMLSTR("port")) == 0) {
            tmp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
            if (tmp) {
                if(configuration->port == 0)
                    configuration->port = atoi(tmp);
                listener->port = atoi(tmp);
                xmlSafeFree(tmp);
            } else {
                ICECAST_LOG_WARN("<port> must not be empty.");
            }
        }
        else if (xmlStrcmp (node->name, XMLSTR("ssl")) == 0) {
            tmp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
            listener->ssl = atoi(tmp);
            if(tmp) xmlSafeFree(tmp);
        }
        else if (xmlStrcmp (node->name, XMLSTR("shoutcast-compat")) == 0) {
            tmp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
            listener->shoutcast_compat = atoi(tmp);
            if(tmp) xmlSafeFree(tmp);
        }
        else if (xmlStrcmp (node->name, XMLSTR("shoutcast-mount")) == 0) {
            if (listener->shoutcast_mount) xmlSafeFree (listener->shoutcast_mount);
            listener->shoutcast_mount = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, 
                    node->xmlChildrenNode, 1);
        }
        else if (xmlStrcmp (node->name, XMLSTR("bind-address")) == 0) {
            if (listener->bind_address) xmlSafeFree (listener->bind_address);
            listener->bind_address = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, 
                    node->xmlChildrenNode, 1);
        }
        else if (xmlStrcmp (node->name, XMLSTR("so-sndbuf")) == 0) {
            tmp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
            listener->so_sndbuf = atoi(tmp);
            if(tmp) xmlSafeFree(tmp);
        }
    } while ((node = node->next));

    /* we know there's at least one of these, so add this new one after the first
     * that way it can be removed easily later on */
    listener->next = configuration->listen_sock->next;
    configuration->listen_sock->next = listener;
    configuration->listen_sock_count++;
    if (listener->shoutcast_mount)
    {
        _Ptr<listener_t> sc_port = calloc<listener_t> (1, sizeof (listener_t));
        sc_port->port = listener->port+1;
        sc_port->shoutcast_compat = 1;
        sc_port->shoutcast_mount = (_Nt_array_ptr<char>)xmlStrdup ((_Nt_array_ptr<xmlChar>) (listener->shoutcast_mount));
        if (listener->bind_address)
            sc_port->bind_address = (_Nt_array_ptr<char>)xmlStrdup ((_Nt_array_ptr<xmlChar>) (listener->bind_address));

        sc_port->next = listener->next;
        listener->next = sc_port;
        configuration->listen_sock_count++;
    }
}

static void _parse_authentication(xmlDocPtr doc, xmlNodePtr node, _Ptr<ice_config_t> configuration)
{
    do {
        if (node == NULL) break;
        if (xmlIsBlankNode(node)) continue;

        if (xmlStrcmp (node->name, XMLSTR("source-password")) == 0) {
            if (xmlGetProp(node, XMLSTR("mount"))) {
                ICECAST_LOG_ERROR("Mount level source password defined within global <authentication> section.");
            }
            else {
                if (configuration->source_password)
                    xmlSafeFree(configuration->source_password);
                configuration->source_password = 
                    (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
            }
        } else if (xmlStrcmp (node->name, XMLSTR("admin-password")) == 0) {
            if(configuration->admin_password)
                xmlSafeFree(configuration->admin_password);
            configuration->admin_password =
                (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
        } else if (xmlStrcmp (node->name, XMLSTR("admin-user")) == 0) {
            if(configuration->admin_username)
                xmlSafeFree(configuration->admin_username);
            configuration->admin_username =
                (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
        } else if (xmlStrcmp (node->name, XMLSTR("relay-password")) == 0) {
            if(configuration->relay_password)
                xmlSafeFree(configuration->relay_password);
            configuration->relay_password =
                (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
        } else if (xmlStrcmp (node->name, XMLSTR("relay-user")) == 0) {
            if(configuration->relay_username)
                xmlSafeFree(configuration->relay_username);
            configuration->relay_username =
                (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
        }
    } while ((node = node->next));
}

static void _parse_directory(xmlDocPtr doc, xmlNodePtr node, _Ptr<ice_config_t> configuration)
{
  _Nt_array_ptr<char> tmp = NULL;

    if (configuration->num_yp_directories >= MAX_YP_DIRECTORIES) {
        ICECAST_LOG_ERROR("Maximum number of yp directories exceeded!");
        return;
    }
    do {
        if (node == NULL) break;
        if (xmlIsBlankNode(node)) continue;

        if (xmlStrcmp (node->name, XMLSTR("yp-url")) == 0) {
            if (configuration->yp_url[configuration->num_yp_directories]) 
                xmlSafeFree(configuration->yp_url[configuration->num_yp_directories]);
            configuration->yp_url[configuration->num_yp_directories] = 
                (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
        } else if (xmlStrcmp (node->name, XMLSTR("yp-url-timeout")) == 0) {
            tmp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
            configuration->yp_url_timeout[configuration->num_yp_directories] = 
                atoi(tmp);
            if (tmp) xmlSafeFree(tmp);
        } else if (xmlStrcmp (node->name, XMLSTR("server")) == 0) {
            _add_server(doc, node->xmlChildrenNode, configuration);
        } else if (xmlStrcmp (node->name, XMLSTR("touch-interval")) == 0) {
            tmp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
            configuration->yp_touch_interval[configuration->num_yp_directories] =
                atoi(tmp);
            if (tmp) xmlSafeFree(tmp);
        }
    } while ((node = node->next));
    if (configuration->yp_url [configuration->num_yp_directories] == NULL)
        return;
    configuration->num_yp_directories++;
}

static void _parse_paths(xmlDocPtr doc, xmlNodePtr node, _Ptr<ice_config_t> configuration)
{
    _Nt_array_ptr<char> temp = NULL;
    _Ptr<aliases> alias = ((void *)0);
_Ptr<aliases> current = ((void *)0);
_Ptr<aliases> last = ((void *)0);


    do {
        if (node == NULL) break;
        if (xmlIsBlankNode(node)) continue;

        if (xmlStrcmp (node->name, XMLSTR("basedir")) == 0) {
            if (configuration->base_dir) xmlSafeFree(configuration->base_dir);
            configuration->base_dir = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
        } else if (xmlStrcmp (node->name, XMLSTR("logdir")) == 0) {
            if (!(temp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1))) {
                ICECAST_LOG_WARN("<logdir> must not be empty.");
                continue;
            }
            if (configuration->log_dir) xmlSafeFree(configuration->log_dir);
            configuration->log_dir = temp;
        } else if (xmlStrcmp (node->name, XMLSTR("pidfile")) == 0) {
            if (configuration->pidfile) xmlSafeFree(configuration->pidfile);
            configuration->pidfile = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
        } else if (xmlStrcmp (node->name, XMLSTR("deny-ip")) == 0) {
            if (configuration->banfile) xmlSafeFree(configuration->banfile);
            configuration->banfile = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
        } else if (xmlStrcmp (node->name, XMLSTR("allow-ip")) == 0) {
            if (configuration->allowfile) xmlSafeFree(configuration->allowfile);
            configuration->allowfile = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
        } else if (xmlStrcmp (node->name, XMLSTR("ssl-certificate")) == 0) {
            if (configuration->cert_file) xmlSafeFree(configuration->cert_file);
            configuration->cert_file = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
        } else if (xmlStrcmp (node->name, XMLSTR("ssl-allowed-ciphers")) == 0) {
            if (configuration->cipher_list) xmlSafeFree(configuration->cipher_list);
            configuration->cipher_list = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
        } else if (xmlStrcmp (node->name, XMLSTR("webroot")) == 0) {
            if (!(temp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1))) {
                ICECAST_LOG_WARN("<webroot> must not be empty.");
                continue;
            }
            if (configuration->webroot_dir) xmlSafeFree(configuration->webroot_dir);
            configuration->webroot_dir = temp;
            _Nt_array_ptr<char> tmp = configuration->webroot_dir;
            size_t len = strlen(tmp) _Where tmp : bounds(tmp, 
                                                         tmp + len);
            if(tmp[len - 1] == '/') { 
                tmp[len - 1] = 0;
            }
        } else if (xmlStrcmp (node->name, XMLSTR("adminroot")) == 0) {
            if (!(temp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1))) {
                ICECAST_LOG_WARN("<adminroot> must not be empty.");
                continue;
            }
            if (configuration->adminroot_dir) 
                xmlSafeFree(configuration->adminroot_dir);
            configuration->adminroot_dir = (_Nt_array_ptr<char>)temp;
            _Nt_array_ptr<char> tmp = configuration->adminroot_dir;
            size_t len = strlen(tmp) _Where tmp : bounds(tmp, 
                                                         tmp + len);
            if(tmp[len - 1] == '/')
                tmp[len - 1] = 0;
        } else if (xmlStrcmp (node->name, XMLSTR("alias")) == 0) {
            alias = malloc<aliases>(sizeof(aliases));
            alias->next = NULL;
            alias->source = (_Nt_array_ptr<char>)xmlGetProp(node, XMLSTR("source"));
            if(alias->source == NULL) {
                free<aliases>(alias);
                continue;
            }
            alias->destination = (_Nt_array_ptr<char>)xmlGetProp(node, XMLSTR("destination"));
            if (!alias->destination)
                alias->destination = (_Nt_array_ptr<char>)xmlGetProp(node, XMLSTR("dest"));
            if(alias->destination == NULL) {
                xmlSafeFree(alias->source);
                free<aliases>(alias);
                continue;
            }
            temp = NULL;
            temp = (_Nt_array_ptr<char>)xmlGetProp(node, XMLSTR("port"));
            if(temp != NULL) {
                alias->port = atoi(temp);
                xmlSafeFree(temp);
            }
            else
                alias->port = -1;
            alias->bind_address = (_Nt_array_ptr<char>)xmlGetProp(node, XMLSTR("bind-address"));
            current = configuration->aliases;
            last = NULL;
            while(current) {
                last = current;
                current = current->next;
            }
            if(last)
                last->next = alias;
            else
                configuration->aliases = alias;
        }
    } while ((node = node->next));
}

static void _parse_logging(xmlDocPtr doc, xmlNodePtr node, _Ptr<ice_config_t> configuration)
{
    _Nt_array_ptr<char> tmp = NULL;
    do {
        if (node == NULL) break;
        if (xmlIsBlankNode(node)) continue;

        if (xmlStrcmp (node->name, XMLSTR("accesslog")) == 0) {
            if (!(tmp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1))) {
                ICECAST_LOG_WARN("<accesslog> must not be empty.");
                continue;
            }
            if (configuration->access_log) xmlSafeFree(configuration->access_log);
            configuration->access_log = tmp;
        } else if (xmlStrcmp (node->name, XMLSTR("errorlog")) == 0) {
            if (!(tmp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1))) {
                ICECAST_LOG_WARN("<errorlog> must not be empty.");
                continue;
            }
            if (configuration->error_log) xmlSafeFree(configuration->error_log);
            configuration->error_log = tmp;
        } else if (xmlStrcmp (node->name, XMLSTR("playlistlog")) == 0) {
            if (configuration->playlist_log) xmlSafeFree(configuration->playlist_log);
            configuration->playlist_log = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
        } else if (xmlStrcmp (node->name, XMLSTR("logsize")) == 0) {
            _Nt_array_ptr<char>tmp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
            configuration->logsize = atoi(tmp);
            if (tmp) xmlSafeFree(tmp);
        } else if (xmlStrcmp (node->name, XMLSTR("loglevel")) == 0) {
           _Nt_array_ptr<char>tmp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
           configuration->loglevel = atoi(tmp);
           if (tmp) xmlSafeFree(tmp);
        } else if (xmlStrcmp (node->name, XMLSTR("logarchive")) == 0) {
            _Nt_array_ptr<char>tmp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
            configuration->logarchive = atoi(tmp);
            if (tmp) xmlSafeFree(tmp);
        }
    } while ((node = node->next));
}

static void _parse_security(xmlDocPtr doc, xmlNodePtr node, _Ptr<ice_config_t> configuration)
{
   _Nt_array_ptr<char> tmp = NULL;
   xmlNodePtr oldnode = NULL;

   do {
       if (node == NULL) break;
       if (xmlIsBlankNode(node)) continue;

       if (xmlStrcmp (node->name, XMLSTR("chroot")) == 0) {
           tmp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
           configuration->chroot = atoi(tmp);
           if (tmp) xmlSafeFree(tmp);
       } else if (xmlStrcmp (node->name, XMLSTR("changeowner")) == 0) {
           configuration->chuid = 1;
           oldnode = node;
           node = node->xmlChildrenNode;
           do {
               if(node == NULL) break;
               if(xmlIsBlankNode(node)) continue;
               if(xmlStrcmp (node->name, XMLSTR("user")) == 0) {
                   if(configuration->user) xmlSafeFree(configuration->user);
                   configuration->user = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
               } else if(xmlStrcmp (node->name, XMLSTR("group")) == 0) {
                   if(configuration->group) xmlSafeFree(configuration->group);
                   configuration->group = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
               }
           } while((node = node->next));
           node = oldnode;
       }
   } while ((node = node->next));
}

static void _add_server(xmlDocPtr doc, xmlNodePtr node, _Ptr<ice_config_t> configuration)
{
    _Ptr<ice_config_dir_t> dirnode = ((void *)0);
_Ptr<ice_config_dir_t> server = ((void *)0);

    int addnode;
    _Nt_array_ptr<char> tmp = NULL;

    server = (_Ptr<ice_config_dir_t>)malloc<ice_config_dir_t>(sizeof(ice_config_dir_t));
    server->touch_interval = configuration->touch_interval;
    server->host = NULL;
    addnode = 0;
    
    do {
        if (node == NULL) break;
        if (xmlIsBlankNode(node)) continue;

        if (xmlStrcmp (node->name, XMLSTR("host")) == 0) {
            server->host = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, 
                    node->xmlChildrenNode, 1);
            addnode = 1;
        } else if (xmlStrcmp (node->name, XMLSTR("touch-interval")) == 0) {
            tmp = (_Nt_array_ptr<char>)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
            server->touch_interval = atoi(tmp);
            if (tmp) xmlSafeFree(tmp);
        }
        server->next = NULL;
    } while ((node = node->next));

    if (addnode) {
        dirnode = configuration->dir_list;
        if (dirnode == NULL) {
            configuration->dir_list = server;
        } else {
            while (dirnode->next) dirnode = dirnode->next;
            
            dirnode->next = server;
        }
        
        server = NULL;
        addnode = 0;
    }
    else {
        free<ice_config_dir_t> (server);
    }
}

static void merge_mounts(_Ptr<mount_proxy> dst, _Ptr<mount_proxy> src) {
    _Ptr<ice_config_http_header_t> http_header_next = ((void *)0);
    _Ptr<_Ptr<ice_config_http_header_t>> http_header_tail = ((void *)0);

    if (!dst || !src)
    	return;

    if (!dst->username)
    	dst->username = (_Nt_array_ptr<char>)xmlStrdup((_Nt_array_ptr<xmlChar>)src->username);
    if (!dst->password)
    	dst->password = (_Nt_array_ptr<char>)xmlStrdup((_Nt_array_ptr<xmlChar>)src->password);
    if (!dst->dumpfile)
    	dst->dumpfile = (_Nt_array_ptr<char>)xmlStrdup((_Nt_array_ptr<xmlChar>)src->dumpfile);
    if (!dst->intro_filename)
    	dst->intro_filename = (_Nt_array_ptr<char>)xmlStrdup((_Nt_array_ptr<xmlChar>)src->intro_filename);
    if (!dst->fallback_when_full)
    	dst->fallback_when_full = src->fallback_when_full;
    if (dst->max_listeners == -1)
    	dst->max_listeners = src->max_listeners;
    if (!dst->fallback_mount)
    	dst->fallback_mount = (_Nt_array_ptr<char>)xmlStrdup((_Nt_array_ptr<xmlChar>)src->fallback_mount);
    if (!dst->fallback_override)
    	dst->fallback_override = src->fallback_override;
    if (!dst->no_mount)
    	dst->no_mount = src->no_mount;
    if (dst->burst_size == -1)
    	dst->burst_size = src->burst_size;
    if (!dst->queue_size_limit)
    	dst->queue_size_limit = src->queue_size_limit;
    if (!dst->hidden)
    	dst->hidden = src->hidden;
    if (!dst->source_timeout)
    	dst->source_timeout = src->source_timeout;
    if (!dst->charset)
    	dst->charset = (_Nt_array_ptr<char>)xmlStrdup((_Nt_array_ptr<xmlChar>)src->charset);
    if (dst->mp3_meta_interval == -1)
    	dst->mp3_meta_interval = src->mp3_meta_interval;
    if (!dst->auth_type)
    	dst->auth_type = (_Nt_array_ptr<char>)xmlStrdup((_Nt_array_ptr<xmlChar>)src->auth_type);
    // TODO: dst->auth
    if (!dst->cluster_password)
    	dst->cluster_password = (_Nt_array_ptr<char>)xmlStrdup((_Nt_array_ptr<xmlChar>)src->cluster_password);
    // TODO: dst->auth_options
    if (!dst->on_connect)
    	dst->on_connect = (_Nt_array_ptr<char>)xmlStrdup((_Nt_array_ptr<xmlChar>)src->on_connect);
    if (!dst->on_disconnect)
    	dst->on_disconnect = (_Nt_array_ptr<char>)xmlStrdup((_Nt_array_ptr<xmlChar>)src->on_disconnect);
    if (!dst->max_listener_duration)
    	dst->max_listener_duration = src->max_listener_duration;
    if (!dst->stream_name)
    	dst->stream_name = (_Nt_array_ptr<char>)xmlStrdup((_Nt_array_ptr<xmlChar>)src->stream_name);
    if (!dst->stream_description)
    	dst->stream_description = (_Nt_array_ptr<char>)xmlStrdup((_Nt_array_ptr<xmlChar>)src->stream_description);
    if (!dst->stream_url)
    	dst->stream_url = (_Nt_array_ptr<char>)xmlStrdup((_Nt_array_ptr<xmlChar>)src->stream_url);
    if (!dst->stream_genre)
    	dst->stream_genre = (_Nt_array_ptr<char>)xmlStrdup((_Nt_array_ptr<xmlChar>)src->stream_genre);
    if (!dst->bitrate)
    	dst->bitrate = (_Nt_array_ptr<char>)xmlStrdup((_Nt_array_ptr<xmlChar>)src->bitrate);
    if (!dst->type)
    	dst->type = (_Nt_array_ptr<char>)xmlStrdup((_Nt_array_ptr<xmlChar>)src->type);
    if (!dst->subtype)
    	dst->subtype = (_Nt_array_ptr<char>)xmlStrdup((_Nt_array_ptr<xmlChar>)src->subtype);
    if (dst->yp_public == -1)
    	dst->yp_public = src->yp_public;

    if (dst->http_headers) _Checked {
        http_header_next = dst->http_headers;
        while (http_header_next->next) http_header_next = http_header_next->next;
        http_header_tail = &(http_header_next->next);
    } else _Checked {
        http_header_tail = &(dst->http_headers);
    }
    *http_header_tail = config_copy_http_header(src->http_headers);
}

static inline void _merge_mounts_all(_Ptr<ice_config_t> c) {
    _Ptr<mount_proxy> mountinfo = c->mounts;
    _Ptr<mount_proxy> default_mount = ((void *)0);

    for (; mountinfo; mountinfo = mountinfo->next)
    {
    	if (mountinfo->mounttype != MOUNT_TYPE_NORMAL)
	    continue;

        default_mount = config_find_mount(c, mountinfo->mountname, MOUNT_TYPE_DEFAULT);

	merge_mounts(mountinfo, default_mount);
    }
}

int fnmatch(const char *pattern : itype(_Nt_array_ptr<const char>), const char *string : itype(_Nt_array_ptr<const char>), int flags);


/* return the mount details that match the supplied mountpoint */
mount_proxy *config_find_mount(ice_config_t *config : itype(_Ptr<ice_config_t>), const char *mount : itype(_Nt_array_ptr<const char>), mount_type type) : itype(_Ptr<mount_proxy>)
{
    _Ptr<mount_proxy> mountinfo = config->mounts;

    for (; mountinfo; mountinfo = mountinfo->next)
    {
        if (mountinfo->mounttype != type)
	    continue;

	if (mount == NULL || mountinfo->mountname == NULL)
            break;

	if (mountinfo->mounttype == MOUNT_TYPE_NORMAL && strcmp (mountinfo->mountname, mount) == 0)
            break;

#ifndef _WIN32
        if (fnmatch((mountinfo->mountname), mount, FNM_PATHNAME) == 0)
            break;
#else
        if (strcmp(mountinfo->mountname, mount) == 0)
            break;
#endif
    }

    /* retry with default mount */
    if (!mountinfo && type == MOUNT_TYPE_NORMAL)
            mountinfo = config_find_mount(config, mount, MOUNT_TYPE_DEFAULT);

    return mountinfo;
}

/* Helper function to locate the configuration details of the listening 
 * socket
 */
listener_t *config_get_listen_sock(ice_config_t *config : itype(_Ptr<ice_config_t>), connection_t *con : itype(_Ptr<connection_t>)) : itype(_Ptr<listener_t>)
{
    _Ptr<listener_t> listener = ((void *)0);
    int i = 0;

    listener = config->listen_sock;
    while (listener)
    {
        if (i >= global.server_sockets)
            listener = NULL;
        else
        {
            if (global.serversock[i] == con->serversock)
                break;
            listener = listener->next;
            i++;
        }
    }
    return listener;
}

