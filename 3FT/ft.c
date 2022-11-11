/*--------------------------------------------------------------------*/
/* ft.c                                                               */
/* Author: Ishaan Javali & Jack Zhang                                 */
/*--------------------------------------------------------------------*/

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "dynarray.h"
#include "ft.h"
#include "nodeFT.h"

/*
  A Directory Tree is a representation of a hierarchy of directories,
  represented as an AO with 3 state variables:
*/

/* 1. a flag for being in an initialized state (TRUE) or not (FALSE) */
static boolean bIsInitialized;
/* 2. a pointer to the root node in the hierarchy */
static Node_T oNRoot;
/* 3. a counter of the number of nodes in the hierarchy */
static size_t ulCount;

int FT_init(void) {
    assert(CheckerDT_isValid(bIsInitialized, oNRoot, ulCount));

    if (bIsInitialized) return INITIALIZATION_ERROR;

    bIsInitialized = TRUE;
    oNRoot = NULL;
    ulCount = 0;

    assert(CheckerDT_isValid(bIsInitialized, oNRoot, ulCount));
    return SUCCESS;
}

/*
  Returns SUCCESS if pcPath exists in the hierarchy,
  Otherwise, returns:
  * INITIALIZATION_ERROR if the FT is not in an initialized state
  * BAD_PATH if pcPath does not represent a well-formatted path
  * CONFLICTING_PATH if the root's path is not a prefix of pcPath
  * NO_SUCH_PATH if absolute path pcPath does not exist in the FT
  * MEMORY_ERROR if memory could not be allocated to complete request

  When returning SUCCESS,
  if path is a directory: sets *pbIsFile to FALSE, *pulSize unchanged
  if path is a file: sets *pbIsFile to TRUE, and
                     sets *pulSize to the length of file's contents

  When returning another status, *pbIsFile and *pulSize are unchanged.
*/

int FT_stat(const char *pcPath, boolean *pbIsFile, size_t *pulSize) {
    Path_T oPPrefix = NULL;
    int iStatus;
    Node_T oNFound = NULL;
    const char *pcStart = pcPath;
    const char *pcEnd = pcPath;
    assert(pcPath != NULL);
    assert(pbIsFile != NULL);
    assert(pulSize != NULL);

    if (!bIsInitialized) {
        return INITIALIZATION_ERROR
    }

    /* -------- */
    /* FIXME: Doubtful about BAD_PATH and MEMORY_ERROR checking. unfreed memory */
    /* Copied from path.c, Path_split() */
    if (*pcPath == '\0') {
        *poDComponents = NULL;
        return BAD_PATH;
    }

    oDSubstrings = DynArray_new(0);
    if (oDSubstrings == NULL) {
        *poDComponents = NULL;
        return MEMORY_ERROR;
    }

    /* validate and split pcPath */
    while (*pcEnd != '\0') {
        pcEnd = pcStart;
        /* component can't start with delimiter */
        if (*pcEnd == '/') {
            DynArray_map(oDSubstrings,
                         (void (*)(void *, void *))Path_freeString, NULL);
            DynArray_free(oDSubstrings);
            return BAD_PATH;
        }

        /* advance pcEnd to end of next token */
        while (*pcEnd != '/' && *pcEnd != '\0') pcEnd++;

        /* final component can't end with slash */
        if (*pcEnd == '\0' && *(pcEnd - 1) == '/') {
            DynArray_map(oDSubstrings,
                         (void (*)(void *, void *))Path_freeString, NULL);
            DynArray_free(oDSubstrings);
            return BAD_PATH;
        }

        pcCopy = calloc((size_t)(pcEnd - pcStart + 1), sizeof(char));
        if (pcCopy == NULL) {
            DynArray_map(oDSubstrings,
                         (void (*)(void *, void *))Path_freeString, NULL);
            DynArray_free(oDSubstrings);
            return MEMORY_ERROR;
        }

        if (DynArray_add(oDSubstrings, pcCopy) == 0) {
            DynArray_map(oDSubstrings,
                         (void (*)(void *, void *))Path_freeString, NULL);
            DynArray_free(oDSubstrings);
            return MEMORY_ERROR;
        }

        while (pcStart != pcEnd) {
            *pcCopy = *pcStart;
            pcCopy++;
            pcStart++;
        }

        pcStart++;
    }

    /* Copied from dtGood.c, DT_traversePath */
    /* Path_prefix can return NO_SUCH_PATH and MEMORY_ERROR */
    iStatus = Path_prefix(oPPath, 1, &oPPrefix);
    if(iStatus != SUCCESS) {
        return iStatus;
    }
    if(Path_comparePath(Node_getPath(oNRoot), oPPrefix)) {
        Path_free(oPPrefix);
        return CONFLICTING_PATH;
    }
    
    /* -------- */

    iStatus = FT_findNode(pcPath, &oNFound);
    if(iStatus != SUCCESS){
        return iStatus;
    }
    *pbIsFile = *(*oNFound->pbIsFile);
    if(*(*oNFound->pbIsFile)){
        *pulSize = *oNFound->ulLength;
    }

    return SUCCESS;
}

void *FT_getFileContents(const char *pcPath){
    int iStatus;
    Node_T oNFound = NULL;
    assert(pcPath != NULL);
    if(!FT_containsFile(pcPath)){
        return NULL;
    }
    iStatus = FT_findNode(pcPath, &oNFound);
    if(iStatus != SUCCESS){
        return NULL;
    }
    if(*(*oNFound->pbIsFile)){
        return *oNFound->value;
    }
    return NULL;
}

/*
  Replaces current contents of the file with absolute path pcPath with
  the parameter pvNewContents of size ulNewLength bytes.
  Returns the old contents if successful. (Note: contents may be NULL.)
  Returns NULL if unable to complete the request for any reason.
*/
void *FT_replaceFileContents(const char *pcPath, void *pvNewContents,
                             size_t ulNewLength) {
    int iStatus;
    Node_T oNFound = NULL;
    assert(pcPath != NULL);
    if(!FT_containsFile(pcPath)){
        return NULL;
    }
    iStatus = FT_findNode(pcPath, &oNFound);
    if(iStatus != SUCCESS){
        return NULL;
    }
    
    if(*(*oNFound->pbIsFile)){
        void* oldContent = *oNFound->value;
        *oNFound->value = pvNewContents;
        *oNFound->ulLength = ulNewLength;
        return oldContent;
    }
    return NULL;
}

/*
  Traverses the FT to find a node with absolute path pcPath. Returns a
  int SUCCESS status and sets *poNResult to be the node, if found.
  Otherwise, sets *poNResult to NULL and returns with status:
  * INITIALIZATION_ERROR if the DT is not in an initialized state
  * BAD_PATH if pcPath does not represent a well-formatted path
  * CONFLICTING_PATH if the root's path is not a prefix of pcPath
  * NO_SUCH_PATH if no node with pcPath exists in the hierarchy
  * MEMORY_ERROR if memory could not be allocated to complete request
 */
static int FT_findNode(const char *pcPath, Node_T *poNResult) {
   Path_T oPPath = NULL;
   Node_T oNFound = NULL;
   int iStatus;

   assert(pcPath != NULL);
   assert(poNResult != NULL);

   if(!bIsInitialized) {
      *poNResult = NULL;
      return INITIALIZATION_ERROR;
   }

   iStatus = Path_new(pcPath, &oPPath);
   if(iStatus != SUCCESS) {
      *poNResult = NULL;
      return iStatus;
   }

   iStatus = FT_traversePath(oPPath, &oNFound);
   if(iStatus != SUCCESS)
   {
      Path_free(oPPath);
      *poNResult = NULL;
      return iStatus;
   }

   if(oNFound == NULL) {
      Path_free(oPPath);
      *poNResult = NULL;
      return NO_SUCH_PATH;
   }

   if(Path_comparePath(Node_getPath(oNFound), oPPath) != 0) {
      Path_free(oPPath);
      *poNResult = NULL;
      return NO_SUCH_PATH;
   }

   Path_free(oPPath);
   *poNResult = oNFound;
   return SUCCESS;
}


int FT_insertDir(const char *pcPath) {
   int iStatus;
   Path_T oPPath = NULL;
   Node_T oNFirstNew = NULL;
   Node_T oNCurr = NULL;
   size_t ulDepth, ulIndex;
   size_t ulNewNodes = 0;

   assert(pcPath != NULL);

   /* validate pcPath and generate a Path_T for it */
   if(!bIsInitialized)
      return INITIALIZATION_ERROR;

   iStatus = Path_new(pcPath, &oPPath);
   if(iStatus != SUCCESS)
      return iStatus;

   /* find the closest ancestor of oPPath already in the tree */
   iStatus= FT_traversePath(oPPath, &oNCurr);
   if(iStatus != SUCCESS)
   {
      Path_free(oPPath);
      return iStatus;
   }

   /* no ancestor node found, so if root is not NULL,
      pcPath isn't underneath root. */
   if(oNCurr == NULL && oNRoot != NULL) {
      Path_free(oPPath);
      return CONFLICTING_PATH;
   }

   ulDepth = Path_getDepth(oPPath);
   if(oNCurr == NULL) /* new root! */
      ulIndex = 1;
   else {
      ulIndex = Path_getDepth(Node_getPath(oNCurr))+1;

      /* oNCurr is the node we're trying to insert */
      if(ulIndex == ulDepth+1 && !Path_comparePath(oPPath,
                                       Node_getPath(oNCurr))) {
         Path_free(oPPath);
         return ALREADY_IN_TREE;
      }
   }

   /* starting at oNCurr, build rest of the path one level at a time */
   while(ulIndex <= ulDepth) {
      Path_T oPPrefix = NULL;
      Node_T oNNewNode = NULL;

      /* generate a Path_T for this level */
      iStatus = Path_prefix(oPPath, ulIndex, &oPPrefix);
      if(iStatus != SUCCESS) {
         Path_free(oPPath);
         if(oNFirstNew != NULL)
            (void) Node_free(oNFirstNew);
         return iStatus;
      }

      /* insert the new node for this level */
      iStatus = Node_new(oPPrefix, oNCurr, &oNNewNode, TRUE, NULL, 0);
      if(iStatus != SUCCESS) {
         Path_free(oPPath);
         Path_free(oPPrefix);
         if(oNFirstNew != NULL)
            (void) Node_free(oNFirstNew);
         return iStatus;
      }

      /* set up for next level */
      Path_free(oPPrefix);
      oNCurr = oNNewNode;
      ulNewNodes++;
      if(oNFirstNew == NULL)
         oNFirstNew = oNCurr;
      ulIndex++;
   }

   Path_free(oPPath);
   /* update DT state variables to reflect insertion */
   if(oNRoot == NULL)
      oNRoot = oNFirstNew;
   ulCount += ulNewNodes;

   return SUCCESS;
}

int FT_insertFile(const char *pcPath, void *pvContents, size_t ulLength) {
   int iStatus;
   Path_T oPPath = NULL;
   Node_T oNFirstNew = NULL;
   Node_T oNCurr = NULL;
   size_t ulDepth, ulIndex;
   size_t ulNewNodes = 0;

   assert(pcPath != NULL);

   /* validate pcPath and generate a Path_T for it */
   if(!bIsInitialized)
      return INITIALIZATION_ERROR;

   iStatus = Path_new(pcPath, &oPPath);
   if(iStatus != SUCCESS)
      return iStatus;

   /* find the closest ancestor of oPPath already in the tree */
   iStatus= FT_traversePath(oPPath, &oNCurr);
   if(iStatus != SUCCESS)
   {
      Path_free(oPPath);
      return iStatus;
   }

   /* no ancestor node found, so if root is not NULL,
      pcPath isn't underneath root. */
   if(oNCurr == NULL && oNRoot != NULL) {
      Path_free(oPPath);
      return CONFLICTING_PATH;
   }

   ulDepth = Path_getDepth(oPPath);
   if(oNCurr == NULL) /* new root! */
      ulIndex = 1;
   else {
      ulIndex = Path_getDepth(Node_getPath(oNCurr))+1;

      /* oNCurr is the node we're trying to insert */
      if(ulIndex == ulDepth+1 && !Path_comparePath(oPPath,
                                       Node_getPath(oNCurr))) {
         Path_free(oPPath);
         return ALREADY_IN_TREE;
      }
   }

   /* starting at oNCurr, build rest of the path one level at a time */
   while(ulIndex <= ulDepth) {
      Path_T oPPrefix = NULL;
      Node_T oNNewNode = NULL;

      /* generate a Path_T for this level */
      iStatus = Path_prefix(oPPath, ulIndex, &oPPrefix);
      if(iStatus != SUCCESS) {
         Path_free(oPPath);
         if(oNFirstNew != NULL)
            (void) Node_free(oNFirstNew);
         return iStatus;
      }

      /* insert the new node for this level */
      iStatus = Node_new(oPPrefix, oNCurr, &oNNewNode, TRUE, pvContents, ulLength);
      if(iStatus != SUCCESS) {
         Path_free(oPPath);
         Path_free(oPPrefix);
         if(oNFirstNew != NULL)
            (void) Node_free(oNFirstNew);
         return iStatus;
      }

      /* set up for next level */
      Path_free(oPPrefix);
      oNCurr = oNNewNode;
      ulNewNodes++;
      if(oNFirstNew == NULL)
         oNFirstNew = oNCurr;
      ulIndex++;
   }

   Path_free(oPPath);
   /* update DT state variables to reflect insertion */
   if(oNRoot == NULL)
      oNRoot = oNFirstNew;
   ulCount += ulNewNodes;

   return SUCCESS;
}


int FT_destroy(void) {
   if(!bIsInitialized)
      return INITIALIZATION_ERROR;

   if(oNRoot) {
      ulCount -= Node_free(oNRoot);
      oNRoot = NULL;
   }

   bIsInitialized = FALSE;
   return SUCCESS;
}
