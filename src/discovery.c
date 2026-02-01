/*
 * discovery.c - mDNS/Avahi service discovery
 * 
 * Announces RootStream service on local network using Avahi/Bonjour.
 * Allows automatic peer discovery without manual IP entry.
 * 
 * Service name: _rootstream._udp
 * TXT records:
 *   - version=1.0.0
 *   - pubkey=<base64_public_key>
 *   - hostname=<device_name>
 * 
 * How it works:
 * 1. Service announces itself via mDNS on port 5353
 * 2. Other devices browse for _rootstream._udp services
 * 3. When found, TXT records provide public key and hostname
 * 4. Automatic pairing if both devices trust each other
 */

#include "../include/rootstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_AVAHI
#include <avahi-client/client.h>
#include <avahi-client/publish.h>
#include <avahi-client/lookup.h>
#include <avahi-common/simple-watch.h>
#include <avahi-common/error.h>
#include <avahi-common/malloc.h>

typedef struct {
    AvahiClient *client;
    AvahiEntryGroup *group;
    AvahiServiceBrowser *browser;
    AvahiSimplePoll *simple_poll;
    rootstream_ctx_t *ctx;
} avahi_ctx_t;

/*
 * Avahi entry group callback
 * Called when service registration state changes
 */
static void entry_group_callback(AvahiEntryGroup *g, AvahiEntryGroupState state,
                                void *userdata) {
    (void)g;
    avahi_ctx_t *avahi = (avahi_ctx_t*)userdata;

    switch (state) {
        case AVAHI_ENTRY_GROUP_ESTABLISHED:
            printf("✓ Service registered on mDNS\n");
            break;

        case AVAHI_ENTRY_GROUP_COLLISION:
            fprintf(stderr, "WARNING: Service name collision\n");
            /* TODO: Rename service */
            break;

        case AVAHI_ENTRY_GROUP_FAILURE:
            fprintf(stderr, "ERROR: Service registration failed: %s\n",
                   avahi_strerror(avahi_client_errno(avahi->client)));
            break;

        case AVAHI_ENTRY_GROUP_UNCOMMITED:
        case AVAHI_ENTRY_GROUP_REGISTERING:
            /* Transitional states */
            break;
    }
}

/*
 * Service browser callback
 * Called when services are found or lost
 */
static void browse_callback(AvahiServiceBrowser *b, AvahiIfIndex interface,
                           AvahiProtocol protocol, AvahiBrowserEvent event,
                           const char *name, const char *type,
                           const char *domain, AvahiLookupResultFlags flags,
                           void *userdata) {
    (void)b;
    (void)interface;
    (void)protocol;
    (void)type;
    (void)domain;
    (void)flags;

    avahi_ctx_t *avahi = (avahi_ctx_t*)userdata;

    switch (event) {
        case AVAHI_BROWSER_NEW:
            printf("INFO: Discovered RootStream service: %s\n", name);

            /* Resolve service to get IP and TXT records */
            /* TODO: avahi_service_resolver_new() with avahi->client */
            /* Extract public key from TXT records */
            /* Add peer if trusted */
            break;

        case AVAHI_BROWSER_REMOVE:
            printf("INFO: RootStream service removed: %s\n", name);
            /* TODO: Remove peer if connected */
            break;

        case AVAHI_BROWSER_FAILURE:
            fprintf(stderr, "ERROR: Browser failed: %s\n",
                   avahi_strerror(avahi_client_errno(avahi->client)));
            break;

        case AVAHI_BROWSER_CACHE_EXHAUSTED:
        case AVAHI_BROWSER_ALL_FOR_NOW:
            /* Informational */
            break;
    }
}

/*
 * Avahi client callback
 * Called when client state changes
 */
static void client_callback(AvahiClient *c, AvahiClientState state,
                           void *userdata) {
    (void)userdata;  /* avahi_ctx_t not currently used */

    switch (state) {
        case AVAHI_CLIENT_S_RUNNING:
            printf("✓ Avahi client running\n");
            /* Now we can register services */
            break;

        case AVAHI_CLIENT_FAILURE:
            fprintf(stderr, "ERROR: Avahi client failed: %s\n",
                   avahi_strerror(avahi_client_errno(c)));
            break;

        case AVAHI_CLIENT_S_COLLISION:
        case AVAHI_CLIENT_S_REGISTERING:
            /* Transitional states */
            break;

        case AVAHI_CLIENT_CONNECTING:
            printf("INFO: Connecting to Avahi daemon...\n");
            break;
    }
}

#endif /* HAVE_AVAHI */

/*
 * Initialize discovery system
 */
int discovery_init(rootstream_ctx_t *ctx) {
    if (!ctx) {
        fprintf(stderr, "ERROR: Invalid context\n");
        return -1;
    }

#ifdef HAVE_AVAHI
    avahi_ctx_t *avahi = calloc(1, sizeof(avahi_ctx_t));
    if (!avahi) {
        fprintf(stderr, "ERROR: Cannot allocate Avahi context\n");
        return -1;
    }

    avahi->ctx = ctx;

    /* Create simple poll object */
    avahi->simple_poll = avahi_simple_poll_new();
    if (!avahi->simple_poll) {
        free(avahi);
        fprintf(stderr, "ERROR: Cannot create Avahi poll object\n");
        return -1;
    }

    /* Create client */
    int error;
    avahi->client = avahi_client_new(
        avahi_simple_poll_get(avahi->simple_poll),
        0,  /* flags */
        client_callback,
        avahi,
        &error);

    if (!avahi->client) {
        fprintf(stderr, "ERROR: Cannot create Avahi client: %s\n",
               avahi_strerror(error));
        avahi_simple_poll_free(avahi->simple_poll);
        free(avahi);
        return -1;
    }

    ctx->discovery.avahi_client = avahi;
    ctx->discovery.running = true;

    printf("✓ Discovery initialized (Avahi)\n");
    return 0;

#else
    fprintf(stderr, "WARNING: Built without Avahi support\n");
    fprintf(stderr, "INFO: Auto-discovery disabled\n");
    return 0;
#endif
}

/*
 * Announce service on network
 */
int discovery_announce(rootstream_ctx_t *ctx) {
    if (!ctx) return -1;

#ifdef HAVE_AVAHI
    avahi_ctx_t *avahi = (avahi_ctx_t*)ctx->discovery.avahi_client;
    if (!avahi || !avahi->client) {
        fprintf(stderr, "ERROR: Discovery not initialized\n");
        return -1;
    }

    /* Create entry group if needed */
    if (!avahi->group) {
        avahi->group = avahi_entry_group_new(avahi->client, 
                                            entry_group_callback, avahi);
        if (!avahi->group) {
            fprintf(stderr, "ERROR: Cannot create Avahi entry group\n");
            return -1;
        }
    }

    /* Prepare TXT records */
    AvahiStringList *txt = NULL;
    char version_txt[64], pubkey_txt[256];
    
    snprintf(version_txt, sizeof(version_txt), "version=%s", ROOTSTREAM_VERSION);
    txt = avahi_string_list_add(txt, version_txt);
    
    snprintf(pubkey_txt, sizeof(pubkey_txt), "code=%s", 
             ctx->keypair.rootstream_code);
    txt = avahi_string_list_add(txt, pubkey_txt);

    /* Add service */
    int ret = avahi_entry_group_add_service_strlst(
        avahi->group,
        AVAHI_IF_UNSPEC,         /* All interfaces */
        AVAHI_PROTO_UNSPEC,      /* IPv4 and IPv6 */
        0,                       /* flags */
        ctx->keypair.identity,   /* Service name */
        "_rootstream._udp",      /* Service type */
        NULL,                    /* Domain (use default) */
        NULL,                    /* Host (use default) */
        ctx->port,               /* Port */
        txt);                    /* TXT records */

    avahi_string_list_free(txt);

    if (ret < 0) {
        fprintf(stderr, "ERROR: Cannot add service: %s\n",
               avahi_strerror(ret));
        return -1;
    }

    /* Commit changes */
    ret = avahi_entry_group_commit(avahi->group);
    if (ret < 0) {
        fprintf(stderr, "ERROR: Cannot commit entry group: %s\n",
               avahi_strerror(ret));
        return -1;
    }

    printf("→ Announcing service on network\n");
    return 0;

#else
    (void)ctx;
    return 0;
#endif
}

/*
 * Browse for services on network
 */
int discovery_browse(rootstream_ctx_t *ctx) {
    if (!ctx) return -1;

#ifdef HAVE_AVAHI
    avahi_ctx_t *avahi = (avahi_ctx_t*)ctx->discovery.avahi_client;
    if (!avahi || !avahi->client) {
        fprintf(stderr, "ERROR: Discovery not initialized\n");
        return -1;
    }

    /* Create browser */
    avahi->browser = avahi_service_browser_new(
        avahi->client,
        AVAHI_IF_UNSPEC,      /* All interfaces */
        AVAHI_PROTO_UNSPEC,   /* IPv4 and IPv6 */
        "_rootstream._udp",   /* Service type */
        NULL,                 /* Domain (use default) */
        0,                    /* flags */
        browse_callback,
        avahi);

    if (!avahi->browser) {
        fprintf(stderr, "ERROR: Cannot create service browser: %s\n",
               avahi_strerror(avahi_client_errno(avahi->client)));
        return -1;
    }

    printf("→ Browsing for RootStream peers...\n");
    return 0;

#else
    (void)ctx;
    return 0;
#endif
}

/*
 * Cleanup discovery
 */
void discovery_cleanup(rootstream_ctx_t *ctx) {
    if (!ctx || !ctx->discovery.running) return;

#ifdef HAVE_AVAHI
    avahi_ctx_t *avahi = (avahi_ctx_t*)ctx->discovery.avahi_client;
    if (!avahi) return;

    if (avahi->browser) {
        avahi_service_browser_free(avahi->browser);
    }

    if (avahi->group) {
        avahi_entry_group_free(avahi->group);
    }

    if (avahi->client) {
        avahi_client_free(avahi->client);
    }

    if (avahi->simple_poll) {
        avahi_simple_poll_free(avahi->simple_poll);
    }

    free(avahi);
    ctx->discovery.avahi_client = NULL;
#endif

    ctx->discovery.running = false;
}
