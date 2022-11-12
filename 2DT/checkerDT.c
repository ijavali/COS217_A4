/*--------------------------------------------------------------------*/
/* checkerDT.c                                                        */
/* Author:                                                            */
/*--------------------------------------------------------------------*/

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "checkerDT.h"
#include "dynarray.h"
#include "path.h"



/* see checkerDT.h for specification */
boolean CheckerDT_Node_isValid(Node_T oNNode) {
    Node_T oNParent;
    Path_T oPNPath;
    Path_T oPPPath;
    size_t ulIndex;


    /* Sample check: a NULL pointer is not a valid node */
    if (oNNode == NULL) {
        fprintf(stderr, "A node is a NULL pointer\n");
        return FALSE;
    }

    oNParent = Node_getParent(oNNode);
    oPNPath = Node_getPath(oNNode);
    if (oNParent != NULL) {
        oPPPath = Node_getPath(oNParent);

        /* Sample check: parent's path must be the longest possible
        proper prefix of the node's path */
        if (Path_getSharedPrefixDepth(oPNPath, oPPPath) !=
            Path_getDepth(oPNPath) - 1) {
            fprintf(stderr, "P-C nodes don't have P-C paths: (%s) (%s)\n",
                    Path_getPathname(oPPPath), Path_getPathname(oPNPath));
            return FALSE;
        }

        /* Sample check: parent must be an ancestor of child*/
        if (Path_getSharedPrefixDepth(oPNPath, oPPPath) <
            Path_getDepth(oPPPath)) {
            fprintf(stderr, "A parent node is not an ancestor of a child\n");
            return FALSE;
        }

        /* Sample check: parent must be exactly one level up from child */
        if (Path_getDepth(oPNPath) != Path_getDepth(oPPPath) + 1) {
            fprintf(stderr, "A node is not one level down from its parent\n");
            return FALSE;
         }
      
      /* parent must not already have child with this path */
       /*if(Node_hasChild(oNParent, oPNPath, &ulIndex)) {
            fprintf(stderr, "A parent already has a child with this path\n");
            return FALSE;
         }*/

      
      /* all children under a parent must be in lexigraphical order*/
        for(ulIndex = 0; ulIndex < Node_getNumChildren(oNParent) - 1; ulIndex++)
         {
            Node_T prev = NULL;
            Node_T next = NULL;
            int iStatusPrev = Node_getChild(oNParent, ulIndex, &prev);
            int iStatusNext = Node_getChild(oNParent, ulIndex + 1, &next);
            if(iStatusPrev != SUCCESS) {
               fprintf(stderr, "getNumChildren claims more children than getChild returns\n");
               return FALSE;
            }
            if(iStatusNext != SUCCESS) {
               fprintf(stderr, "getNumChildren claims more children than getChild returns\n");
               return FALSE;
            }
            if (Node_compare(prev, next) > 0)
            {
               fprintf(stderr, "The children of a parent are not in lexigraphical order\n");
               return FALSE;
            }
         }
            
   }
   else
   {
      /* Sample check: if there is no parent, the current node must be a root */
      if(Path_getDepth(oPNPath) != 1) {
         fprintf(stderr, "There is only one node, which must be a root\n");
            return FALSE;
        }
    }


    return TRUE;
}

/*
   Performs a pre-order traversal of the tree rooted at oNNode.
   Returns -1 if a broken invariant is found and
   returns the number of valid children from a node otherwise.

*/
static int CheckerDT_treeCheck(Node_T oNNode) {
   size_t ulIndex;
   int ulCount = 0;
   int lengthOfSubtree;


   if(oNNode!= NULL) {
      ulCount = 1;
      /* Sample check on each node: node must be valid */
      /* If not, pass that failure back up immediately */
      if(!CheckerDT_Node_isValid(oNNode))
         return -1;

      /* Recur on every child of oNNode */
      for(ulIndex = 0; ulIndex < Node_getNumChildren(oNNode); ulIndex++)
      {
         Node_T oNChild = NULL;
         int iStatus = Node_getChild(oNNode, ulIndex, &oNChild);
         
         if(iStatus != SUCCESS) {
            fprintf(stderr, "getNumChildren claims more children than getChild returns\n");
            return -1;
         }

         /* if recurring down one subtree results in a failed check
            farther down, passes the failure back up immediately */
         lengthOfSubtree = CheckerDT_treeCheck(oNChild);
         if(lengthOfSubtree == -1)
         {
            return -1;
         }
         else
         {
            ulCount += lengthOfSubtree;
         }
      }
   }
   return ulCount;
}

/* see checkerDT.h for specification */
boolean CheckerDT_isValid(boolean bIsInitialized, Node_T oNRoot,
                          size_t ulCount) {
   int check;
    /* Sample check on a top-level data structure invariant:
       if the DT is not initialized, its count should be 0. */
    if (!bIsInitialized)
    {
         if (oNRoot)
         {
            fprintf(stderr, "Not initialized, but root still exists\n");
            return FALSE;
         }
        if (ulCount != 0) {
            fprintf(stderr, "Not initialized, but count is not 0\n");
            return FALSE;
        }
    }
   
   /*The size of the tree should be equal to the actual number of valid nodes in the tree*/
   check = CheckerDT_treeCheck(oNRoot);
   if (check != (int)(ulCount))
   {
      fprintf(stderr, "Size of tree is not equal to the number of valid nodes in the tree\n");
      return FALSE;
   }
   
   return TRUE;
}
