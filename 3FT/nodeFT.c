/*--------------------------------------------------------------------*/
/* nodeDT.c                                                           */
/* Author: Ishaan Javali, Jack Zhang                                  */
/*--------------------------------------------------------------------*/

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "path.h"
#include "dynarray.h"
#include "nodeFT.h"


/* A node in a FT */
struct node {
   /* the object corresponding to the node's absolute path */
   Path_T oPPath;
   /* this node's parent */
   Node_T oNParent;
   /* the object containing links to this node's children that are files */
   DynArray_T fDChildren;
   /* the object containing links to this node's children that are directories */
   DynArray_T dDChildren;
   /* the value associated with a node if it is a file*/
   void* value;
   /* a boolean indicating if a node is a file or not*/
   boolean* isFile; 
   /* size of file contents in bytes*/
   size_t ulLength;
};


/*
  Links new child oNChild into oNParent's children array at index
  ulIndex. Returns SUCCESS if the new child was added successfully,
  or  MEMORY_ERROR if allocation fails adding oNChild to the array.
*/
static int Node_addChild(Node_T oNParent, Node_T oNChild,
                         boolean isFile, size_t ulIndex) {
   assert(oNParent != NULL);
   assert(oNChild != NULL);
   if(isFile){
      if(DynArray_addAt(oNParent->fDChildren, ulIndex, oNChild))
         return SUCCESS;
      else
         return MEMORY_ERROR;
   }else{
      if(DynArray_addAt(oNParent->dDChildren, ulIndex, oNChild))
         return SUCCESS;
      else
         return MEMORY_ERROR;
   }
   
}

/*
  Compares the string representation of oNfirst with a string
  pcSecond representing a node's path.
  Returns <0, 0, or >0 if oNFirst is "less than", "equal to", or
  "greater than" pcSecond, respectively.
*/
static int Node_compareString(const Node_T oNFirst,
                                 const char *pcSecond) {
   assert(oNFirst != NULL);
   assert(pcSecond != NULL);

   return Path_compareString(oNFirst->oPPath, pcSecond);
}

int Node_new(Path_T oPPath, Node_T oNParent, Node_T *poNResult, boolean isFile, void* value, size_t contentLength) {
   struct node *psNew;
   Path_T oPParentPath = NULL;
   Path_T oPNewPath = NULL;
   size_t ulParentDepth;
   size_t ulIndex;
   int iStatus;

   assert(oPPath != NULL);

   /* allocate space for a new node */
   psNew = malloc(sizeof(struct node));
   if(psNew == NULL) {
      *poNResult = NULL;
      return MEMORY_ERROR;
   }

   /* set the new node's path */
   iStatus = Path_dup(oPPath, &oPNewPath);
   if(iStatus != SUCCESS) {
      free(psNew);
      if(oPNewPath != NULL)
         Path_free(oPNewPath);
      *poNResult = NULL;
      return iStatus;
   }
   psNew->oPPath = oPNewPath;

   /* validate and set the new node's parent */
   if(oNParent != NULL) {
      size_t ulSharedDepth;

      oPParentPath = oNParent->oPPath;
      ulParentDepth = Path_getDepth(oPParentPath);
      ulSharedDepth = Path_getSharedPrefixDepth(psNew->oPPath,
                                                oPParentPath);
      /* parent must be an ancestor of child */
      if(ulSharedDepth < ulParentDepth) {
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return CONFLICTING_PATH;
      }

      /* parent must be exactly one level up from child */
      if(Path_getDepth(psNew->oPPath) != ulParentDepth + 1) {
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return NO_SUCH_PATH;
      }

      /* parent must not already have child with this path */
      if(Node_hasChild(oNParent, oPPath, isFile, &ulIndex)) {
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return ALREADY_IN_TREE;
      }
   }
   else {
      /* new node must be root */
      /* can only create one "level" at a time */
      if(Path_getDepth(psNew->oPPath) != 1) {
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return NO_SUCH_PATH;
      }
   }
   psNew->oNParent = oNParent;

   /* initialize the new node */
   psNew->fDChildren = DynArray_new(0);
   if(psNew->fDChildren == NULL) {
      Path_free(psNew->oPPath);
      free(psNew);
      *poNResult = NULL;
      return MEMORY_ERROR;
   }
   /* initialize the new node */
   psNew->dDChildren = DynArray_new(0);
   if(psNew->dDChildren == NULL) {
      DynArray_free(psNew->fDChildren);
      Path_free(psNew->oPPath);
      free(psNew);
      *poNResult = NULL;
      return MEMORY_ERROR;
   }

   /* Link into parent's children list */
   if(oNParent != NULL) {
      iStatus = Node_addChild(oNParent, psNew, isFile, ulIndex);
      if(iStatus != SUCCESS) {
         Path_free(psNew->oPPath);
         DynArray_free(psNew->fDChildren);
         DynArray_free(psNew->dDChildren);
         free(psNew);
         *poNResult = NULL;
         return iStatus;
      }
   }
    
    /* If node is a file, set value equal to parameter value, 
    and set file content length equal to the parameter contentLength,
    otherwise set value equal to NULL and ulLength to 0.*/
    psNew->isFile = malloc(sizeof(boolean));
    *(psNew->isFile) = isFile;
    if(isFile)
    {
        psNew->ulLength = contentLength;
        psNew->value = value;
    }
    else
    {
        psNew->ulLength = 0;
        psNew->value = NULL;
    }
   *poNResult = psNew;

   if(oNParent != NULL){
      for(ulIndex = 0; ulIndex < DynArray_getLength(oNParent->fDChildren); ulIndex++){
         Node_T temp;
         Node_getChild(oNParent, ulIndex, TRUE, &temp);
      } 
   }
   return SUCCESS;
}

boolean Node_isFile(Node_T oNNode){
   assert(oNNode != NULL);
   return *(oNNode->isFile);
}
size_t Node_getUlLength(Node_T oNNode){
   assert(oNNode != NULL);
   return oNNode->ulLength;
}
void Node_setUlLength(Node_T oNNode, size_t ulLength){
   assert(oNNode != NULL);
   oNNode->ulLength = ulLength;
}
void* Node_getValue(Node_T oNNode){
   assert(oNNode != NULL);
   return oNNode->value;
}
void Node_setValue(Node_T oNNode, void* value){
   assert(oNNode != NULL);
   oNNode->value = value;
}


size_t Node_free(Node_T oNNode) {
   size_t ulIndex;
   size_t ulCount = 0;

   assert(oNNode != NULL);
   /* remove this file from parent's list */
   if(*(oNNode->isFile)){
      if(oNNode->oNParent != NULL) {
         if(DynArray_bsearch(
               oNNode->oNParent->fDChildren,
               oNNode, &ulIndex,
               (int (*)(const void *, const void *)) Node_compare)
         ){
            (void) DynArray_removeAt(oNNode->oNParent->fDChildren,
                                 ulIndex);
         }
      }
   }else{
      /* remove this directory from parent's list */
      if(oNNode->oNParent != NULL) {
         if(DynArray_bsearch(
               oNNode->oNParent->dDChildren,
               oNNode, &ulIndex,
               (int (*)(const void *, const void *)) Node_compare)
         )
            (void) DynArray_removeAt(oNNode->oNParent->dDChildren,
                                 ulIndex);
      }
      /* recursively remove directory children */
      while(DynArray_getLength(oNNode->fDChildren) != 0) {
         ulCount += Node_free(DynArray_removeAt(oNNode->fDChildren, 0));
      }
      /* recursively remove directory children */
      while(DynArray_getLength(oNNode->dDChildren) != 0) {
         ulCount += Node_free(DynArray_removeAt(oNNode->dDChildren, 0));
      }
   }
   
   DynArray_free(oNNode->fDChildren);
   DynArray_free(oNNode->dDChildren);

   /* remove path */
   Path_free(oNNode->oPPath);

   /* finally, free the struct node */
   free(oNNode->isFile);
   free(oNNode);
   ulCount++;
   return ulCount;
}

Path_T Node_getPath(Node_T oNNode) {
   assert(oNNode != NULL);

   return oNNode->oPPath;
}

boolean Node_hasChild(Node_T oNParent, Path_T oPPath, boolean isFile,
                         size_t *pulChildID) {
   assert(oNParent != NULL);
   assert(oPPath != NULL);
   assert(pulChildID != NULL);

   /* *pulChildID is the index into oNParent->oDChildren */
   if(isFile){
      return DynArray_bsearch(oNParent->fDChildren,
               (char*) Path_getPathname(oPPath), pulChildID,
               (int (*)(const void*,const void*)) Node_compareString);
   }
   return DynArray_bsearch(oNParent->dDChildren,
               (char*) Path_getPathname(oPPath), pulChildID,
               (int (*)(const void*,const void*)) Node_compareString);
}

void Node_setFile(Node_T oNNode, boolean value);

size_t Node_getNumFileChildren(Node_T oNParent) {
   assert(oNParent != NULL);

   return DynArray_getLength(oNParent->fDChildren);
}
size_t Node_getNumDirChildren(Node_T oNParent) {
   assert(oNParent != NULL);

   return DynArray_getLength(oNParent->dDChildren);
}

int Node_getChild(Node_T oNParent, size_t ulChildID, boolean isFile,
                   Node_T *poNResult) {

   assert(oNParent != NULL);
   assert(poNResult != NULL);
   if(isFile){
      /* ulChildID is the index into oNParent->oDChildren */
      if(ulChildID >= Node_getNumFileChildren(oNParent)) {
         *poNResult = NULL;
         return NO_SUCH_PATH;
      }
      else {
         *poNResult = DynArray_get(oNParent->fDChildren, ulChildID);
         return SUCCESS;
      }
   }else{
      /* ulChildID is the index into oNParent->oDChildren */
      if(ulChildID >= Node_getNumDirChildren(oNParent)) {
         *poNResult = NULL;
         return NO_SUCH_PATH;
      }
      else {
         *poNResult = DynArray_get(oNParent->dDChildren, ulChildID);
         return SUCCESS;
      }
   }
   return NO_SUCH_PATH;
}

Node_T Node_getParent(Node_T oNNode) {
   assert(oNNode != NULL);

   return oNNode->oNParent;
}

int Node_compare(Node_T oNFirst, Node_T oNSecond) {
   assert(oNFirst != NULL);
   assert(oNSecond != NULL);

   return Path_comparePath(oNFirst->oPPath, oNSecond->oPPath);
}

char *Node_toString(Node_T oNNode) {
   char *copyPath;

   assert(oNNode != NULL);

   copyPath = malloc(Path_getStrLength(Node_getPath(oNNode))+1);
   if(copyPath == NULL)
      return NULL;
   else
      return strcpy(copyPath, Path_getPathname(Node_getPath(oNNode)));
}