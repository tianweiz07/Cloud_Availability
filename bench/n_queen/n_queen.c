/***********************************************************
* You can use all the programs on  www.c-program-example.com
* for personal and learning purposes. For permissions to use the
* programs for commercial purposes,
* contact info@c-program-example.com
* To find more C programs, do visit www.c-program-example.com
* and browse!
* 
*                      Happy Coding
***********************************************************/
 
#include<stdio.h>
//#include<conio.h>
#include<math.h>

int a[30],count=0;
int place(int pos)
{
 int i;
 for(i=1;i<pos;i++)
 {
  if((a[i]==a[pos])||((abs(a[i]-a[pos])==abs(i-pos))))
   return 0;
 }
 return 1;
}
void print_sol(int n)
{
 int i,j;
 count++;
// printf("\n\nSolution #%d:\n",count);
/*
 for(i=1;i<=n;i++)
 {
  for(j=1;j<=n;j++)
  {
   if(a[i]==j)
    printf("Q\t");
   else
    printf("*\t");
  }
  printf("\n");
 }
*/
}
void queen(int n)
{
 int k=1;
 a[k]=0;
 while(k!=0)
 {
  a[k]=a[k]+1;
  while((a[k]<=n)&&!place(k))
   a[k]++;
  if(a[k]<=n)
  {
   if(k==n)
    print_sol(n);
   else
   {
    k++;
    a[k]=0;
   }
  }
  else
   k--;
 }
  printf("There are %d solutions\n", count);
}
void main(int argc, char *argv[])
{
 int i,n;
 n = atoi(argv[1]);
 queen(n);
 printf("\nTotal solutions=%d",count);
}
