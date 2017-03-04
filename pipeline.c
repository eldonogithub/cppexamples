/* 
 * Ahamed 
 * Using cURL libcurl
 *
 * For sending pipelined http requests
 * Usage : ./send_pipelined_http <no of pipelined requests> <http://x.x.x.x>
 * Some basic validations added.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

static void usage();

#define ERR_LEN 512
#define MIN_URL_LEN 14
#define MAX_URL_LEN 32 
#define MIN_PIPELINE_REQ 1
#define MAX_PIPELINE_REQ 32
#define err(x, y) {printf("\nError: %s...\nExiting...\n", x); if(y)exit(0); usage();}

static unsigned int pipeline_req;
static char url[MAX_URL_LEN];
static char err_str[ERR_LEN];

static void handle_curl_error(CURLcode ret, char *func)
{
  if(ret != CURLE_OK){
    fprintf(stderr, "%s() failed: %s\n", func,
            curl_multi_strerror(ret));
  }
  return;
}

static void usage()
{
  printf("\nDeveloped by Ahamed using cURL API via libcurl\n");
  printf("Usage : ./send_pipelined_http <no of req> <http://x.x.x.x>\n");
  printf("        Pipeline Request - Min %d : Max : %d\n", 
                  MIN_PIPELINE_REQ, MAX_PIPELINE_REQ);
  printf("        Hostname not supported yet. Please provide IP address.\n");
  printf("        URL - Min Length %d : Max Length : %d\n\n", 
                  MIN_URL_LEN, MAX_URL_LEN);
  exit(0);
}

static void parse_args(int argc, char **argv)
{

  if(argc <= 2)
    err("Insufficient arguments", 0);

  if(!argv)
    err("Something unexpected happened", 1);

  pipeline_req = atol(argv[1]);
  if(pipeline_req > MAX_PIPELINE_REQ || pipeline_req < MIN_PIPELINE_REQ)
    err("Invalid Pipeline Request", 0);
  
  if(strlen(argv[2]) > MAX_URL_LEN || strlen(argv[2]) < MIN_URL_LEN)
    err("Invalid URL length", 0);

  strncpy(url, argv[2], MAX_URL_LEN);
  
  return;
}
 
int main(int argc, char **argv)
{

  int i=0;
  parse_args(argc, argv);

  CURLM *m_curl;
  CURLMcode res;

  m_curl = curl_multi_init();
  curl_multi_setopt(m_curl, CURLMOPT_PIPELINING, 1L);

  CURL *curl[MAX_PIPELINE_REQ];
  for(i=0; i<pipeline_req; i++){

    curl[i] = curl_easy_init();
    if(!curl[i])
      err("Something went wrong", 0);

    res = curl_easy_setopt(curl[i], CURLOPT_URL, url);
    handle_curl_error(res, "curl_easy_setopt");

    res = curl_easy_setopt(curl[i], CURLOPT_FOLLOWLOCATION, 1L);
    handle_curl_error(res, "curl_easy_setopt");

    res = curl_multi_add_handle(m_curl, curl[i]);
    handle_curl_error(res, "curl_multi_add_handle");
  }

  int ret = 1;
  do {
    res = curl_multi_perform(m_curl, &ret);
  } while(ret);

  handle_curl_error(res, (char*)__FUNCTION__);

  for(i=0; i<pipeline_req; i++){
    curl_multi_remove_handle(m_curl, curl[i]); 
    curl_easy_cleanup(curl[i]);
  }
  curl_multi_cleanup(m_curl);

  return 0;
}
