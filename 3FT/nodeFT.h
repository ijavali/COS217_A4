/*--------------------------------------------------------------------*/
/* nodeDT.h                                                           */
/* Author: Ishaan Javali, Jack Zhang, crediting Christopher Moretti                                        */
/*--------------------------------------------------------------------*/

#ifndef NODE_INCLUDED
#define NODE_INCLUDED

#include <stddef.h>
#include "a4def.h"
#include "path.h"


/* A Node_T is a node in a Directory Tree */
typedef struct node *Node_T;

/*
  Creates a new node in the Directory Tree, with path oPPath and
  parent oNParent. If the node is a file (i.e. isFile is true), 
  set the node's value equal to value and the node's content length
  equal to ulLength.
  Returns an int SUCCESS status and sets *poNResult
  to be the new node if successful. Otherwise, sets *poNResult to NULL
  and returns status:
  * MEMORY_ERROR if memory could not be allocated to complete request
  * CONFLICTING_PATH if oNParent's path is not an ancestor of oPPath
  * NO_SUCH_PATH if oPPath is of depth 0
                 or oNParent's path is not oPPath's direct parent
                 or oNParent is NULL but oPPath is not of depth 1
  * ALREADY_IN_TREE if oNParent already has a child with this path
*/
int Node_new(Path_T oPPath, Node_T oNParent, Node_T *poNResult, boolean isFile, void* value, size_t ulLength);

/*
  Destroys and frees all memory allocated for the subtree rooted at
  oNNode, i.e., deletes this node and all its descendents. Returns the
  number of nodes deleted.
*/
size_t Node_free(Node_T oNNode);

/* Returns the path object representing oNNode's absolute path. */
Path_T Node_getPath(Node_T oNNode);

/*
  Returns TRUE if oNParent has a child with path oPPath. Returns
  FALSE if it does not.
  If oNParent has such a child, stores in *pulChildID the child's
  identifier (as used in Node_getChild). If oNParent does not have
  such a child, stores in *pulChildID the identifier that such a
  child _would_ have if inserted.
*/
boolean Node_hasChild(Node_T oNParent, Path_T oPPath, boolean isFile,
                         size_t *pulChildID);

/*
  Returns TRUE if oNNode is a file, FALSE if it is a directory
*/
boolean Node_isFile(Node_T oNNode);

/*
  Returns the size of file contents in bytes if oNNode is a file;
  else returns 0.
*/
size_t Node_getUlLength(Node_T oNNode);

/*
  Sets the size of oNNode's file contents in bytes to ulLength.
  Returns nothing.
*/
void Node_setUlLength(Node_T oNNode, size_t ulLength);

/*
  Returns a pointer to the value of oNNode's file contents.
  Returns NULL if not a file.
*/
void* Node_getValue(Node_T oNNode);

/*
  Sets the pointer to the value of oNNode's file contents to
  the parameter 'value'.
  Returns nothing.
*/
void Node_setValue(Node_T oNNode, void* value);

/* Returns the number of file children that oNParent has. */
size_t Node_getNumFileChildren(Node_T oNParent);

/* Returns the number of directory children that oNParent has. */
size_t Node_getNumDirChildren(Node_T oNParent);

/*
  Returns an int SUCCESS status and sets *poNResult to be the child
  node of oNParent with identifier ulChildID, if one exists.
  Otherwise, sets *poNResult to NULL and returns status:
  * NO_SUCH_PATH if ulChildID is not a valid child for oNParent
*/
int Node_getChild(Node_T oNParent, size_t ulChildID, boolean isFile,
                  Node_T *poNResult);

/*
  Returns a the parent node of oNNode.
  Returns NULL if oNNode is the root and thus has no parent.
*/
Node_T Node_getParent(Node_T oNNode);

/*
  Compares oNFirst and oNSecond lexicographically based on their paths.
  Returns <0, 0, or >0 if onFirst is "less than", "equal to", or
  "greater than" oNSecond, respectively.
*/
int Node_compare(Node_T oNFirst, Node_T oNSecond);

/*
  Returns a string representation for oNNode, or NULL if
  there is an allocation error.
  Allocates memory for the returned string, which is then owned by
  the caller!
*/
char *Node_toString(Node_T oNNode);

/*
  Returns value of oNNode if oNNode is a file, otherwise returns NULL.
*/
void *Node_getValue(Node_T oNNode);
#endif