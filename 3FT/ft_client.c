/*--------------------------------------------------------------------*/
/* ft_client.c                                                        */
/* Author: Christopher Moretti                                        */
/*--------------------------------------------------------------------*/

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ft.h"

/* Tests the FT implementation with an assortment of checks.
   Prints the status of the data structure along the way to stderr.
   Returns 0. */
int main(void) {
  enum {ARRLEN = 1000};
  char* temp;
  boolean bIsFile;
  size_t l;
  char arr[ARRLEN];
  arr[0] = '\0';

  /* Before the data structure is initialized:
     * insert*, rm*, and destroy should all return INITIALIZATION_ERROR
     * contains* should return FALSE
     * toString should return NULL.
  */
  assert(FT_insertDir("1root/2child/3gkid") == INITIALIZATION_ERROR);
  assert(FT_containsDir("1root/2child/3gkid") == FALSE);
  assert(FT_rmDir("1root/2child/3gkid") == INITIALIZATION_ERROR);
  assert(FT_insertFile("1root/2child/3gkid/4ggk",NULL,0) ==
         INITIALIZATION_ERROR);
  assert(FT_containsFile("1root/2child/3gkid/4ggk") == FALSE);
  assert(FT_rmFile("1root/2child/3gkid/4ggk") == INITIALIZATION_ERROR);
  assert((temp = FT_toString()) == NULL);
  assert(FT_destroy() == INITIALIZATION_ERROR);

  assert(FT_init() == SUCCESS);
  assert(FT_containsDir("1root/2child/3gkid") == FALSE);
  assert(FT_containsFile("1root/2child/3gkid/4ggk") == FALSE);
  assert((temp = FT_toString()) != NULL);
  assert(!strcmp(temp,""));
  free(temp);


 assert(FT_insertDir("") == BAD_PATH);
  assert(FT_insertDir("/1root/2child") == BAD_PATH);
  assert(FT_insertDir("1root/2child/") == BAD_PATH);
  assert(FT_insertDir("1root//2child") == BAD_PATH);
  assert(FT_insertFile("", NULL, 0) == BAD_PATH);
  assert(FT_insertFile("/1root/2child", NULL, 0) == BAD_PATH);
  assert(FT_insertFile("1root/2child/", NULL, 0) == BAD_PATH);
  assert(FT_insertFile("1root//2child", NULL, 0) == BAD_PATH);

  /* putting a file at the root is illegal */
  assert(FT_insertFile("A",NULL,0) == CONFLICTING_PATH);

assert(FT_insertDir("1root/2child/3gk2/4ggk") == SUCCESS);
assert(FT_stat("1root/H", &bIsFile, &l) == NO_SUCH_PATH);

  


  return 0;
}
