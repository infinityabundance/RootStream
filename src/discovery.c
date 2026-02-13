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
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* Discovery timeout for UDP broadcast (milliseconds) */
#define BROADCAST_DISCOVERY_TIMEOUT_MS 1000

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
 * Service resolver callback (enhanced PHASE 17)
 * Called when a discovered service has been resolved
 */
static void resolve_callback(AvahiServiceResolver *r,
                             AvahiIfIndex interface,
                             AvahiProtocol protocol,
                             AvahiResolverEvent event,
                             const char *name,
                             const char *type,
                             const char *domain,
                             const char *host_name,
                             const AvahiAddress *address,
                             uint16_t port,
                             AvahiStringList *txt,
                             AvahiLookupResultFlags flags,
                             void* userdata) {
    (void)interface;
    (void)protocol;
    (void)type;
    (void)domain;
    (void)host_name;
    (void)flags;

    avahi_ctx_t *avahi = (avahi_ctx_t*)userdata;
    rootstream_ctx_t *ctx = avahi->ctx;

    if (event == AVAHI_RESOLVER_FOUND) {
        char addr_str[AVAHI_ADDRESS_STR_MAX];
        avahi_address_snprint(addr_str, sizeof(addr_str), address);

        printf("✓ Resolved RootStream host: %s at %s:%u\n",
               name, addr_str, port);

        /* Extract enhanced TXT records (PHASE 17) */
        peer_cache_entry_t cache_entry = {0};
        strncpy(cache_entry.hostname, name, sizeof(cache_entry.hostname) - 1);
        strncpy(cache_entry.ip_address, addr_str, sizeof(cache_entry.ip_address) - 1);
        cache_entry.port = port;
        cache_entry.discovered_time_us = get_timestamp_us();
        cache_entry.last_seen_time_us = cache_entry.discovered_time_us;
        cache_entry.ttl_seconds = 3600;  /* Default 1 hour TTL */
        cache_entry.is_online = true;
        
        /* Extract RootStream code */
        AvahiStringList *code_txt = avahi_string_list_find(txt, "code");
        if (code_txt) {
            char *key = NULL;
            char *value = NULL;
            size_t value_len = 0;

            if (avahi_string_list_get_pair(code_txt, &key, &value, &value_len) >= 0) {
                strncpy(cache_entry.rootstream_code, value,
                       sizeof(cache_entry.rootstream_code) - 1);
                cache_entry.rootstream_code[sizeof(cache_entry.rootstream_code) - 1] = '\0';
                avahi_free(key);
                avahi_free(value);
            }
        }
        
        /* Extract capability */
        AvahiStringList *capability_txt = avahi_string_list_find(txt, "capability");
        if (capability_txt) {
            char *key = NULL;
            char *value = NULL;
            size_t value_len = 0;
            if (avahi_string_list_get_pair(capability_txt, &key, &value, &value_len) >= 0) {
                strncpy(cache_entry.capability, value,
                       sizeof(cache_entry.capability) - 1);
                avahi_free(key);
                avahi_free(value);
            }
        } else {
            strncpy(cache_entry.capability, "unknown", sizeof(cache_entry.capability) - 1);
        }
        
        /* Extract version */
        AvahiStringList *version_txt = avahi_string_list_find(txt, "version");
        if (version_txt) {
            char *key = NULL;
            char *value = NULL;
            size_t value_len = 0;
            if (avahi_string_list_get_pair(version_txt, &key, &value, &value_len) >= 0) {
                strncpy(cache_entry.version, value,
                       sizeof(cache_entry.version) - 1);
                avahi_free(key);
                avahi_free(value);
            }
        }
        
        /* Extract max_peers */
        AvahiStringList *max_peers_txt = avahi_string_list_find(txt, "max_peers");
        if (max_peers_txt) {
            char *key = NULL;
            char *value = NULL;
            size_t value_len = 0;
            if (avahi_string_list_get_pair(max_peers_txt, &key, &value, &value_len) >= 0) {
                cache_entry.max_peers = (uint32_t)atoi(value);
                avahi_free(key);
                avahi_free(value);
            }
        }
        
        /* Extract bandwidth */
        AvahiStringList *bandwidth_txt = avahi_string_list_find(txt, "bandwidth");
        if (bandwidth_txt) {
            char *key = NULL;
            char *value = NULL;
            size_t value_len = 0;
            if (avahi_string_list_get_pair(bandwidth_txt, &key, &value, &value_len) >= 0) {
                strncpy(cache_entry.bandwidth, value,
                       sizeof(cache_entry.bandwidth) - 1);
                avahi_free(key);
                avahi_free(value);
            }
        }
        
        /* Add to cache */
        discovery_cache_add_peer(ctx, &cache_entry);
        
        /* Add discovered peer to context (backwards compatibility) */
        if (strlen(cache_entry.rootstream_code) > 0 && ctx->num_peers < MAX_PEERS) {
            peer_t *peer = &ctx->peers[ctx->num_peers];

            /* Parse address */
            struct sockaddr_in *addr = (struct sockaddr_in*)&peer->addr;
            addr->sin_family = AF_INET;
            addr->sin_port = htons(port);
            if (avahi_address_parse(addr_str, AVAHI_PROTO_INET,
                                   (AvahiAddress*)&addr->sin_addr) != NULL) {
                /* Store peer info */
                strncpy(peer->hostname, name, sizeof(peer->hostname) - 1);
                peer->hostname[sizeof(peer->hostname) - 1] = '\0';

                strncpy(peer->rootstream_code, cache_entry.rootstream_code,
                       sizeof(peer->rootstream_code) - 1);
                peer->rootstream_code[sizeof(peer->rootstream_code) - 1] = '\0';

                peer->state = PEER_DISCOVERED;
                peer->last_seen = get_timestamp_ms();

                ctx->num_peers++;

                printf("  → Added peer: %s (code: %.8s..., %s)\n",
                       peer->hostname, peer->rootstream_code, cache_entry.capability);
                       
                ctx->discovery.mdns_discoveries++;
            }
        }
    } else {
        fprintf(stderr, "WARNING: Failed to resolve service %s: %s\n",
                name, avahi_strerror(avahi_client_errno(avahi->client)));
    }

    /* Free the resolver */
    avahi_service_resolver_free(r);
}

/*
 * Service browser callback (enhanced PHASE 17)
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
    rootstream_ctx_t *ctx = avahi->ctx;

    switch (event) {
        case AVAHI_BROWSER_NEW:
            printf("INFO: Discovered RootStream service: %s\n", name);

            /* Resolve service to get IP address and TXT records */
            if (!avahi_service_resolver_new(avahi->client, interface, protocol,
                                            name, type, domain,
                                            AVAHI_PROTO_UNSPEC, 0,
                                            resolve_callback, avahi)) {
                fprintf(stderr, "ERROR: Failed to create resolver for %s: %s\n",
                       name, avahi_strerror(avahi_client_errno(avahi->client)));
            }
            break;

        case AVAHI_BROWSER_REMOVE:
            printf("INFO: RootStream service removed: %s\n", name);

            /* Remove from cache */
            discovery_cache_remove_peer(ctx, name);

            /* Find and remove peer (backwards compatibility) */
            for (int i = 0; i < ctx->num_peers; i++) {
                if (strcmp(ctx->peers[i].hostname, name) == 0) {
                    /* Disconnect if connected */
                    if (ctx->peers[i].state == PEER_CONNECTED) {
                        printf("  → Disconnecting peer %s\n", name);
                        ctx->peers[i].state = PEER_DISCONNECTED;
                    }

                    /* Remove from peer list by shifting remaining peers */
                    for (int j = i; j < ctx->num_peers - 1; j++) {
                        ctx->peers[j] = ctx->peers[j + 1];
                    }
                    ctx->num_peers--;

                    printf("  → Removed peer: %s\n", name);
                    break;
                }
            }
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
 * Initialize discovery system with fallback support (PHASE 5)
 */
int discovery_init(rootstream_ctx_t *ctx) {
    if (!ctx) {
        fprintf(stderr, "ERROR: Invalid context\n");
        return -1;
    }

    printf("INFO: Initializing peer discovery...\n");

#ifdef HAVE_AVAHI
    /* Try mDNS/Avahi first (Tier 1) */
    printf("INFO: Attempting discovery backend: mDNS/Avahi\n");
    
    avahi_ctx_t *avahi = calloc(1, sizeof(avahi_ctx_t));
    if (!avahi) {
        fprintf(stderr, "WARNING: Cannot allocate Avahi context\n");
        goto try_broadcast;
    }

    avahi->ctx = ctx;

    /* Create simple poll object */
    avahi->simple_poll = avahi_simple_poll_new();
    if (!avahi->simple_poll) {
        free(avahi);
        fprintf(stderr, "WARNING: Cannot create Avahi poll object\n");
        goto try_broadcast;
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
        fprintf(stderr, "WARNING: Cannot create Avahi client: %s\n",
               avahi_strerror(error));
        avahi_simple_poll_free(avahi->simple_poll);
        free(avahi);
        goto try_broadcast;
    }

    ctx->discovery.avahi_client = avahi;
    ctx->discovery.running = true;

    printf("✓ Discovery backend 'mDNS/Avahi' initialized\n");
    return 0;

try_broadcast:
    fprintf(stderr, "WARNING: mDNS/Avahi failed, trying next...\n");
#endif

    /* Try UDP Broadcast (Tier 2) */
    printf("INFO: Attempting discovery backend: UDP Broadcast\n");
    
    /* UDP broadcast doesn't need initialization, just mark discovery as available */
    ctx->discovery.running = true;
    
    printf("✓ Discovery backend 'UDP Broadcast' initialized\n");
    printf("INFO: Manual peer entry also available (--peer-add)\n");
    
    return 0;
}

/*
 * Announce service on network (with fallback support - PHASE 5, enhanced PHASE 17)
 */
int discovery_announce(rootstream_ctx_t *ctx) {
    if (!ctx) return -1;

#ifdef HAVE_AVAHI
    avahi_ctx_t *avahi = (avahi_ctx_t*)ctx->discovery.avahi_client;
    if (avahi && avahi->client) {
        /* mDNS/Avahi is available, use it */
        
        /* Create entry group if needed */
        if (!avahi->group) {
            avahi->group = avahi_entry_group_new(avahi->client, 
                                                entry_group_callback, avahi);
            if (!avahi->group) {
                fprintf(stderr, "WARNING: Cannot create Avahi entry group\n");
                goto try_broadcast;
            }
        }

        /* Prepare enhanced TXT records (PHASE 17) */
        AvahiStringList *txt = NULL;
        char version_txt[64], pubkey_txt[256], capability_txt[64];
        char max_peers_txt[64], bandwidth_txt[64];
        
        snprintf(version_txt, sizeof(version_txt), "version=%s", ROOTSTREAM_VERSION);
        txt = avahi_string_list_add(txt, version_txt);
        
        snprintf(pubkey_txt, sizeof(pubkey_txt), "code=%s", 
                 ctx->keypair.rootstream_code);
        txt = avahi_string_list_add(txt, pubkey_txt);
        
        /* Add capability: host if we can encode, client if we can decode */
        const char *capability = ctx->is_host ? "host" : "client";
        snprintf(capability_txt, sizeof(capability_txt), "capability=%s", capability);
        txt = avahi_string_list_add(txt, capability_txt);
        
        /* Add max peers capacity */
        snprintf(max_peers_txt, sizeof(max_peers_txt), "max_peers=%d", MAX_PEERS);
        txt = avahi_string_list_add(txt, max_peers_txt);
        
        /* Add bandwidth estimate (simplified) */
        uint32_t bitrate_mbps = ctx->settings.video_bitrate / 1000000;
        snprintf(bandwidth_txt, sizeof(bandwidth_txt), "bandwidth=%uMbps", bitrate_mbps);
        txt = avahi_string_list_add(txt, bandwidth_txt);

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
            fprintf(stderr, "WARNING: Cannot add service: %s\n",
                   avahi_strerror(ret));
            goto try_broadcast;
        }

        /* Commit changes */
        ret = avahi_entry_group_commit(avahi->group);
        if (ret < 0) {
            fprintf(stderr, "WARNING: Cannot commit entry group: %s\n",
                   avahi_strerror(ret));
            goto try_broadcast;
        }

        printf("→ Announcing service on network (mDNS) [%s]\n", capability);
        return 0;
    }

try_broadcast:
#endif

    /* Try UDP broadcast as fallback */
    if (discovery_broadcast_announce(ctx) == 0) {
        printf("→ Announcing service on network (UDP broadcast)\n");
        return 0;
    }
    
    fprintf(stderr, "WARNING: All discovery announce methods failed\n");
    fprintf(stderr, "INFO: Peers can still connect manually (--peer-add)\n");
    return 0;  /* Not fatal - manual entry always works */
}

/*
 * Browse for services on network (with fallback support - PHASE 5)
 */
int discovery_browse(rootstream_ctx_t *ctx) {
    if (!ctx) return -1;

#ifdef HAVE_AVAHI
    avahi_ctx_t *avahi = (avahi_ctx_t*)ctx->discovery.avahi_client;
    if (avahi && avahi->client) {
        /* mDNS/Avahi is available, use it */
        
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
            fprintf(stderr, "WARNING: Cannot create service browser: %s\n",
                   avahi_strerror(avahi_client_errno(avahi->client)));
            goto try_broadcast;
        }

        printf("→ Browsing for RootStream peers (mDNS)...\n");
        return 0;
    }

try_broadcast:
#endif

    /* Try UDP broadcast as fallback */
    printf("→ Browsing for RootStream peers (UDP broadcast)...\n");
    
    /* Listen for broadcast announcements with a short timeout */
    if (discovery_broadcast_listen(ctx, BROADCAST_DISCOVERY_TIMEOUT_MS) > 0) {
        printf("  Found peer via broadcast\n");
    }
    
    return 0;
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

    /* Cleanup cache */
    discovery_cache_cleanup(ctx);
    
    ctx->discovery.running = false;
}

/* ============================================================================
 * PHASE 17: Enhanced Discovery Cache Management
 * ============================================================================ */

/*
 * Add peer to discovery cache
 */
int discovery_cache_add_peer(rootstream_ctx_t *ctx, const peer_cache_entry_t *entry) {
    if (!ctx || !entry) return -1;
    
    /* Check if peer already exists */
    for (int i = 0; i < ctx->discovery.num_cached_peers; i++) {
        if (strcmp(ctx->discovery.peer_cache[i].hostname, entry->hostname) == 0) {
            /* Update existing entry */
            ctx->discovery.peer_cache[i] = *entry;
            ctx->discovery.peer_cache[i].contact_count++;
            return 0;
        }
    }
    
    /* Add new entry if space available */
    if (ctx->discovery.num_cached_peers >= MAX_CACHED_PEERS) {
        fprintf(stderr, "WARNING: Peer cache full, cannot add %s\n", entry->hostname);
        return -1;
    }
    
    ctx->discovery.peer_cache[ctx->discovery.num_cached_peers] = *entry;
    ctx->discovery.num_cached_peers++;
    ctx->discovery.total_discoveries++;
    
    printf("✓ Cached peer: %s (%s:%u)\n", entry->hostname, entry->ip_address, entry->port);
    return 0;
}

/*
 * Update peer's last seen time
 */
int discovery_cache_update_peer(rootstream_ctx_t *ctx, const char *hostname,
                               uint64_t last_seen_time_us) {
    if (!ctx || !hostname) return -1;
    
    for (int i = 0; i < ctx->discovery.num_cached_peers; i++) {
        if (strcmp(ctx->discovery.peer_cache[i].hostname, hostname) == 0) {
            ctx->discovery.peer_cache[i].last_seen_time_us = last_seen_time_us;
            ctx->discovery.peer_cache[i].is_online = true;
            return 0;
        }
    }
    
    return -1;  /* Peer not found */
}

/*
 * Remove peer from cache
 */
int discovery_cache_remove_peer(rootstream_ctx_t *ctx, const char *hostname) {
    if (!ctx || !hostname) return -1;
    
    for (int i = 0; i < ctx->discovery.num_cached_peers; i++) {
        if (strcmp(ctx->discovery.peer_cache[i].hostname, hostname) == 0) {
            /* Shift remaining entries */
            for (int j = i; j < ctx->discovery.num_cached_peers - 1; j++) {
                ctx->discovery.peer_cache[j] = ctx->discovery.peer_cache[j + 1];
            }
            ctx->discovery.num_cached_peers--;
            ctx->discovery.total_losses++;
            return 0;
        }
    }
    
    return -1;  /* Peer not found */
}

/*
 * Get peer from cache
 */
peer_cache_entry_t* discovery_cache_get_peer(rootstream_ctx_t *ctx, const char *hostname) {
    if (!ctx || !hostname) return NULL;
    
    for (int i = 0; i < ctx->discovery.num_cached_peers; i++) {
        if (strcmp(ctx->discovery.peer_cache[i].hostname, hostname) == 0) {
            return &ctx->discovery.peer_cache[i];
        }
    }
    
    return NULL;
}

/*
 * Get all cached peers
 */
int discovery_cache_get_all(rootstream_ctx_t *ctx, peer_cache_entry_t *entries,
                           int max_entries) {
    if (!ctx || !entries || max_entries <= 0) return -1;
    
    int count = ctx->discovery.num_cached_peers;
    if (count > max_entries) count = max_entries;
    
    for (int i = 0; i < count; i++) {
        entries[i] = ctx->discovery.peer_cache[i];
    }
    
    return count;
}

/*
 * Get only online cached peers
 */
int discovery_cache_get_online(rootstream_ctx_t *ctx, peer_cache_entry_t *entries,
                               int max_entries) {
    if (!ctx || !entries || max_entries <= 0) return -1;
    
    int count = 0;
    for (int i = 0; i < ctx->discovery.num_cached_peers && count < max_entries; i++) {
        if (ctx->discovery.peer_cache[i].is_online) {
            entries[count++] = ctx->discovery.peer_cache[i];
        }
    }
    
    return count;
}

/*
 * Expire old cache entries based on TTL
 */
void discovery_cache_expire_old_entries(rootstream_ctx_t *ctx) {
    if (!ctx) return;
    
    uint64_t now_us = get_timestamp_us();
    ctx->discovery.last_cache_cleanup_us = now_us;
    
    /* Iterate backwards to safely remove entries */
    for (int i = ctx->discovery.num_cached_peers - 1; i >= 0; i--) {
        peer_cache_entry_t *entry = &ctx->discovery.peer_cache[i];
        uint64_t age_us = now_us - entry->last_seen_time_us;
        uint64_t ttl_us = (uint64_t)entry->ttl_seconds * 1000000ULL;
        
        if (age_us > ttl_us) {
            printf("INFO: Expiring cached peer: %s (age: %llu sec)\n",
                   entry->hostname, age_us / 1000000ULL);
            discovery_cache_remove_peer(ctx, entry->hostname);
        } else if (age_us > ttl_us / 2) {
            /* Mark as potentially offline if not seen in half TTL */
            entry->is_online = false;
        }
    }
}

/*
 * Cleanup cache
 */
void discovery_cache_cleanup(rootstream_ctx_t *ctx) {
    if (!ctx) return;
    
    ctx->discovery.num_cached_peers = 0;
    memset(ctx->discovery.peer_cache, 0, sizeof(ctx->discovery.peer_cache));
}

/*
 * Print discovery statistics
 */
void discovery_print_stats(rootstream_ctx_t *ctx) {
    if (!ctx) return;
    
    printf("\n=== Discovery Statistics ===\n");
    printf("  Total discoveries:     %lu\n", (unsigned long)ctx->discovery.total_discoveries);
    printf("  Total losses:          %lu\n", (unsigned long)ctx->discovery.total_losses);
    printf("  mDNS discoveries:      %lu\n", (unsigned long)ctx->discovery.mdns_discoveries);
    printf("  Broadcast discoveries: %lu\n", (unsigned long)ctx->discovery.broadcast_discoveries);
    printf("  Manual discoveries:    %lu\n", (unsigned long)ctx->discovery.manual_discoveries);
    printf("  Cached peers:          %d\n", ctx->discovery.num_cached_peers);
    
    if (ctx->discovery.num_cached_peers > 0) {
        printf("\n=== Cached Peers ===\n");
        for (int i = 0; i < ctx->discovery.num_cached_peers; i++) {
            peer_cache_entry_t *entry = &ctx->discovery.peer_cache[i];
            printf("  %d. %s (%s:%u) - %s %s\n", i + 1,
                   entry->hostname,
                   entry->ip_address,
                   entry->port,
                   entry->capability,
                   entry->is_online ? "[online]" : "[offline]");
        }
    }
    printf("\n");
}
