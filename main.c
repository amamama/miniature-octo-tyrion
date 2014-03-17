#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <oauth.h>
#include <curl/curl.h>

#include "tweet.h"
#include "keys.h"

static char **alloc_strcat(char **dest, char const *src) {
	#ifdef DEBUG
	puts(__func__);
	#endif

	int destlen = 0, srclen = 0,wasdestnull = 0;
	destlen = (*dest)?strlen(*dest):0;
	srclen = strlen(src);
	wasdestnull = !(*dest);
	*dest = (char*)realloc(*dest, (destlen + srclen + 1)*sizeof(char));
	if (dest) {
		if (wasdestnull) {
			memset(*dest, 0, strlen(src) + 1);
		}
		strncat(*dest, src, strlen(src));
	} else {
		fprintf(stderr,"realloc failed\n");
	}
	return dest;
}

/* Array of skip-bytes-per-initial character.
 *  */
inline static char *utf8_next_char(char *p) {
	const char utf8_skip_data[256] = {
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
		2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
		3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,6,6,1,1
	};
	return (char *)((p) + utf8_skip_data[*(unsigned char *)(p)]);
}

static size_t write_data(char *buffer, size_t size, size_t nmemb, void *rep) {
	*(buffer + size * nmemb) = '\0';
	alloc_strcat((char**)rep, buffer);

	return size * nmemb;
}

#define WIZARDS_URL "http://gatherer.wizards.com"
#define FLAVOR_ID "<div id=\"ctl00_ctl00_ctl00_MainContent_SubContent_SubContent_FlavorText\" class=\"value\">\r\n                            <div class=\"cardtextbox\">"

char **get_html(char **rep) {
	if (*rep) {
		memset(*rep,0,strlen(*rep));
	}
	CURL *curl;
	CURLcode ret;
	curl = curl_easy_init();
	if (!curl) {
		fprintf(stderr, "failed to initialize curl\n");
	}
	curl_easy_setopt(curl, CURLOPT_URL,WIZARDS_URL "/Pages/Card/Details.aspx?action=random");
	curl_easy_setopt(curl, CURLOPT_COOKIE, "CardDatabaseSettings=1=ja-JP;");
	//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt (curl, CURLOPT_WRITEDATA, (void *) rep);
	curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, write_data);
	ret = curl_easy_perform (curl);
	if (ret != CURLE_OK) {
		fprintf (stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror (ret));
	}
	curl_easy_cleanup(curl);

	return rep;
}

char **replace_tag(char *text, char **rep) {
	if (*rep) {
		memset(*rep,0,strlen(*rep));
	}

	int len = strlen(text);
	char *flavor_start = strstr(text, "<i>");
	char *flavor_end = (flavor_start += 2);

	for (; (flavor_start - text) < len; flavor_start = utf8_next_char(flavor_start)) {
		if (*flavor_start != '>') {
			continue;
		} else {
			for (flavor_end = flavor_start;(flavor_end - text) < len; flavor_end = utf8_next_char(flavor_end)) {
				if (*flavor_end != '<') {
					continue;
				} else if (flavor_end == utf8_next_char(flavor_start)){
					break;
				} else {
					*flavor_end = '\0';
					alloc_strcat(rep, utf8_next_char(flavor_start));
					alloc_strcat(rep, "\n");
					flavor_start = flavor_end;
					break;
				}
			}
		}
	}
	
	return rep;
}

int main(void) {

	char *rep = NULL, *dummy = NULL;
	char *flavor_start = NULL;
	char *flavor = NULL;

	bear_init(&(union KEYS){.keys_array = {c_key, c_sec, t_key, t_sec}});
	curl_global_init(CURL_GLOBAL_DEFAULT);

	do {
		get_html(&rep);
		flavor_start = strstr(rep, FLAVOR_ID);
	} while(!flavor_start);

	flavor_start += strlen(FLAVOR_ID);
	*(strstr(flavor_start, "\r\n") - 12) = '\0';
	alloc_strcat(&flavor, flavor_start);
	free(rep); rep = NULL;

	replace_tag(flavor, &rep);

	post_statuses_update(rep, &dummy, 0, 0, (struct GEOCODE){0,0,0,""}, 0, 0, 0);

	free(rep); rep = NULL;
	free(dummy); dummy = NULL;
	free(flavor); flavor = NULL;
	bear_cleanup();
	curl_global_cleanup();

	return 0;
}

