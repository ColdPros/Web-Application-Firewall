/* 
**  mod_waf.c -- Apache sample waf module
**  [Autogenerated via ``apxs -n waf -g'']
**
**  To play with this sample module first compile it into a
**  DSO file and install it into Apache's modules directory 
**  by running:
**
**    $ apxs -c -i mod_waf.c
**
**  Then activate it in Apache's apache2.conf file for instance
**  for the URL /waf in as follows:
**
**    #   apache2.conf
**    LoadModule waf_module modules/mod_waf.so
**    <Location /waf>
**    SetHandler waf
**    </Location>
**
**  Then after restarting Apache via
**
**    $ apachectl restart
**
**  you immediately can request the URL /waf and watch for the
**  output of this module. This can be achieved for instance via:
**
**    $ lynx -mime_header http://localhost/waf 
**
**  The output should be similar to the following one:
**
**    HTTP/1.1 200 OK
**    Date: Tue, 31 Mar 1998 14:42:22 GMT
**    Server: Apache/1.3.4 (Unix)
**    Connection: close
**    Content-Type: text/html
**  
**    The sample page from mod_waf.c
*/ 

#include "mod_waf.h"

int examineHeaderParams(request_rec *, node *, int );
int isMalicious(request_rec *, const char *, const char *, node *, int );
const char *ADMIN_CONFIG_FILE="/home/dexter/git_working/waf_apache_module/waf/admin_config";

/* The sample content handler */
static int waf_handler(request_rec *r)
{
    int detect_ret;
    MODE_CURRENT = readCurrMode(r);


//Parse signature file to read all the defined signatures to detect anamolies
    parseConfigFile(r);
    // examine GET parameters to check characteristic of malliciousness
    if(gNum >0 && strcmp(r->method, "GET")==0){
        if(examineGetParams(r, gList, gNum)==MALICIOUS){
            ap_set_content_type(r, "text/html");
            ap_rprintf(r,"<body bgcolor=\"#B0E0E6\"><H2><center>Request is blocked by WAF because the request contains a malicious string</center></H2></body> <H1><center>%s</center></H1>\n", illegalStr); 
            return DONE;            
        }
    }
    // examine POST parameters to check characteristic of malliciousness
    if(pNum > 0 && strcmp(r->method, "POST")==0){
        if(examinePostParams(r, pList, pNum)==MALICIOUS){
            ap_set_content_type(r, "text/html");
            ap_rprintf(r,"<body bgcolor=\"#B0E0E6\"><H2><center>Request is blocked by WAF because the request contains a malicious string</center></H2></body> <H1><center>%s</center></H1>\n", illegalStr); 
            return DONE;
        }
    }
    
    //examine HEADER parameters to check characteristic of malliciousness
    if(hNum > 0){
        if(examineHeaderParams(r, hList, hNum)==MALICIOUS){
            ap_set_content_type(r, "text/html");
            ap_rprintf(r,"<body bgcolor=\"#B0E0E6\"><H2><center>Request is blocked by WAF because the request contains a malicious string</center></H2></body> <H1><center>%s</center></H1>\n", illegalStr); 
            return DONE;
        }
    }

    if(MODE_CURRENT == MODE_TRAIN)
    {
        trainModule(r);
    } else if (MODE_CURRENT == MODE_NORMALITY_PROFILE) 
    {
        generateNormalityProfile(r);
        ap_rprintf(r,"<body bgcolor=\"#B0E0E6\"><H2><center>Generate Profile Success</center></H2></body>\n"); 
            return DONE;
        
    } else if (MODE_CURRENT == MODE_DETECT)
    {
        detect_ret = detectAnomolies(r);
        if(detect_ret == PARAM_LEN_ILLEGAL)
        {
            ap_rprintf(r, "<body bgcolor=\"#B0E0E6\"><H1><center>Request is blocked by WAF because the request contains Illegal Parameter Length</center><H1></body> \n");
            return DONE;
        } else if (detect_ret == CONTAINS_NO_SEEN_CHAR)
        {
            ap_rprintf(r, "<body bgcolor=\"#B0E0E6\"><H1><center>Request is blocked by WAF because the request contains Illegal Character Set</center><H1></body> \n");
            return DONE;
        } else if (detect_ret == UNKNOWN_PARAM)
        {
            ap_rprintf(r, "<body bgcolor=\"#B0E0E6\"><H1><center>Request is blocked by WAF because the request contains Unknown Parameter</center><H1></body> \n");
            return DONE;
        } else if(detect_ret == PASS_DETECTION)
        {
            return DECLINED;
        } else if(detect_ret == EXCEED_ALL_MAX_PARAM)
        {
            ap_rprintf(r, "<body bgcolor=\"#B0E0E6\"><H1><center>Request is blocked by WAF because the request exceeds Global Maximum Parameters</center><H1></body>\n");
            return DONE;
        }else if(detect_ret == EXCEED_MAX_PARAM)
        {
            ap_rprintf(r, "<body bgcolor=\"#B0E0E6\"><H1><center>Request is blocked by WAF because the request exceeds Maximum Parameter</center><H1></body> \n");
            return DONE;
        }
    }
    return DECLINED;
}

int examineGetParams(request_rec *r, node *getSigs, int getSigsCount){
    //Parse get request and store in table 
    apr_table_t *GET;
    ap_args_to_table(r, &GET);

    //Get all GET-request parameters from GET table
    const apr_array_header_t *getParamsArr = apr_table_elts(GET);
    const apr_table_entry_t *getParams = (apr_table_entry_t*)getParamsArr->elts;

    //For each GET-request parameter check if the request parameter is mallicious i.e. defined in SignatureList
    //If the parameter is mallicious return MALICIOUS else return NON_MALICIOUS.
    int i = 0;
    for(i = 0; i < getParamsArr->nelts; i++) {
    //    ap_rprintf(r, "examineGetParams :: %s = %s\n", getParams[i].key, getParams[i].val);
        if(0 == isMalicious(r, getParams[i].key, getParams[i].val, getSigs, getSigsCount)){
            return MALICIOUS;
       }
    }
    return NON_MALICIOUS;    
}

kvPair *readPostParams(request_rec *r) {
//    ap_rprintf(r, "Sig r->params %s",r->args);
    apr_array_header_t *pairs = NULL;
    apr_off_t len;
    apr_size_t size;
    int res;
    int i = 0;
    char *buffer;
    kvPair *kvp = NULL;
    
    apr_table_t *t; 
    ap_args_to_table(r, &t);
    

    res = ap_parse_form_data(r, NULL, &pairs, -1, HUGE_STRING_LEN);
    if (res != OK || !pairs) return NULL; 
    kvp = apr_pcalloc(r->pool, sizeof(kvPair) * (pairs->nelts + 1));
    params_size_post = pairs->nelts;
//    ap_rprintf(r, "pairs->nelts %d",pairs->nelts);

    while (pairs && !apr_is_empty_array(pairs)) {
        ap_form_pair_t *pair = (ap_form_pair_t *) apr_array_pop(pairs);
        apr_brigade_length(pair->value, 1, &len);
        size = (apr_size_t) len;
        buffer = apr_palloc(r->pool, size + 1);
        apr_brigade_flatten(pair->value, buffer, &size);
        buffer[len] = 0;
        kvp[i].key = (char *)pair->name;
        kvp[i].value = (char *)buffer;
        // ap_rprintf(r, " key is %s\n", (char *)kvp[i].key);
        // ap_rprintf(r, " val is %s\n", (char *)kvp[i].value);
        i++;
    }
    return kvp;
}

int examinePostParams(request_rec *r, node * postSigs, int postSigsCount){
 //   kvPair *formData = NULL;

    //Get post parameters from request received
    formData = readPostParams(r);
    if (formData) {
        int i;
        for (i = 0; &formData[i]; i++) {
            if (formData[i].key && formData[i].value) {
    //              ap_rprintf(r, "FORM-DATA %s = %s\n",(char *)formData[i].key, (char *)formData[i].value);
                if(0 == isMalicious(r, formData[i].key, formData[i].value, postSigs, postSigsCount)){
                    return MALICIOUS;
                }
            }else {
                break;
            }
        }
    }
    return NON_MALICIOUS;
}

int examineHeaderParams(request_rec *r, node *headerSigs, int headerSigsCount){
    const apr_array_header_t *hdrParamsArr;
    apr_table_entry_t *headerParams = 0;

    //Get Header parameters from request_received
    hdrParamsArr = apr_table_elts(r->headers_in);
    headerParams = (apr_table_entry_t *) hdrParamsArr->elts;
    int i = 0;
    for(i = 0; i < hdrParamsArr->nelts; i++) {
        if(0 == isMalicious(r, headerParams[i].key, headerParams[i].val, headerSigs, headerSigsCount)){
            return MALICIOUS;
        }
    }
    return NON_MALICIOUS;
}

//return 1 is non-malicious and return 0 if malicious 
int isMalicious(request_rec *r, const char* key, const char* value, node * sigs, int sigsCount){    
    int i = 0;
    for(i = 0; i < sigsCount; i++){
        // For GET requests, signatures file has key=* 
        if(strcmp(sigs[i].key,"*")==0 || strcmp(sigs[i].key, key)==0){
                        
            //Check if any substring of GET-parameter is a malicious content
            if(strstr(value, sigs[i].val)!=NULL){
                if(illegalStr == NULL){
                    illegalStr = (char *) calloc(0, BUFF_SIZE);
                }
                strcpy(illegalStr, sigs[i].val);
                return 0;
            }
        }
    }
    return 1;
}

void toLowerCase(char * str){
    int len = strlen(str);
    int i = 0;
    for(i=0; i<len; i++){
        str[i] = tolower(str[i]);
    }
}

int readCurrMode(request_rec *r){
    FILE * fptr;
    fptr = fopen("/home/dexter/git_working/waf_apache_module/waf/admin_config", "r");

    char *line = (char *)malloc(BUFF_SIZE);
    char *curr_mode_raw = (char *)malloc(BUFF_SIZE);
    char *curr_mode = (char *)malloc(BUFF_SIZE);
    if(fptr){
        if (fgets(line, BUFF_SIZE, fptr) != NULL){ 
            curr_mode_raw = strtok(line, "=");
            curr_mode_raw = strtok(NULL, "=");
            strncpy(curr_mode, curr_mode_raw, strlen(curr_mode_raw)-1);
        }
        fclose(fptr);           
    }
    if (line){
        free(line);
    }
    int curr_mode_val = 0;

    int result = strcmp(curr_mode, "MODE_TRAIN") == 0;

    if(0 == (strcmp(curr_mode, "MODE_TRAIN"))){
        curr_mode_val = MODE_TRAIN;
    } else if(0 == (strcmp(curr_mode, "MODE_NORMALITY_PROFILE"))) {
        curr_mode_val = MODE_NORMALITY_PROFILE;
    }else{
        curr_mode_val = MODE_DETECT;
    }
    return curr_mode_val;
}

void read_admin_config_file(request_rec *r, char *usr, char *pwd){
    FILE * fp;
    fp = fopen("/home/dexter/git_working/waf_apache_module/waf/admin_config", "r");

    char *line = (char *)malloc(BUFF_SIZE);
    if(fp) {
        char *buf;
        fgets(line,BUFF_SIZE, fp) != NULL;
        if (fgets(line,BUFF_SIZE, fp) != NULL){            
            buf = strtok(line, "=");
            buf = strtok(NULL, "=");
            strncpy(usr, buf, strlen(buf)-1);
            // ap_rprintf(r, "sud :: read_admin_config_file :: usr %s \n", usr);
            
        }
        if (fgets(line,BUFF_SIZE, fp) != NULL){ 
            buf = strtok(line, "=");
            buf = strtok(NULL, "=");
            strncpy(pwd, buf, strlen(buf)-1);
            // ap_rprintf(r, "sud :: read_admin_config_file :: pwd %s \n", pwd);
        }
        
        fclose(fp);           
    }
    if (line){
        free(line);
    }
}

static void waf_register_hooks(apr_pool_t *p)
{
    ap_hook_post_read_request(waf_handler, NULL, NULL, APR_HOOK_FIRST);
}

/* Dispatch list for API hooks */
module AP_MODULE_DECLARE_DATA waf_module = {
    STANDARD20_MODULE_STUFF, 
    NULL,                  /* create per-dir    config structures */
    NULL,                  /* merge  per-dir    config structures */
    NULL,                  /* create per-server config structures */
    NULL,                  /* merge  per-server config structures */
    NULL,                  /* table of config file commands       */
    waf_register_hooks  /* register hooks                      */
};