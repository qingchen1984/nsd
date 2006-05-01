/*
 * checkconf - Read and repeat configuration file to output.
 *
 * Copyright (c) 2001-2006, NLnet Labs. All rights reserved.
 *
 * See LICENSE for the license.
 *
 */
#include <config.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include "options.h"
#include "util.h"
#include "dname.h"

#define ZONE_GET_ACL(NAME, VAR) 		\
	if (strcasecmp(#NAME, (VAR)) == 0) { 	\
		quote_acl((zone->NAME)); 		\
		fputs("", stdout); 		\
		return; 			\
	}

#define ZONE_GET_STR(NAME, VAR) 		\
	if (strcasecmp(#NAME, (VAR)) == 0) { 	\
		quote(zone->NAME); 		\
		fputs("", stdout); 		\
		return; 			\
	}

#define ZONE_GET_BINARY(NAME, VAR) 			\
	if (strcasecmp(#NAME, (VAR)) == 0) { 		\
		printf("%s\n", zone->NAME?"yes":"no"); 	\
	}

#define SERV_GET_BINARY(NAME, VAR) 			\
	if (strcasecmp(#NAME, (VAR)) == 0) { 		\
		printf("%s\n", opt->NAME?"yes":"no"); 	\
	}

static char buf[BUFSIZ];

static char *
underscore(const char *s) {
	const char *j = s;
	size_t i = 0;

	while(*j) {
		if (*j == '-') {
			buf[i++] = '_';
		} else {
			buf[i++] = *j;
		}
		j++;
		if (i > BUFSIZ) {
			return NULL;
		}
	}
	buf[i] = '\0';
	return buf;
}

static void
usage(void)
{
	fprintf(stderr, "usage: checkconf [-v] [-o option] [-z zonename] <configfilename>\n");
	exit(EXIT_FAILURE);
}

static void 
print_string_var(const char* varname, const char* value)
{
	if (!value) {
		printf("\t#%s\n", varname);
	} else { 
		printf("\t%s \"%s\"\n", varname, value);
	}
}

static void
quote(const char *v)
{
	printf("%s\n", v);
}

static void 
quote_acl(acl_options_t* acl)
{
	while(acl)
	{
		printf("%s %s\n", acl->ip_address_spec,
			acl->nokey?"NOKEY":(acl->blocked?"BLOCKED":
			(acl->key_name?acl->key_name:"(null)")));
		acl=acl->next;
	}
	fputs("", stdout);
}

static void 
print_acl(const char* varname, acl_options_t* acl)
{
	while(acl)
	{
		printf("\t%s %s %s\n", varname, acl->ip_address_spec,
			acl->nokey?"NOKEY":(acl->blocked?"BLOCKED":
			(acl->key_name?acl->key_name:"(null)")));
		if(1) {
			printf("\t# %s", acl->is_ipv6?"ip6":"ip4");
			if(acl->port == 0) printf(" noport");
			else printf(" port=%d", acl->port);
			if(acl->rangetype == acl_range_single) printf(" single");
			if(acl->rangetype == acl_range_mask)   printf(" masked");
			if(acl->rangetype == acl_range_subnet) printf(" subnet");
			if(acl->rangetype == acl_range_minmax) printf(" minmax");
			if(acl->is_ipv6) {
#ifdef INET6
				char dest[INET6_ADDRSTRLEN+100];
				inet_ntop(AF_INET6, &acl->addr.addr6, dest, sizeof(dest));
				printf(" addr=%s", dest);
				if(acl->rangetype != acl_range_single) {
					inet_ntop(AF_INET6, &acl->range_mask.addr6, dest, sizeof(dest));
					printf(" rangemask=%s", dest);
				}
#else
				printf(" ip6addr-noip6defined");
#endif
			} else {
				char dest[INET_ADDRSTRLEN+100];
				inet_ntop(AF_INET, &acl->addr.addr6, dest, sizeof(dest));
				printf(" addr=%s", dest);
				if(acl->rangetype != acl_range_single) {
					inet_ntop(AF_INET, &acl->range_mask.addr6, dest, sizeof(dest));
					printf(" rangemask=%s", dest);
				}
			}
			printf("\n");
		}
		acl=acl->next;
	}
}


void
config_print_zone(nsd_options_t* opt, const char *o, const char *z)
{
#if 0
	ip_address_option_t* ip;
	key_options_t* key;
#endif
	zone_options_t* zone;

	if (!o) {
		/* need something to work with */
		return;
	}

	if (z) {
		/* look per zone */
		RBTREE_FOR(zone, zone_options_t*, opt->zone_options)
		{
			if (strcasecmp(z, zone->name) == 0) {
				/* -z matches, return are in the defines */
				ZONE_GET_STR(zonefile, o);
				ZONE_GET_ACL(request_xfr, o);
				ZONE_GET_ACL(allow_notify, o);
			}
		}
	} else {
		/* look in the server section */
		SERV_GET_BINARY(ip4_only, o);
		/* and more */	
		/* ... */
	}
}

void 
config_test_print_server(nsd_options_t* opt)
{
	ip_address_option_t* ip;
	key_options_t* key;
	zone_options_t* zone;

	printf("# Config settings.\n");
	printf("server:\n");
	printf("\tdebug-mode: %s\n", opt->debug_mode?"yes":"no");
	printf("\tip4-only: %s\n", opt->ip4_only?"yes":"no");
	printf("\tip6-only: %s\n", opt->ip6_only?"yes":"no");
	print_string_var("database:", opt->database);
	print_string_var("identity:", opt->identity);
	print_string_var("logfile:", opt->logfile);
	printf("\tserver_count: %d\n", opt->server_count);
	printf("\ttcp_count: %d\n", opt->tcp_count);
	print_string_var("pidfile:", opt->pidfile);
	print_string_var("port:", opt->port);
	printf("\tstatistics: %d\n", opt->statistics);
	print_string_var("chroot:", opt->chroot);
	print_string_var("username:", opt->username);
	print_string_var("zonesdir:", opt->zonesdir);
	print_string_var("difffile:", opt->xfrdfile);
	print_string_var("xfrdfile:", opt->xfrdfile);
	printf("\txfrd_reload_timeout: %d\n", opt->xfrd_reload_timeout);

	for(ip = opt->ip_addresses; ip; ip=ip->next)
	{
		print_string_var("ip-address:", ip->address);
	}
	for(key = opt->keys; key; key=key->next)
	{
		printf("\nkey:\n");
		print_string_var("name:", key->name);
		print_string_var("algorithm:", key->algorithm);
		print_string_var("secret:", key->secret);
	}
	RBTREE_FOR(zone, zone_options_t*, opt->zone_options)
	{
		printf("\nzone:\n");
		print_string_var("name:", zone->name);
		print_string_var("zonefile:", zone->zonefile);
		print_acl("allow-notify:", zone->allow_notify);
		print_acl("request-xfr:", zone->request_xfr);
		print_acl("notify:", zone->notify);
		print_acl("provide-xfr:", zone->provide_xfr);
	}
	
}

static int 
additional_checks(nsd_options_t* opt, const char* filename)
{
	ip_address_option_t* ip = opt->ip_addresses;
	zone_options_t* zone;
	key_options_t* key;
	int num = 0;
	int errors = 0;
	while(ip) {
		num++;
		ip = ip->next;
	}
	if(num >= MAX_INTERFACES) {
		fprintf(stderr, "%s: too many interfaces (ip-address:) specified.\n", filename);
		errors ++;
	}

	RBTREE_FOR(zone, zone_options_t*, opt->zone_options)
	{
		const dname_type* dname = dname_parse(opt->region, zone->name); /* memory leak. */
		if(!dname) {
			fprintf(stderr, "%s: cannot parse zone name syntax for zone %s.\n", filename, zone->name);
			errors ++;
		}
	}

	for(key = opt->keys; key; key=key->next)
	{
		const dname_type* dname = dname_parse(opt->region, key->name); /* memory leak. */
		uint8_t data[4000];
		int size;

		if(!dname) {
			fprintf(stderr, "%s: cannot parse tsig name syntax for key %s.\n", filename, key->name);
			errors ++;
		}
		size = b64_pton(key->secret, data, sizeof(data));
		if(size == -1) {
			fprintf(stderr, "%s: cannot base64 decode tsig secret: for key %s.\n", filename, key->name);
			errors ++;
		}
		if(strcmp(key->algorithm, "hmac-md5") != 0)
		{
			fprintf(stderr, "%s: bad tsig algorithm: for key %s.\n", filename, key->name);
			errors ++;
		}
	}

#ifndef BIND8_STATS
	if(opt->statistics > 0)
	{
		fprintf(stderr, "%s: 'statistics: %d' but BIND 8 statistics feature not enabled.\n", 
			filename, opt->statistics);
		errors ++;
	}
#endif
#ifndef HAVE_CHROOT
	if(opt->chroot != 0)
	{
		fprintf(stderr, "%s: chroot %s given. chroot not supported on this platform.\n", 
			filename, opt->chroot);
		errors ++;
	}
#endif
#ifndef INET6
	if(opt->ipv6_only)
	{
		fprintf(stderr, "%s: ipv6_only given but IPv6 support not enabled.\n", filename);
		errors ++;
	}
#endif
	if (strlen(opt->identity) > UCHAR_MAX) {
                fprintf(stderr, "%s: server identity too long (%u characters)\n",
                      filename, (unsigned) strlen(opt->identity));
		errors ++;
        }

	/* not done here: parsing of ip-address. parsing of username. */

        if (opt->chroot) {
                int l = strlen(opt->chroot);

                if (opt->pidfile && strncmp(opt->chroot, opt->pidfile, l) != 0) {
			fprintf(stderr, "%s: pidfile %s is not relative to chroot %s.\n", 
				filename, opt->pidfile, opt->chroot);
			errors ++;
                } 
		if (opt->database && strncmp(opt->chroot, opt->database, l) != 0) {
			fprintf(stderr, "%s: databasefile %s is not relative to chroot %s.\n", 
				filename, opt->database, opt->chroot);
			errors ++;
                }
		if (opt->difffile && strncmp(opt->chroot, opt->difffile, l) != 0) {
			fprintf(stderr, "%s: difffile %s is not relative to chroot %s.\n", 
				filename, opt->difffile, opt->chroot);
			errors ++;
                }
		if (opt->xfrdfile && strncmp(opt->chroot, opt->xfrdfile, l) != 0) {
			fprintf(stderr, "%s: xfrdfile %s is not relative to chroot %s.\n", 
				filename, opt->xfrdfile, opt->chroot);
			errors ++;
                }
        }
	if (atoi(opt->port) <= 0) {
		fprintf(stderr, "%s: port number '%s' is not a positive number.\n", 
			filename, opt->port);
		errors ++;
	}
	if(errors != 0) {
		fprintf(stderr, "%s: parse ok %d zones, %d keys, but %d semantic errors.\n",
			filename, (int)nsd_options_num_zones(opt), 
			(int)opt->numkeys, errors);
	}
	
	return (errors == 0);
}

int 
main(int argc, char* argv[])
{
	int c;
	int verbose = 0; 
	const char * conf_opt = NULL; /* what option do you want? Can be NULL -> print all */
	const char * conf_zone = NULL; /* what zone are we talking about */
	const char* configfile;
	nsd_options_t *options;

        /* Parse the command line... */
        while ((c = getopt(argc, argv, "vo:z:")) != -1) {
		switch (c) {
		case 'v':
			verbose = 1;
			break;
		case 'o':
			conf_opt = optarg;
			break;
		case 'z':
			conf_zone = optarg;
			break;
		default:
			usage();
		};
	}
        argc -= optind;
        argv += optind;
        if (argc == 0 || argc>=2) {
		usage();
	}

	configfile = argv[0];

	/* read config file */
	options = nsd_options_create(region_create(xalloc, free));
	if (!parse_options_file(options, configfile) ||
	   !additional_checks(options, configfile)) {
		exit(EXIT_FAILURE);
	}
	if (conf_opt) {
		config_print_zone(options, underscore(conf_opt), conf_zone);
	} else {
		printf("# Read file %s: %d zones, %d keys.\n", configfile, 
				(int)nsd_options_num_zones(options), 
				(int)options->numkeys);

		if (verbose) {
			config_test_print_server(options);
		}
	}

	exit(EXIT_SUCCESS);
}
