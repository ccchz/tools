#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "include\uv.h"
#include "http_parser\http_parser.h"
#include <assert.h>
#include "include\libconfig.h"
#include <signal.h>
#include <iconv.h>
#include <winspool.h>
#include "webppd.h"
#include "favicon.h"



uv_loop_t *loop;	
uv_tcp_t Print_svr;
uv_signal_t signal1;
int rcount;
char * webppd_cfg[CFG_MAX];


int g2u(char * inbuf, char * outbuf, int len){
	iconv_t cc;
	int ilen,olen;
	char * inb,* outb;
	
	inb = inbuf;
	outb = outbuf;
	ilen = strlen(inbuf);
	//printf("len:%d\n",ilen);
	olen = len;
	cc = iconv_open("UNICODELITTLE","GB2312");
	if (cc == (iconv_t) -1) return 1;
	if((int)iconv(cc,&inb,(size_t *)&ilen,&outb,(size_t *)&olen) == -1) {
		iconv_close(cc);
		return 1;
	}

	iconv_close(cc);
	return 0;
}

 void * new_str_pair( ){
		struct str_pair_t *p = malloc(sizeof(struct str_pair_t));
		p -> next = NULL;
		p -> key = NULL;
		p -> value = NULL;
		return p;
}

static int del_str_pair(struct str_pair_t * p){
		if(p -> key != NULL ) {
		//	printf("%s:",p->key);
			free(p -> key);
		}
		if(p -> value != NULL){
		//	printf("%s\n",p->value);
			free(p-> value);
		}
		free(p);
		return 0;
}

static void * new_req_http() {
		struct req_http_t *p = malloc(sizeof(struct req_http_t));
		memset(p,0,sizeof(struct req_http_t));
/*		p -> root = NULL;
		p -> Current = NULL;
		p -> method = NULL;
		p -> url = NULL;
		p -> body = NULL;
		p -> flags = 0;
		p -> major = 0;
		p -> minor = 0;
*/
		return p;
}

int free_array(char **array,unsigned int count) {
	unsigned int i;
	if(array) {
		for (i=0;i< count;i++) {
			if (array[i]){
				printf("%s:%s\n",url_field_str[i],array[i]);
				free(array[i]);
		
			}
		}
	return 0;
	}
 return 1;
}

static int del_req_http(struct req_http_t * p){
		struct str_pair_t *n;
		struct str_pair_t *q;
	//	int i;

		if (p == NULL){ return 1; }
		q=(struct str_pair_t *) p ->root;

		if (p -> url != NULL ) {
		//	printf("url:%s\n",p->url);
			free(p -> url);
		}
		if (p -> body != NULL) {
		//	printf("body:%s\n",p->body);
			free (p -> body);
		}
		if(p -> hfile){
			if(strcmp(webppd_cfg[2],"port") ==0){
				CloseHandle((HANDLE)p ->hfile);
			}else {
				EndPagePrinter((HANDLE)p ->hfile);
				EndDocPrinter((HANDLE)p ->hfile);
				ClosePrinter((HANDLE)p ->hfile);
			}
		}
		if(p -> cc){
			
			iconv_close(p->cc);
		}
		while (q != NULL){
			n = q;
			q = q -> next;
			del_str_pair(n);
		}
/*		for(i=0;i<UF_MAX;i++) {
			if (p->url_field[i] !=NULL){
				printf("%s:%s\n",url_field_str[i],p->url_field[i]);       
				free(p->url_field[i]);
			}
		}
*/
		free_array(p->url_field,UF_MAX);
		free (p);
		return 0;
}					

static void * alloc_copy( const char * buf ,size_t len){
			char * p = malloc(len+1);
			memset(p,0,len+1);
			memcpy(p,buf,len);
			p[len] = '\0';
			return p;
}

int find_str_pair(str_pair_t *p,char * key,char * value){

	while(p){
		if(strcasecmp(p->key,key) !=0 && strcasecmp(p->value,value)!=0){
			p = p->next;
			continue;
		}

	//	printf("%s:%s\n",p->key,p->value);
		return 1;
	}
	
	return 0;
}

static int cb_url(http_parser * parser ,const char * buf ,size_t len){
		struct req_http_t *p = (struct req_http_t *) parser -> data;
		p -> method = (char *) http_method_str(parser -> method);
		p -> major = parser -> http_major;
		p -> minor = parser -> http_minor;
		p -> url = alloc_copy(buf,len);
		return 0;
}

static int add_str_pair(struct req_http_t * p){
			struct str_pair_t *q =(struct str_pair_t *) &p->root;
			while (q -> next != NULL){
					q = q -> next;
			}
			q -> next = new_str_pair();
			p -> Current = q ->next;
			return 0;
}

static int cb_key (http_parser * parser ,const char * buf ,size_t len){
		struct req_http_t *p = (struct req_http_t *) parser -> data;
		add_str_pair(p);
		p -> Current ->key = alloc_copy(buf,len);
		return 0;
}
		
static int cb_value (http_parser * parser ,const char * buf ,size_t len){
		struct req_http_t *p = (struct req_http_t *) parser -> data;

		p -> Current ->value = alloc_copy(buf,len);
		return 0;
}

static int cb_body (http_parser * parser ,const char * buf ,size_t len){
//		printf("on_body.\n");
		struct req_http_t *p = (struct req_http_t *) parser -> data;
		if(p->body) free(p->body);
		p -> body = alloc_copy(buf,len);
		return 0;
}

static int on_msg_begin (http_parser * parser) {
//	printf("on_msg_begin.\n");
		struct req_http_t * req ;
		req = new_req_http();
		parser ->data = req;
	return 0;
}

static int on_msg_end (http_parser * parser) {
//	printf("on_msg_end.\n");
	struct req_http_t *req_http = (struct req_http_t *) parser -> data;
	req_http->msgend = MSG_END;
	return 0;
}

static int on_header_end (http_parser * parser) {
//	printf("on_header_end.\n");
	int i;
	struct http_parser_url *parser_url;
	struct req_http_t *req_http = (struct req_http_t *) parser -> data;
	req_http->state = HEADER_END;
	parser_url = (struct http_parser_url *) &req_http->parser_url;

	http_parser_url_init(parser_url);

	i = http_parser_parse_url((char *)req_http->url,strlen(req_http->url),0,parser_url);
	if(!i){

		for (i = 0 ; i < UF_MAX; i++) {
		if(parser_url->field_set & (1<<i)) {
		//printf("len:%d--off:%d\n", parser_url->field_data[i].len, parser_url->field_data[i].off);
			req_http->url_field[i] = alloc_copy((char *)req_http->url+ parser_url->field_data[i].off, parser_url->field_data[i].len);
		}
		}
	}

	return 0;
}

static http_parser_settings parser_setting = {
		.on_message_begin = on_msg_begin,
		.on_message_complete = on_msg_end,
		.on_headers_complete = on_header_end,
		.on_header_field = cb_key,
		.on_header_value = cb_value,
		.on_url = cb_url,
		.on_body = cb_body
	};
	
static void alloc_buffer(uv_handle_t* handle, size_t suggested_size,uv_buf_t *buf)
{
	*buf = uv_buf_init(memset(malloc(suggested_size),0,suggested_size), suggested_size);
}

static void cb_write(uv_write_t *w,int status){
		free(w);
}

static void cb_close(uv_handle_t * handle){
		free(handle);
}

static void Print_write(uv_stream_t * stream,void* data,int len) {
	uv_buf_t buf;
	uv_write_t *w;

	if (data == NULL || len == 0) return;
	if (len == -1)
		len = strlen(data);
	buf = uv_buf_init(data,len);
	w = malloc(sizeof(uv_write_t));
	uv_write(w,stream,&buf,1,cb_write);
//	uv_close((uv_handle_t *)stream,NULL);
//	free(stream);
}

int printopen(req_http_t *req){
	char buf0[1024],buf1[1024];
	DOC_INFO_1 doc;
	
	if(strcmp(webppd_cfg[2],"port") ==0){
		sprintf(buf0,"\\\\.\\%s",webppd_cfg[3]);
		printf("%s\n",buf0);
		memset(buf1,0,1024);
		if(g2u(buf0,buf1,1024)){
		//	printf("errno:%d\n",errno);
			return 1;
		}
		req->hfile = CreateFileW((LPCWSTR)buf1, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0,NULL);
		if ((HANDLE)req->hfile == INVALID_HANDLE_VALUE) {
			return 1;
		}
	}else{
		sprintf(buf0,"%s",webppd_cfg[3]);
		printf("%s\n",buf0);
		memset(buf1,0,1024);
		if(g2u(buf0,buf1,1024)){
		//	printf("errno:%d\n",errno);
			return 1;
		}
		if (!OpenPrinterW((LPWSTR)buf1,(PHANDLE) &req->hfile, 0)) {
		//	printf("err:%d\n",errno);
			return 1;
		}
		doc.pDatatype = TEXT("TEXT");
		doc.pDocName = NULL;
		doc.pOutputFile = NULL;
		if (!StartDocPrinter((HANDLE)req->hfile, 1,(LPBYTE) &doc)) {
			ClosePrinter((HANDLE)req->hfile);
			return 1;
		}
		if (! StartPagePrinter((HANDLE)req->hfile)){
			ClosePrinter((HANDLE)req->hfile);
			return 1;
		}
	}
	return 0;	
}

static int http_proc(uv_stream_t *stream, ssize_t nread, const uv_buf_t* buf) {
	struct http_parser * parser;
	struct req_http_t * req;
	char *p, *q;
	char buf0[1024],buf1[1024];
	char *tmpbuf;
	int wb,len,olen;

	p=q=NULL;
	buf0[0]=0;
	buf1[0]=0;
	parser = (struct http_parser *) stream->data;
	req = (struct req_http_t *) parser->data;
// 读卡器处理 。
	if(strcmp(req->url_field[UF_PATH],"/readcard") == 0 ){
		if(!req->url_field[UF_QUERY]) return 4;
		p=strstr(req->url_field[UF_QUERY],"callback");
		if(p) {
			while(*p) {
				if(*p == '='){ q=p;
				}
				else if(*p == '&') {
					break;
				}
				p++;
			}
			if(q ==0|| (*q != '=')) {

				return 4;
			}
			else {
				q++;
			}	
			strncat(buf1,q,p-q);
			strcat(buf1,"({\"id\":\"6001\"})");
			sprintf(buf0,http_readcard,(uint64_t)strlen(buf1));
			strcat(buf0,buf1);
//			printf("buf:%s\n",buf0);
			Print_write(stream,buf0,-1);
			if(req->msgend){
				return 6;
			}
	
		}
		return 4;
	}
// 打印处理。
	if(strcmp(req->url_field[UF_PATH],"/print") == 0 ){

	
		if(req->body){
			if(!req->hfile){
				if(printopen(req))
					return 5;
			}
			if(find_str_pair(req->root,"charset","gb2312")){

				if(strcmp(webppd_cfg[2],"port") ==0){
					WriteFile((HANDLE)req->hfile, req->body, (DWORD)strlen(req->body),(DWORD*) &wb, 0);
				}else{
					WritePrinter((HANDLE)req->hfile, req->body, (DWORD)strlen(req->body),(DWORD*) &wb);	
				}
			}else{
			
				//printf("utf-8\n");
				if(!req->cc){
					req->cc = iconv_open("GBK","UTF-8");
					if(req->cc ==(iconv_t) -1){
						return 5;
					}
				}


				len=strlen(req->body);
				if(req->incompchar[0] != '\0'){
					len = len + strlen(req->incompchar);
					p=malloc(len+1);
					memset(p,0,len+1);
					strcpy(p,req->incompchar);
					strcat(p,req->body);
					free(req->body);
					req->body=p;
				}
				olen = len;
				tmpbuf=malloc(len+1);
				memset(tmpbuf,0,len+1);
				p=req->body;q=tmpbuf;
				memset(req->incompchar,0,8);
				if((int)iconv(req->cc,&p,(size_t*)&len,&q,(size_t*)&olen) == -1){
					if(errno != EINVAL){
						return 5;
					}
					strncpy(req->incompchar,p,8);
				}
				if(strcmp(webppd_cfg[2],"port") ==0){
					WriteFile((HANDLE)req->hfile, tmpbuf, (DWORD)strlen(req->body),(DWORD*) &wb, 0);
				}else{
					WritePrinter((HANDLE)req->hfile, tmpbuf, (DWORD)strlen(req->body),(DWORD*) &wb);	
				}
			//	printf("free..tmpbuf\n");
				free(tmpbuf);
					
			}
		}
		if(req->msgend) {
		//	Print_write(stream,http_ok,-1);
			return 2;
		}

		return 0;
	}
// favicon.ico  response...
	if((strcmp(req->url_field[UF_PATH],"/favicon.ico") == 0)){
		sprintf(buf0,http_data,"image/x-icon",favicon_lens);	
		Print_write(stream,buf0,-1);
		Print_write(stream,favicon,favicon_lens);
		if(req->msgend){
			return 6;
		}
	}
	return 1;
}
static void Print_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
	int res,i;
	struct http_parser * parser;
	struct req_http_t * req;
	parser = (struct http_parser *) stream->data;


	if (nread < 0) {

		assert(nread == UV_EOF);
		printf("Socket Closed!!\n");
		free(parser);
		if(buf->base) {
			free(buf->base);
		}
		uv_close((uv_handle_t *)stream,cb_close);

		return ;
	}

	if (nread == 0 ){
		free(buf->base);       
		return;
	}
  	res = http_parser_execute(parser,&parser_setting,buf->base,nread);
  	//res = http_parser_execute(parser,&parser_setting,http_ttt,strlen(http_ttt));
	//printf("content_length:%I64u\n",parser->content_length);
  	//res = http_parser_execute(parser,&parser_setting,http_ttt1,strlen(http_ttt1));

	
	req = (struct req_http_t *) parser->data;
	
	if( parser->upgrade == 1 || res != nread ){
		

		Print_write(stream,http_400,-1);
		del_req_http((struct req_http_t *)req);
		free(parser);
		uv_close((uv_handle_t*)stream,cb_close);	
	}

	

	if(req->state == HEADER_END) {
		i =  http_proc(stream,nread,buf);
		if(i){
			Print_write(stream,resp_str[i],-1);
			del_req_http((struct req_http_t *)req);
			free(parser);
			uv_close((uv_handle_t*)stream,cb_close);
		}	
	}
	free(buf->base);
}

static void Print_on_connection(uv_stream_t* Svr,int status)
{
	if (status == 0) {

	
	uv_tcp_t *client = (uv_tcp_t*) malloc (sizeof(uv_tcp_t));
	uv_tcp_init(loop,client);
	if (uv_accept(Svr,(uv_stream_t*) client) ==0) {
		rcount++;
		printf("request:%d\n",rcount);
		struct http_parser * parser = malloc(sizeof(http_parser));
		memset(parser,0,sizeof(http_parser));
		http_parser_init(parser,HTTP_REQUEST);

		client ->data = parser;
		uv_read_start((uv_stream_t *)client,alloc_buffer,Print_read);

		}
	else {
	printf("Accept error!!!\n");
	uv_close((uv_handle_t*) client,cb_close);
		}
	}
}





int init_conf(){
	
  config_t cfg;
  const char *str;
  unsigned int i;

  config_init(&cfg);
  memset(webppd_cfg,0,CFG_MAX*sizeof(char *));
  /* Read the file. If there is an error, report it and exit. */
  if(! config_read_file(&cfg, "webppd.conf"))
  {
    fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
            config_error_line(&cfg), config_error_text(&cfg));
    config_destroy(&cfg);
    return 1;
  }

  /* Get the store name. */
  for(i=0;i<ARRAY_SIZE(str_cfg);i++) {
 	 if(config_lookup_string(&cfg, str_cfg[i], &str)) {
		 webppd_cfg[i] = alloc_copy(str,strlen(str));
  		  printf("%s: %s\n", str_cfg[i],webppd_cfg[i]);
	 }
 	 else
  		  fprintf(stderr, "No '%s' setting in configuration file.\n",str_cfg[i]);
	}

  config_destroy(&cfg);
 ;
  return 0;


}

static void signal_cb(uv_signal_t * handle,int signum) {
	if(signum == SIGINT){
		printf("Signal INT...\n");
		uv_signal_stop(handle);
		uv_stop(loop);
	}
}

int main() 
{	
	loop = uv_default_loop();	
	rcount = 0;
	struct sockaddr_in bind_addr;
	int r;
	
	if(init_conf()) {
		printf("Error reading conf file...\n");
		return 1;
	}
	uv_signal_init(loop,&signal1);
	uv_signal_start(&signal1,signal_cb,SIGINT);
		
	printf("Starting...\n");
	uv_ip4_addr(webppd_cfg[0]?webppd_cfg[0]:"0.0.0.0",webppd_cfg[1]?atoi(webppd_cfg[1]):7789,&bind_addr);
	uv_tcp_init(loop,&Print_svr);
	printf("Binding...\n");
	uv_tcp_bind(&Print_svr,(const struct sockaddr *)&bind_addr,0);
	printf("Listening...\n");
	r =  uv_listen((uv_stream_t*) &Print_svr,128,Print_on_connection);
	if (r) {
		printf("Listen Error!!!\n");
		return 1;
	}
	printf("Accepting...\n");

	uv_run(loop, UV_RUN_DEFAULT);
	
	printf("Exiting...\n");
	free_array(webppd_cfg,CFG_MAX);	
	uv_close((uv_handle_t *) &Print_svr,NULL);
	uv_close((uv_handle_t *) &signal1,NULL);
	uv_loop_close(loop);



	return 0;
}
