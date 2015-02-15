#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <error.h>
#include <errno.h>
#include <ctype.h> 
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <evhttp.h>
#include <syslog.h>
#include <time.h>
#include <math.h>
#include <signal.h>
#include "cjson/cJSON.h"
#include <inttypes.h>
#include "libqqwry/qqwry.h"
#include <iconv.h>

char *result;
struct timeval before , after;
char *db_path = "./QQWry.Dat";

double time_diff(struct timeval x , struct timeval y)
{
	double x_ms , y_ms , diff;
	
	x_ms = (double)x.tv_sec*1000000 + (double)x.tv_usec;
	y_ms = (double)y.tv_sec*1000000 + (double)y.tv_usec;
	
	diff = (double)y_ms - (double)x_ms;
	
	return diff;
}

char *rtrim_str (char *str)
{
   char *p;
   if (!str)
      return NULL;
   if (!*str)
      return str;
   for (p = str + strlen (str) - 1; (p >= str) && isspace (*p); --p);
   p[1] = '\0';
   return str;
}

char *ltrim_str (char *str){
   char *p;
   if (!str)
      return NULL;
   if (!*str)
      return str;
   for (p = str ; (p <= str) && isspace (*p); ++p);
   str = p;
   return str;

}

char *trim_str (char *str){
   return ltrim_str(rtrim_str(str));
}


char *query_ip(const char *ip) {
    result = (char *) malloc (1024);
    bzero(result, 1024);
    char country[1024] = {'\0'};
    char area[1024] = {'\0'};
    FILE *wry_file;
    wry_file = fopen(db_path,"r");
    qqwry_get_location(country, area, ip, wry_file);
    fclose(wry_file);
    if (strlen(country)>0) {
        sprintf(result, "%s", country);
    }

    if (strlen(area) > 0) {
        if (strlen(country) > 0) {
            sprintf(result, "%s ",result);
        }
        if (strlen(country) <= 0) {
            sprintf(result, "unknown");
        } else {
            sprintf(result, "%s %s",result, area);
        }
    }
    int inlen = strlen(result);
    iconv_t cd = iconv_open("UTF-8","GBK");
    char *outbuf = (char *) malloc (inlen * 4);
    bzero(outbuf,inlen * 4);
    char *in = result;
    char *out = outbuf;
    size_t outlen = inlen * 4;
    iconv(cd, &in, (size_t *)&inlen, &out, &outlen);
    iconv_close(cd);
    strcpy(result,outbuf);
    free(outbuf);
    return result;
}

void process_request(struct evhttp_request *req, void *arg){
    time_t t = time(0);
    char date[32];
    strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", localtime(&t));
    struct evbuffer *buf = evbuffer_new();
    if (buf == NULL) return;
    const char *request;
    struct evkeyvalq request_param;
    request = evhttp_request_uri(req);
    evhttp_parse_query(request, &request_param);
    const char *request_get_data = evhttp_find_header(&request_param, "ip");
    const char *request_get_cmd  = evhttp_find_header(&request_param, "type");
    //const char *http_charset     = evhttp_find_header(&request_param, "charset");
    const char *http_charset     = "utf-8";

    if ( request_get_cmd != NULL ) {
	gettimeofday(&before , NULL);
	char *result = NULL;
	if ( strcmp(request_get_cmd, "getip") == 0 ) {
        	result = query_ip(request_get_data);
	}
	int count =0;
	char *ip_key[5];
	char *p;
	p = (char *) malloc (strlen(result) + 1);
        strcpy(p, result);
	ip_key[0] = malloc(1024);
	while((ip_key[count]=strtok(p, " ") ) !=NULL) {
                count++;
		ip_key[count] = malloc(1024);
                p = NULL;
        }

        cJSON *root,*fmt;
	char *out;        /* declare a few. */
        root=cJSON_CreateObject();
        cJSON_AddItemToObject(root, "ip",        cJSON_CreateString(request_get_data));
        cJSON_AddItemToObject(root, "address",   cJSON_CreateString(result));
        cJSON_AddItemToObject(root, "desc",  fmt=cJSON_CreateObject());
        cJSON_AddNumberToObject(fmt,"code",      200);
        if (ip_key[0] != NULL) {
		char *pos = NULL;
		if ((pos = strstr(ip_key[0], "省")) != NULL) {
			char *tmp[2];
                	int len  = strlen("省");
                	int len0 = pos - ip_key[0] + len;
                	int len1 = strlen(ip_key[0]) - len0;
                	tmp[0] = malloc(len0);
                	tmp[1] = malloc(len1);
                	strncpy(tmp[0], ip_key[0], len0);
                	strncpy(tmp[1], pos + len, len1);
			cJSON_AddStringToObject(fmt,"province", trim_str(tmp[0]));
			cJSON_AddStringToObject(fmt,"city", trim_str(tmp[1]));
		} else {
			if (strstr(ip_key[0], "市") != NULL) {
				cJSON_AddStringToObject(fmt,"city", ip_key[0]);
			} else {
				cJSON_AddStringToObject(fmt,"country", ip_key[0]);
			}
		}
	}
        if (count >=1 && ip_key[1] != NULL) {
		cJSON_AddStringToObject(fmt,"desc", ip_key[1]);
	}

        if (count >=2 && ip_key[2] != NULL) {
		cJSON_AddStringToObject(fmt,"company", ip_key[2]);
	}
	gettimeofday(&after , NULL);
	char diff_time[25];
	sprintf(diff_time, "%0.8f", time_diff(before , after) / CLOCKS_PER_SEC);
        cJSON_AddStringToObject(fmt, "time", diff_time);
        out=cJSON_Print(root);
        cJSON_Delete(root);
        if(http_charset !=NULL){
            char *content_type = (char *)malloc(64);
            memset(content_type,'\0',64);
            sprintf(content_type,"text/plain; charset=%s",http_charset);
            evhttp_add_header(req->output_headers,"Content-Type",content_type);
            free(content_type);
        } else {
            evhttp_add_header(req->output_headers,"Content-Type","text/plain");
        }
	evbuffer_add_printf(buf, "%s", out);
        free(out);      /* Print to text, Delete the cJSON, print it, release the string. */
    }
    evhttp_send_reply(req, HTTP_OK, "OK", buf);
}                                        

int is_ip_start(const char *buf) {
        char *p;
	char *tmp;
        p = (char *)malloc(strlen(buf) + 1);
        strcpy(p, buf);
        tmp = p;
        int ip_max_len = 15;
        int dot_count = 0;
        int step  = 1;
        int len = 0;
        while (*p != '\0' || len <= ip_max_len) {
                if (isspace(*p)) {
                        break;
                }
                if (step == 1 &&  !isdigit(*p)) {
                        break;
                }
                if (step > 1 && (!isdigit(*p) && *p != '.')) {
                        break;
                }

                if (step > 4) {
                        break;
                }
                step++;
                if (*p == '.') {
                        dot_count = dot_count + 1;
                        step = 1;
                }
                p++;
                len++;

        }
	free(tmp);
	tmp = NULL;
        if (dot_count != 3) {
                return 0;
        }
        return 1;
}

int file_exists(char *filename) {
	return (access(filename, 0) == 0);
}




static void xip_daemon() {
    pid_t pid;

    /* Fork off the parent process */
    pid = fork();

    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);

    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);

    /* On success: The child process becomes session leader */
    if (setsid() < 0)
        exit(EXIT_FAILURE);

    /* Catch, ignore and handle signals */
    //TODO: Implement a working signal handler */
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    /* Fork off for the second time*/
    pid = fork();

    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);

    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);

    /* Set new file permissions */
    umask(0);

    /* Change the working directory to the root directory */
    /* or another appropriated directory */
    chdir("/");

    /* Close all open file descriptors */
    int x;
    for (x = sysconf(_SC_OPEN_MAX); x>0; x--)
    {
        close (x);
    }

    /* Open the log file */
    openlog ("xip_daemon", LOG_PID, LOG_DAEMON);
}


int run_http(char *host, int port) {
	struct event_base *base = NULL;
        struct evhttp *httpd = NULL;
        base = event_init();
        if (base == NULL) return -1;
        httpd = evhttp_new(base);
        if (httpd == NULL) return -1;
        if (evhttp_bind_socket(httpd, host, port) != 0) return -1;
        evhttp_set_gencb(httpd, process_request, NULL);
        event_base_dispatch(base);
	return 0;
}

static void show_help(void) {
    char *b = "--------------------------------------------------------------------------------------------------\n"
            "Author: qidasheng, E-mail: qsf.zzia1@hotmail.com\n"
            "\n"
            "-l <ip_addr>    监听地址\n"
            "-p <num>        监听端口\n"
            "-P <path>       纯真数据库路径,默认./QQWry.Dat\n"
            "-d              后台运行\n"
            "\n"
            "Please visit \"https://github.com/qidasheng/xip\" for more help information.\n\n"
            "--------------------------------------------------------------------------------------------------\n"
            "\n";
    fprintf(stderr, b, strlen(b));
    exit(0);
}

int main (int argc, char *argv[]) {
        int oc;
	char *host = NULL;
	int  port = 0;
        int daemon = 0;
        while((oc = getopt(argc, argv, "l:p:P:d")) != -1)
        {
                switch(oc)
                {
                        case 'l':
                                host = optarg;
                                break;
                        case 'p':
                                port = atoi(optarg);
                                break;
                        case 'P':
                                db_path = optarg;
                                break;
                        case 'd':
                                daemon = 1;
                                break;
                        case 'h':
                                show_help();
                                break;
                        case '?':
                                show_help();
                                break;
                        default:
                                show_help();
                }
        }

	if (host == NULL || port == 0) {
		show_help();
	}

        if( !file_exists(db_path) )   {
                printf("%s :No such file\r\n", db_path);
	}

	if (daemon == 1) {
		xip_daemon();
	}
	result = (char *) malloc (1024);
        bzero(result, 1024);
	run_http(host, port);
	return 0;
}

