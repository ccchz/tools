#ifndef ARRAY_SIZE
# define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

#define http_default  "HTTP/1.1 200 OK\r\n"	\
    "Content-Type:text/html\r\n"	\
    "Connection: close\r\n"	\
    "Content-Length:808\r\n"	\
    "\r\n"	\
    "<html><head><title>网页POS打印</title>"	\
    "<style>tr.header {font-weight:bold;} .dotted {border-top-width: 2px;border-top-style: dotted;border-top-color: rgb(  9,193,255);}"	\
    " .globle-container {width: 600px;margin: 0 auto;padding-top: 5px; margin-top:  0px;margin-right: auto;margin-bottom: 0px;margin-left: auto;}</style>"	\
    "</head><body><h1 align = 'center'>这是网页POS打印转发工具。</h1>"	\
    "<div class='globle-container dotted'><table align = 'center' width =600 border=1 cellspacing='0px' cellpadding='3px'>"	\
    "<tr class='header'><td align = 'center'>接口</td><td align = 'center'>说明</td></tr><tr>"	\
    "<td>/print</td><td>打印访问接口，POST发送，GBK，UTF-8编码。成功返回ok，返回其它数据失败。</td></tr><tr>"	\
    "<td>/readcard</td><td>读卡器访问接口，GET发送，返回JSON格式数据，返回其它数据失败。</td></tr></table></div>"	\
    "</body></html>"

#define http_ok  "HTTP/1.1 200 OK\r\n"	\
    "Content-Type:text/html\r\n"	\
    "Access-Control-Allow-Origin:*\r\n"	\
    "Connection: close\r\n"	\
    "Content-Length:2\r\n"	\
    "\r\n"	\
    "ok"

#define http_readcard  "HTTP/1.1 200 OK\r\n"	\
    "Content-Type:text/json\r\n"	\
    "Connection: close\r\n"	\
    "Content-Length: %I64u\r\n"	\
    "\r\n"

#define http_data  "HTTP/1.1 200 OK\r\n"	\
    "Content-Type:%s\r\n"	\
    "Connection: close\r\n"	\
    "Content-Length: %I64u\r\n"	\
    "\r\n"


#define http_400  "HTTP/1.1 400 OK\r\n"	\
    "Content-Type:text/html\r\n"	\
    "Connection: close\r\n"	\
    "Content-Length:549\r\n"	\
    "\r\n"	\
    "<html><head><title>400 POS网页打印</title></head>"	\
    "<body bgcolor='white'><center><h1>400 参数错误！！</h1></center>"	\
    "<hr><center>POS网页打印</center></body></html>"	\
    "<!-- a padding to disable MSIE and Chrome friendly error page -->"	\
    "<!-- a padding to disable MSIE and Chrome friendly error page -->"	\
    "<!-- a padding to disable MSIE and Chrome friendly error page -->"	\
    "<!-- a padding to disable MSIE and Chrome friendly error page -->"	\
    "<!-- a padding to disable MSIE and Chrome friendly error page -->"	\
    "<!-- a padding to disable MSIE and Chrome friendly error page -->"

#define http_500  "HTTP/1.1 500 OK\r\n"	\
    "Content-Type:text/html\r\n"	\
    "Connection: close\r\n"	\
    "Content-Length:549\r\n"	\
    "\r\n"	\
    "<html><head><title>500 POS网页打印</title></head>"	\
    "<body bgcolor='white'><center><h1>500 内部错误！！</h1></center>"	\
    "<hr><center>POS网页打印</center></body></html>"	\
    "<!-- a padding to disable MSIE and Chrome friendly error page -->"	\
    "<!-- a padding to disable MSIE and Chrome friendly error page -->"	\
    "<!-- a padding to disable MSIE and Chrome friendly error page -->"	\
    "<!-- a padding to disable MSIE and Chrome friendly error page -->"	\
    "<!-- a padding to disable MSIE and Chrome friendly error page -->"	\
    "<!-- a padding to disable MSIE and Chrome friendly error page -->"


#define HEADER_END 1
#define MSG_END	1
#define CFG_MAX 5
char *resp_str[] = {	0,	//not used.
			http_default,
			http_ok,
			http_readcard,	
			http_400,
			http_500,
			0	//处理过程已经返回数据，默认不需再发送数据。
};

#define HTTP_FIELD_STR(XX)	\
	XX("SCHEMA")		\
	XX("HOST")		\
	XX("PORT")		\
	XX("PATH")		\
	XX("QUERY")		\
	XX("FRAGMENT")		\
	XX("USERINFO")		\
	XX("MAX")

static const char * url_field_str[] =
	{
#define ZZ(string) string,
	HTTP_FIELD_STR(ZZ)
#undef ZZ
};



char *str_cfg[] = {	"ipaddr",
			"ipport",
			"printtype",
			"printer",
			"reader"
};

typedef struct str_pair_t {
		struct str_pair_t * next;
		char * key;
		char * value;
} str_pair_t;

typedef struct req_http_t {
		struct str_pair_t *root;
		struct str_pair_t *Current;
		struct http_parser_url parser_url;
		char * url_field[UF_MAX];
		char * method;
		char * url;
		char * body;
		unsigned int flags;
		unsigned int major;
		unsigned int minor;
		unsigned int state;
		unsigned int msgend;
		HANDLE hfile;
		iconv_t cc;
		char incompchar[8];
} req_http_t;

