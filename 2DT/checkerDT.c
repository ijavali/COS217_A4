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

size_t debug = 0;

/* see checkerDT.h for specification */
boolean CheckerDT_Node_isValid(Node_T oNNode) {
    Node_T oNParent;
    Path_T oPNPath;
    Path_T oPPPath;
    size_t ulIndex, ulDepth;

    /* Sample check: a NULL pointer is not a valid node */
    if (oNNode == NULL) {
        fprintf(stderr, "A node is a NULL pointer\n");
        return FALSE;
    }

    oNParent = Node_getParent(oNNode);

    oPNPath = Node_getPath(oNNode);
    if (oNParent != NULL) {
        oPPPath = Node_getPath(oNParent);
        if(debug) fprintf(stderr, "HEREEE\n");

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

        ulIndex = Path_getDepth(oPNPath);
        ulDepth = Path_getDepth(oPPPath);

             fprintf(stderr, " -->  %s %s ||  %d %d ||\n", Path_getPathname(oPPPath),
                Path_getPathname(oPNPath), ulDepth, ulIndex - 1);
                fprintf(stderr, " .      %d\n", Path_comparePath(oPNPath, oPPPath));

        /* oNCurr is the node we're trying to insert */
        char *pName = Path_getPathname(oPPPath);
        char *nName = Path_getPathname(oPNPath);
        size_t pLen = Path_getStrLength(oPPPath);
        size_t nLen = Path_getStrLength(oPNPath);
        size_t i = 0, cnt = 0;
        while(i < pLen && i < nLen){
         if(pName[i] != nName[i]) break;
         if(pName[i] == '/') cnt ++;
         i++;
        }
        
     /*    if (Path_getSharedPrefixDepth(oPNPath, oPPPath) == ulDepth &&
            Path_getSharedPrefixDepth(oPNPath, oPPPath) == ulIndex - 1 
            && cnt == ulDepth - 1) {
            fprintf(stderr,
                    "A parent node already has a child in the tree with an "
                    "identical path\n");
            return FALSE;
        } */
        /* Sample check: parent must not already have a child with this path */
         if (Node_hasChild(oNParent, oPNPath, &ulIndex)) {
            Node_hasChild(oNParent, oPPPath, &ulIndex);
             fprintf(stderr, " the id is: %d\n", ulIndex);
             Node_hasChild(oNParent, oPNPath, &ulIndex);
             fprintf(stderr, " the id is: %d\n", ulIndex);
             fprintf(stderr, " ||  %s %s  ||", Path_getPathname(oPPPath),
                Path_getPathname(oPNPath));
             Node_hasChild(oNNode, oPNPath, &ulIndex);
             fprintf(stderr, " the id is: %d\n", ulIndex);
             fprintf(stderr,
                     "A parent node already has a child in the tree with an "
                     "identical path\n");
             return FALSE;
         }
 
    } else {
        /* Sample check: if there is no parent, the current node must be a root
         */
        if(debug) fprintf(stderr, "asdfasdf aHEREEE %s \n", Path_getPathname(oPNPath));
        /* there is some issue with oPNPath or something. like its null.*/
        if (Path_getDepth(oPNPath) != 1) {
            fprintf(stderr, "asdfasdf aHEREEE\n");
            fprintf(stderr, "There is only one node, which must be a root\n");
            return FALSE;
        }
    }

    return TRUE;
}

/*
   Performs a pre-order traversal of the tree rooted at oNNode.
   Returns FALSE if a broken invariant is found and
   returns TRUE otherwise.

   You may want to change this function's return type or
   parameter list to facilitate constructing your checks.
   If you do, you should update this function comment.   
*/
static boolean CheckerDT_treeCheck(Node_T oNNode) {
    size_t ulIndex;

    if (oNNode != NULL) {
        /* Sample check on each node: node must be valid */
        /* If not, pass that failure back up immediately */
        if (!CheckerDT_Node_isValid(oNNode)) return FALSE;

        /* Recur on every child of oNNode */
        for (ulIndex = 0; ulIndex < Node_getNumChildren(oNNode); ulIndex++) {
            Node_T oNChild = NULL;
            int iStatus = Node_getChild(oNNode, ulIndex, &oNChild);

            if (iStatus != SUCCESS) {
                fprintf(stderr,
                        "getNumChildren claims more children than getChild "
                        "returns\n");
                return FALSE;
            }

            /* if recurring down one subtree results in a failed check
               farther down, passes the failure back up immediately */
            if (!CheckerDT_treeCheck(oNChild)) return FALSE;
        }
    }
    return TRUE;
}

/* see checkerDT.h for specification */
boolean CheckerDT_isValid(boolean bIsInitialized, Node_T oNRoot,
                          size_t ulCount) {
    /* Sample check on a top-level data structure invariant:
       if the DT is not initialized, its count should be 0. */
    if (!bIsInitialized)
        if (ulCount != 0) {
            fprintf(stderr, "Not initialized, but count is not 0\n");
            return FALSE;
        }

    /* Now checks invariants recursively at each node from the root. */
    return CheckerDT_treeCheck(oNRoot);
}
