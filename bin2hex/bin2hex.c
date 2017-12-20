#include<stdio.h>
#include<fcntl.h>
#include<io.h>
#include <stdlib.h>

#define COMMENT(format,...)  printf("/*\r\n");		\
			     printf(format,##__VA_ARGS__);	\
	      		     printf("*/\r\n");
int useage()
{
	printf("This tool covert binary data to 0xXX format for programer.\n");
	printf("Useage:\n\t bin2c arrayname [filename] >> outfile\n");
}

int main(int argc, char * argv[])
{
  const int CLINE = 16;
  FILE *pin = stdin;
  char *array_name = NULL;
  char *p_name = NULL;
  int c = 0, lastc=EOF;
  int ccount = 0;
  int i = 0;

  p_name = argv[0];
  argv++;argc--; //no need program name
  if (argc ) {
    array_name = argv[0];
    argv++;argc--;
  } else {
	useage();
    fprintf(stderr, "Array Name Must Be Supplied.\n", argv[1]);
    exit(1);
  }
  if (argc) {
    pin = fopen(argv[0], "rb");
    if (pin == NULL) {
      fprintf(stderr, "Error Open File %s.\n", argv[1]);
      exit(1);
    }
  } else {
    _setmode(_fileno(stdin), O_BINARY);
  }

  COMMENT("Start the block of %s array by %s\r\n",array_name,__FILE__);

  printf("unsigned char %s[] = {", array_name);
  
  for(c = fgetc(pin); c!=EOF; c = fgetc(pin)) {
    if (lastc!=EOF){
      printf("0x%02X,", lastc);
      i++;
    }
    lastc = c;
    if (ccount++ % CLINE == 0)
      printf("\r\n");
  }
  printf("0x%02X", lastc);  i++;
  printf("\r\n};\r\n");
  printf("\r\nCONST uint64_t %s_lens = %d;\r\n\r\n",array_name,i);

  COMMENT("%s array Totle bytes: %d \r\nEnd the block of %s.\r\n",array_name,i,array_name);

}
