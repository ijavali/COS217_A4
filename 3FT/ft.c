
/*--------------------------------------------------------------------*/
/* ft.c                                                               */
/* Author: Ishaan Javali & Jack Zhang                                 */
/*--------------------------------------------------------------------*/

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "path.h"
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

/*ALL of this is copied from dtGood.c*/

/*
  Alternate version of strlen that uses pulAcc as an in-out parameter
  to accumulate a string length, rather than returning the length of
  oNNode's path, and also always adds one addition byte to the sum.
*/
static void FT_strlenAccumulate(Node_T oNNode, size_t *pulAcc) {
   assert(pulAcc != NULL);

   if(oNNode != NULL)
      *pulAcc += (Path_getStrLength(Node_getPath(oNNode)) + 1);
}


/*
  Alternate version of strcat that inverts the typical argument
  order, appending oNNode's path onto pcAcc, and also always adds one
  newline at the end of the concatenated string.
*/
static void FT_strcatAccumulate(Node_T oNNode, char *pcAcc) {
   assert(pcAcc != NULL);

   if(oNNode != NULL) {
      strcat(pcAcc, Path_getPathname(Node_getPath(oNNode)));
      strcat(pcAcc, "\n");
   }
}
/*
  Frees pcStr. This wrapper is used to match the requirements of the
  callback function pointer passed to DynArray_map. pvExtra is unused.
*/
static void Path_freeString(char *pcStr, void *pvExtra) {
   /* pcStr may be NULL, as this is a no-op to free.
      pvExtra may be NULL, as it is unused. */
   free(pcStr);
}

/*
  Traverses the FT starting at the root as far as possible towards
  absolute path oPPath. If able to traverse, returns an int SUCCESS
  status and sets *poNFurthest to the furthest node reached (which may
  be only a prefix of oPPath, or even NULL if the root is NULL).
  Otherwise, sets *poNFurthest to NULL and returns with status:
  * CONFLICTING_PATH if the root's path is not a prefix of oPPath
  * MEMORY_ERROR if memory could not be allocated to complete request
*/
static int FT_traversePath(Path_T oPPath, boolean isFile, Node_T *poNFurthest) {
   int iStatus;
   Path_T oPPrefix = NULL;
   Node_T oNCurr;
   Node_T oNChild = NULL;
   size_t ulDepth;
   size_t i;
   size_t ulChildID;

   assert(oPPath != NULL);
   assert(poNFurthest != NULL);

   /* root is NULL -> won't find anything */
   if(oNRoot == NULL) {
      *poNFurthest = NULL;
      return SUCCESS;
   }

   iStatus = Path_prefix(oPPath, 1, &oPPrefix);
   if(iStatus != SUCCESS) {
      *poNFurthest = NULL;
      return iStatus;
   }

   if(Path_comparePath(Node_getPath(oNRoot), oPPrefix)) {
      Path_free(oPPrefix);
      *poNFurthest = NULL;
      return CONFLICTING_PATH;
   }
   Path_free(oPPrefix);
   oPPrefix = NULL;

   oNCurr = oNRoot;
   ulDepth = Path_getDepth(oPPath);
   boolean origIsFile = isFile;
   for(i = 2; i <= ulDepth; i++) {
      if(origIsFile)
         isFile = (i == ulDepth);
      iStatus = Path_prefix(oPPath, i, &oPPrefix);
      if(iStatus != SUCCESS) {
         *poNFurthest = NULL;
         return iStatus;
      }
      if(Node_hasChild(oNCurr, oPPrefix, isFile, &ulChildID)) {
         /* go to that child and continue with next prefix */
         Path_free(oPPrefix);
         oPPrefix = NULL;
         iStatus = Node_getChild(oNCurr, ulChildID, isFile, &oNChild);
         if(iStatus != SUCCESS) {
            *poNFurthest = NULL;
            return iStatus;
         }
         oNCurr = oNChild;
      }
      else {
         /* oNCurr doesn't have child with path oPPrefix:
            this is as far as we can go */
            /* If the original thing we were searching for is a file, see if we can go here. */
            /* Added this to fix "Attempting to insert a child of a file is illegal" (line 130ish)
            Reasoning: Imagine this. path we're traversing is a/b/c/d/e/file.
               what exists: a/b/c (file)
               originally, it would go to dir a, then dir b, dir c (nope)
               so it stops at a/b.
               we could've gone a/b/c (file) and then realized its impossible to insert anything else.

             */
            if(origIsFile){
               isFile = TRUE;
               iStatus = Path_prefix(oPPath, i, &oPPrefix);
               if(iStatus != SUCCESS) {
                  *poNFurthest = NULL;
                  return iStatus;
               }
               if(Node_hasChild(oNCurr, oPPrefix, isFile, &ulChildID)) {
                  /* go to that child and continue with next prefix */
                  Path_free(oPPrefix);
                  oPPrefix = NULL;
                  iStatus = Node_getChild(oNCurr, ulChildID, isFile, &oNChild);
                  if(iStatus != SUCCESS) {
                     *poNFurthest = NULL;
                     return iStatus;
                  }
                  oNCurr = oNChild;
                  /* If the only path forward is to enter
                  into a file prematurely (in the path variable we were passed 
                  for traversing there are still more folders to go through)
                  then that means we can't traverse the rest of that path.
                  that's an error. this is not a directory
                   */
                  if(i != ulDepth) {
                     Path_free(oPPrefix);
                     *poNFurthest = oNCurr;
                     return NOT_A_DIRECTORY;
                  }
               }
            }
            break;
      }
   }

   Path_free(oPPrefix);
   *poNFurthest = oNCurr;
   Path_getPathname(Node_getPath(*poNFurthest));
   return SUCCESS;
}
/*
  Traverses the FT to find a node with absolute path pcPath. Returns a
  int SUCCESS status and sets *poNResult to be the node, if found.
  Otherwise, sets *poNResult to NULL and returns with status:
  * INITIALIZATION_ERROR if the FT is not in an initialized state
  * BAD_PATH if pcPath does not represent a well-formatted path
  * CONFLICTING_PATH if the root's path is not a prefix of pcPath
  * NO_SUCH_PATH if no node with pcPath exists in the hierarchy
  * MEMORY_ERROR if memory could not be allocated to complete request
 */

static int FT_findNode(const char *pcPath, Node_T *poNResult, boolean isFile) {
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
   iStatus = FT_traversePath(oPPath, isFile, &oNFound);
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

   if(isFile && !(Node_isFile(oNFound))){
      Path_free(oPPath);
      *poNResult = NULL;
      return NOT_A_FILE;
   }

   if(!isFile && Node_isFile(oNFound)) {
      Path_free(oPPath);
      *poNResult = NULL;
      return NOT_A_DIRECTORY;
   }

   Path_free(oPPath);
   *poNResult = oNFound;
   return SUCCESS;
}



/*
  Performs a pre-order traversal of the tree rooted at n,
  inserting each payload to DynArray_T d beginning at index i.
  Returns the next unused index in d after the insertion(s).
*/
static size_t FT_preOrderTraversal(Node_T n, DynArray_T d, size_t i) {
   size_t c;

   assert(d != NULL);

   if(n != NULL) {
      (void) DynArray_set(d, i, n);
      i++;
      for(c = 0; c < Node_getNumFileChildren(n); c++) {
         int iStatus;
         Node_T oNChild = NULL;
         iStatus = Node_getChild(n, c, TRUE, &oNChild);
         assert(iStatus == SUCCESS);
         i = FT_preOrderTraversal(oNChild, d, i);
      }
      for(c = 0; c < Node_getNumDirChildren(n); c++) {
         int iStatus;
         Node_T oNChild = NULL;
         iStatus = Node_getChild(n, c, FALSE, &oNChild);
         assert(iStatus == SUCCESS);
         i = FT_preOrderTraversal(oNChild, d, i);
      }
   }
   return i;
}

char *FT_toString(void) {
   DynArray_T nodes;
   size_t totalStrlen = 1;
   char *result = NULL;

   if(!bIsInitialized)
      return NULL;

   nodes = DynArray_new(ulCount);
   (void) FT_preOrderTraversal(oNRoot, nodes, 0);

   DynArray_map(nodes, (void (*)(void *, void*)) FT_strlenAccumulate,
                (void*) &totalStrlen);

   result = malloc(totalStrlen);
   if(result == NULL) {
      DynArray_free(nodes);
      return NULL;
   }
   *result = '\0';

   DynArray_map(nodes, (void (*)(void *, void*)) FT_strcatAccumulate,
                (void *) result);

   DynArray_free(nodes);

   return result;
}


int FT_rmDir(const char *pcPath) {
   int iStatus;
   Node_T oNFound = NULL;

   assert(pcPath != NULL);
   if(FT_containsFile(pcPath)){
      return NOT_A_DIRECTORY;
   }
   iStatus = FT_findNode(pcPath, &oNFound, FALSE);
   /* printf("%d\n", iStatus);
   if(iStatus == NO_SUCH_PATH){
      if(oNFound != NULL)
      printf(" removing %s %d", Path_getPathname(Node_getPath(oNFound)),
         Node_isFile(oNFound));
      iStatus = FT_findNode(pcPath, &oNFound, TRUE);
      if(iStatus != SUCCESS)
         return iStatus;
      return NOT_A_DIRECTORY;
   } */

   if(iStatus != SUCCESS)
       return iStatus;
   if(Node_isFile(oNFound))
       return NOT_A_DIRECTORY;
   ulCount -= Node_free(oNFound);
   if(ulCount == 0)
      oNRoot = NULL;
   
   return SUCCESS;
}

int FT_rmFile(const char *pcPath) {
   int iStatus;
   Node_T oNFound = NULL;

   assert(pcPath != NULL);
   
   if(FT_containsDir(pcPath)){
      return NOT_A_FILE;
   }

   iStatus = FT_findNode(pcPath, &oNFound, TRUE);
   /* if(iStatus == NO_SUCH_PATH){
      if(oNFound != NULL)
      iStatus = FT_findNode(pcPath, &oNFound, FALSE);
      printf("NOS %d %d\n", iStatus, FT_containsDir(pcPath));
      if(iStatus != SUCCESS)
         return iStatus;
      return NOT_A_FILE;
   } */
   if(iStatus != SUCCESS)
       return iStatus;
   if(!(Node_isFile(oNFound)))
       return NOT_A_FILE;

   ulCount -= Node_free(oNFound);
   if(ulCount == 0)
      oNRoot = NULL;
 
   return SUCCESS;
}
int FT_init(void) {

    if (bIsInitialized) return INITIALIZATION_ERROR;

    bIsInitialized = TRUE;
    oNRoot = NULL;
    ulCount = 0;

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
    int iStatus;
    Node_T oNFound = NULL;
    DynArray_T oDSubstrings;
    const char *pcStart = pcPath;
    const char *pcEnd = pcPath;
    char *pcCopy;
    assert(pcPath != NULL);
    assert(pbIsFile != NULL);
    assert(pulSize != NULL);

    if (!bIsInitialized) {
        return INITIALIZATION_ERROR;
    }

    /* -------- */
    /* FIXME: Doubtful about BAD_PATH and MEMORY_ERROR checking. unfreed memory */
    /* Copied from path.c, Path_split() */
    if (*pcPath == '\0') {
        return BAD_PATH;
    }

    oDSubstrings = DynArray_new(0);
    if (oDSubstrings == NULL) {
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
            free(pcCopy);
            return MEMORY_ERROR;
        }

        while (pcStart != pcEnd) {
            *pcCopy = *pcStart;
            pcCopy++;
            pcStart++;
        }

        pcStart++;
    }

    /* Copied from dtGood.c, FT_traversePath */
    /* Path_prefix can return NO_SUCH_PATH and MEMORY_ERROR */
    /* TODO commented out below to fix error */
    /* iStatus = Path_prefix(pcPath, 1, &oPPrefix);
    if(iStatus != SUCCESS) {
        return iStatus;
    }
    if(Path_comparePath(Node_getPath(oNRoot), oPPrefix)) {
        Path_free(oPPrefix);
        return CONFLICTING_PATH;
    } */
    
    /* -------- */

    iStatus = FT_findNode(pcPath, &oNFound, *pbIsFile);
    if(iStatus != SUCCESS){
        iStatus = FT_findNode(pcPath, &oNFound, 1 - *pbIsFile);
         if(iStatus != SUCCESS){
            DynArray_map(oDSubstrings,
                         (void (*)(void *, void *))Path_freeString, NULL);
            DynArray_free(oDSubstrings);
            return iStatus;
         }
    }
    *pbIsFile = Node_isFile(oNFound);
    if(Node_isFile(oNFound)){
        *pulSize = Node_getUlLength(oNFound);
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
    iStatus = FT_findNode(pcPath, &oNFound, TRUE);
    if(iStatus != SUCCESS){
        return NULL;
    }
    if(Node_isFile(oNFound)){
        return Node_getValue(oNFound);
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
    iStatus = FT_findNode(pcPath, &oNFound, TRUE);
    if(iStatus != SUCCESS){
        return NULL;
    }
    
    if(Node_isFile(oNFound)){
        void* oldContent = Node_getValue(oNFound);
        Node_setValue(oNFound, pvNewContents);
        Node_setUlLength(oNFound, ulNewLength);
        return oldContent;
    }
    return NULL;
}


boolean FT_containsDir(const char *pcPath) {
   int iStatus;
   Node_T oNFound = NULL;

   assert(pcPath != NULL);

   iStatus = FT_findNode(pcPath, &oNFound, FALSE);
   return (boolean) (iStatus == SUCCESS);
}

boolean FT_containsFile(const char *pcPath) {
   int iStatus;
   Node_T oNFound = NULL;

   assert(pcPath != NULL);

   iStatus = FT_findNode(pcPath, &oNFound, TRUE);
   return (boolean) (iStatus == SUCCESS);
}

int FT_insertDir(const char *pcPath) {
   int iStatus;
   Path_T oPPath = NULL, zPPath = NULL;
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
   iStatus = FT_traversePath(oPPath, FALSE, &oNCurr);
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

   /* Originally */
   /* ulDepth = Path_getDepth(oPPath);
   if(oNCurr == NULL) 
      ulIndex = 1;
   else {
      ulIndex = Path_getDepth(Node_getPath(oNCurr))+1;

      if(ulIndex == ulDepth+1 && !Path_comparePath(oPPath,
                                       Node_getPath(oNCurr))) {
         Path_free(oPPath);
         return ALREADY_IN_TREE;
      }
   } */
   /* ================ */
   iStatus = Path_new(pcPath, &zPPath);
   if(iStatus != SUCCESS){
      Path_free(oPPath);
      Path_free(zPPath);
      return iStatus;
   }

   iStatus = FT_traversePath(zPPath, TRUE, &oNCurr);
   if(iStatus != SUCCESS) {
      Path_free(oPPath);
      Path_free(zPPath);
      return iStatus;
   }
   ulDepth = Path_getDepth(zPPath);
   if(oNCurr == NULL) /* new root! */
      ulIndex = 1;
   else {
      ulIndex = Path_getDepth(Node_getPath(oNCurr))+1;
      /* oNCurr is the node we're trying to insert */
      if(ulIndex == ulDepth+1 && !Path_comparePath(zPPath,
                                       Node_getPath(oNCurr))) {
         Path_free(oPPath);
         Path_free(zPPath);
         return ALREADY_IN_TREE;
      }
   }
   iStatus = FT_traversePath(oPPath, FALSE, &oNCurr);
   if(iStatus != SUCCESS) {
      Path_free(oPPath);
      Path_free(zPPath);
      return iStatus;
   }
   ulDepth = Path_getDepth(oPPath);
   if(oNCurr == NULL) /* new root! */
      ulIndex = 1;
   else {
      ulIndex = Path_getDepth(Node_getPath(oNCurr))+1;
      if(ulIndex == ulDepth+1 && !Path_comparePath(oPPath,
                                       Node_getPath(oNCurr))) {
         Path_free(oPPath);
         Path_free(zPPath);
         return ALREADY_IN_TREE;
      }
   }
   
   /* ================ */

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
      iStatus = Node_new(oPPrefix, oNCurr, &oNNewNode, FALSE, NULL, 0);
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
   /* update FT state variables to reflect insertion */
   if(oNRoot == NULL)
      oNRoot = oNFirstNew;
   ulCount += ulNewNodes;

   return SUCCESS;
}

int FT_insertFile(const char *pcPath, void *pvContents, size_t ulLength) {
   int iStatus;
   Path_T oPPath = NULL, zPPath = NULL;
   Node_T oNFirstNew = NULL;
   Node_T oNCurr = NULL;
   size_t ulDepth, ulIndex;
   size_t ulNewNodes = 0;

   assert(pcPath != NULL);

   /* validate pcPath and generate a Path_T for it */
   if(!bIsInitialized)
      return INITIALIZATION_ERROR;

   iStatus = Path_new(pcPath, &oPPath);
   if(iStatus != SUCCESS){
      Path_free(oPPath);
      return iStatus;
   }

   /* find the closest ancestor of oPPath already in the tree */
   iStatus = FT_traversePath(oPPath, TRUE, &oNCurr);
   if(iStatus != SUCCESS)
   {
      Path_free(oPPath);
      return iStatus;
   }
   /* New modification. I added check for depth.
    because it should be valid to add "1child/2dir/3file"
    to an empty tree. however, adding file "3file" to the root is wrong. 
    Significance: now you can do insertfile("1root/2child/3gkid" as the very first insert and it works)
    */
   /*oNCurr is at the root*/
   if(oNCurr == NULL && ulCount == 0 && Path_getDepth(oPPath) == 1)
   {
      Path_free(oPPath);
      return CONFLICTING_PATH;
   }
   /* no ancestor node found, so if root is not NULL,
      pcPath isn't underneath root. */
   if(oNCurr == NULL && oNRoot != NULL) {
      Path_free(oPPath);
      return CONFLICTING_PATH;
   }

/* DONE: You have to copy this over to insertDir */
/* Commented out below and moved it below */
   /* ulDepth = Path_getDepth(oPPath); */
   iStatus = Path_new(pcPath, &zPPath);
   if(iStatus != SUCCESS){
      Path_free(oPPath);
      Path_free(zPPath);
      return iStatus;
   }

   /* find the closest ancestor of oPPath already in the tree */
   iStatus = FT_traversePath(zPPath, FALSE, &oNCurr);
   if(iStatus != SUCCESS) {
      Path_free(oPPath);
      Path_free(zPPath);
      return iStatus;
   }
   ulDepth = Path_getDepth(zPPath);
   if(oNCurr == NULL) /* new root! */
      ulIndex = 1;
   else {
      ulIndex = Path_getDepth(Node_getPath(oNCurr))+1;
      /* oNCurr is the node we're trying to insert */
      if(ulIndex == ulDepth+1 && !Path_comparePath(zPPath,
                                       Node_getPath(oNCurr))) {
         Path_free(oPPath);
         Path_free(zPPath);
         return ALREADY_IN_TREE;
      }
   }
   iStatus = FT_traversePath(oPPath, TRUE, &oNCurr);
   if(iStatus != SUCCESS) {
      Path_free(oPPath);
      Path_free(zPPath);
      return iStatus;
   }
   ulDepth = Path_getDepth(oPPath);
   if(oNCurr == NULL) /* new root! */
      ulIndex = 1;
   else {
      ulIndex = Path_getDepth(Node_getPath(oNCurr))+1;
      if(ulIndex == ulDepth+1 && !Path_comparePath(oPPath,
                                       Node_getPath(oNCurr))) {
         Path_free(oPPath);
         Path_free(zPPath);
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
         Path_free(zPPath);
         if(oNFirstNew != NULL)
            (void) Node_free(oNFirstNew);
         return iStatus;
      }

      /* insert the new node for this level */
      if(ulIndex < ulDepth) /* We have to insert the directories first */
         iStatus = Node_new(oPPrefix, oNCurr, &oNNewNode, FALSE, pvContents, ulLength);
      else
         iStatus = Node_new(oPPrefix, oNCurr, &oNNewNode, TRUE, pvContents, ulLength);
      if(iStatus != SUCCESS) {
         Path_free(oPPath);
         Path_free(zPPath);
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
   Path_free(zPPath);
   Path_free(oPPath);
   /* update FT state variables to reflect insertion */
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