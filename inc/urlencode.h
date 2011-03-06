//From http://cboard.cprogramming.com/c-programming/92632-frustration-url-encode-decode.html

#include <string>

string UrlEncode(string text)
{ // encoding function
     while((c = getchar()) != EOF ){
      if( 'a' <= c && c <= 'z'
      || 'A' <= c && c <= 'Z'
      || '0' <= c && c <= '9'
      || c == '-' || c == '_' || c == '.' )
         putchar(c);
      else if( c == ' ' )
         putchar('+');
      else {
         putchar('%');
         putchar(h[c >> 4]);
         putchar(h[c & 0x0f]);
      }
    }
}

void UrlDecode(void)
{ // decode function
   char c, c1, c2;
   printf("enter your sentence\n");
   while( (c = getchar()) != EOF ){
      if( c == '%' ){
         c1 = getchar();
         c2 = getchar();
         if( c1 == EOF || c2 == EOF )  exit(0);
         c1 = tolower(c1);
         c2 = tolower(c2);
         if( ! isxdigit(c1) || ! isxdigit(c2) )  exit(0);
         if( c1 <= '9' )
            c1 = c1 - '0';
         else
            c1 = c1 - 'a' + 10;
         if( c2 <= '9' )
            c2 = c2 - '0';
         else
            c2 = c2 - 'a' + 10;
         putchar( 16 * c1 + c2 );
      } else if( c == '+' )
         putchar(' ');
      else
         putchar(c);
   }
} 
