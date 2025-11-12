/*--------------------------------------------------------------------*/
/* nodeFT.h                                                           */
/* Author: [Your Name]                                                */
/*--------------------------------------------------------------------*/

#ifndef NODEFT_INCLUDED
#define NODEFT_INCLUDED

#include <stddef.h>
#include "a4def.h"
#include "path.h"

/* A Node_T is an object that contains a path payload and references to
   the node's parent (if it exists) and children (if they exist). */
typedef struct node *Node_T;

/*
  Creates a new node with path oPPath and parent oNParent. Returns an
  int SUCCESS status and sets *poNResult to be the new node if
  successful. Otherwise, sets *poNResult to NULL and returns status:
  * MEMORY_ERROR if memory could not be allocated to complete request
  * CONFLICTING_PATH if oNParent's path is not an ancestor of oPPath
  * NO_SUCH_PATH if oPPath is of depth 0
                 or oNParent's path is not oPPath's direct parent
                 or oNParent is NULL but oPPath is not of depth 1
  * ALREADY_IN_TREE if oNParent already has a child with this path
  * NOT_A_DIRECTORY if oNParent is a file (files cannot have children)
  
  If bIsFile is TRUE, creates a file node with contents pvContents of
  length ulLength. If bIsFile is FALSE, creates a directory node
  (pvContents and ulLength are ignored).
*/
int Node_new(Path_T oPPath, Node_T oNParent, Node_T *poNResult,
             boolean bIsFile, void *pvContents, size_t ulLength);

/*
  Destroys the entire hierarchy of nodes rooted at oNNode,
  including oNNode itself. Returns the number of nodes destroyed.
*/
size_t Node_free(Node_T oNNode);

/* Returns the path object of oNNode. */
Path_T Node_getPath(Node_T oNNode);

/*
  Returns TRUE if oNParent has a child with path oPPath and returns
  that child's identifier in *pulChildID. Returns FALSE if no such
  child exists. *pulChildID is unchanged if there is no such child.
*/
boolean Node_hasChild(Node_T oNParent, Path_T oPPath,
                      size_t *pulChildID);

/* Returns the number of children of oNParent. */
size_t Node_getNumChildren(Node_T oNParent);

/*
  Returns the child of oNParent with identifier ulChildID and sets
  *poNResult to be that child. If there is no such child, returns
  NO_SUCH_PATH and sets *poNResult to NULL.
*/
int Node_getChild(Node_T oNParent, size_t ulChildID,
                  Node_T *poNResult);

/* Returns the parent of oNNode, or NULL if oNNode is the root. */
Node_T Node_getParent(Node_T oNNode);

/*
  Compares oNFirst and oNSecond lexicographically based on their paths.
  Returns <0, 0, or >0 if oNFirst is less than, equal to, or greater
  than oNSecond, respectively.
*/
int Node_compare(Node_T oNFirst, Node_T oNSecond);

/*
  Returns a string representation of oNNode's path, or NULL if there
  is an allocation error. Allocates memory for the returned string,
  which is then owned by the client.
*/
char *Node_toString(Node_T oNNode);


/*-------------------------------------------------------*/
/* The following functions are specific to the implementation
 * of ft where oNNode could be a file or a directory      */
/*--------------------------------------------------------*/
/* Returns TRUE if oNNode is a file, FALSE if it is a directory. */
boolean Node_isFile(Node_T oNNode);

/*
  Returns a pointer to the contents of oNNode, Returns NULL if 
  oNNode is a directory,
*/
void *Node_getContents(Node_T oNNode);

/*
  Returns the length in bytes of oNNode's contents (0 if oNNode is a
  directory).
*/
size_t Node_getLength(Node_T oNNode);


/*
  Replaces the contents pointer of file oNNode with pointer pvNewContents
  of length ulNewLength (does not free the old contents; used for replacement).
  Returns SUCCESS if successful, or:
  * NOT_A_FILE if oNNode is a directory
  Caller is responsible for managing the old contents memory.
*/
int Node_replaceContents(Node_T oNNode, void *pvNewContents,
                         size_t ulNewLength);

#endif
