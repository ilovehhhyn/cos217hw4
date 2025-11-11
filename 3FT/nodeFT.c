/*--------------------------------------------------------------------*/
/* nodeFT.c                                                           */
/* Author: [Your Name]                                                */
/*--------------------------------------------------------------------*/

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "dynarray.h"
#include "nodeFT.h"

/* A node in an FT */
struct node {
   /* the object corresponding to the node's absolute path */
   Path_T oPPath;
   /* this node's parent */
   Node_T oNParent;
   /* the object containing links to this node's children */
   DynArray_T oDChildren;
   /* TRUE if this node represents a file, FALSE for directory */
   boolean bIsFile;
   /* the file's contents (NULL if directory) */
   void *pvContents;
   /* the length of contents in bytes (0 if directory) */
   size_t ulLength;
};


/*
  Links new child oNChild into oNParent's children array at index
  ulIndex. Returns SUCCESS if the new child was added successfully,
  or MEMORY_ERROR if allocation fails adding oNChild to the array.
*/
static int Node_addChild(Node_T oNParent, Node_T oNChild,
                         size_t ulIndex) {
   assert(oNParent != NULL);
   assert(oNChild != NULL);

   if(DynArray_addAt(oNParent->oDChildren, ulIndex, oNChild))
      return SUCCESS;
   else
      return MEMORY_ERROR;
}

/*
  Compares the string representation of oNFirst with a string
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


int Node_new(Path_T oPPath, Node_T oNParent, Node_T *poNResult,
             boolean bIsFile, void *pvContents, size_t ulLength) {
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
      *poNResult = NULL;
      return iStatus;
   }
   psNew->oPPath = oPNewPath;

   /* validate and set the new node's parent */
   if(oNParent != NULL) {
      size_t ulSharedDepth;

      /* parent cannot be a file */
      if(oNParent->bIsFile) {
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return NOT_A_DIRECTORY;
      }

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
      if(Node_hasChild(oNParent, oPPath, &ulIndex)) {
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

   /* initialize file-specific fields */
   psNew->bIsFile = bIsFile;
   if(bIsFile) {
      /* files have no children */
      psNew->oDChildren = NULL;
      
      /* copy contents if provided */
      if(pvContents != NULL && ulLength > 0) {
         psNew->pvContents = malloc(ulLength);
         if(psNew->pvContents == NULL) {
            Path_free(psNew->oPPath);
            free(psNew);
            *poNResult = NULL;
            return MEMORY_ERROR;
         }
         memcpy(psNew->pvContents, pvContents, ulLength);
         psNew->ulLength = ulLength;
      }
      else {
         psNew->pvContents = NULL;
         psNew->ulLength = 0;
      }
   }
   else {
      /* directories have no contents */
      psNew->pvContents = NULL;
      psNew->ulLength = 0;
      
      /* initialize children array */
      psNew->oDChildren = DynArray_new(0);
      if(psNew->oDChildren == NULL) {
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return MEMORY_ERROR;
      }
   }

   /* Link into parent's children list */
   if(oNParent != NULL) {
      iStatus = Node_addChild(oNParent, psNew, ulIndex);
      if(iStatus != SUCCESS) {
         Path_free(psNew->oPPath);
         if(psNew->oDChildren != NULL)
            DynArray_free(psNew->oDChildren);
         if(psNew->pvContents != NULL)
            free(psNew->pvContents);
         free(psNew);
         *poNResult = NULL;
         return iStatus;
      }
   }

   *poNResult = psNew;
   return SUCCESS;
}

size_t Node_free(Node_T oNNode) {
   size_t ulIndex;
   size_t ulCount = 0;

   assert(oNNode != NULL);

   /* remove from parent's list */
   if(oNNode->oNParent != NULL) {
      if(DynArray_bsearch(
            oNNode->oNParent->oDChildren,
            oNNode, &ulIndex,
            (int (*)(const void *, const void *)) Node_compare)
        )
         (void) DynArray_removeAt(oNNode->oNParent->oDChildren,
                                  ulIndex);
   }

   /* recursively remove children (only if directory) */
   if(!oNNode->bIsFile && oNNode->oDChildren != NULL) {
      while(DynArray_getLength(oNNode->oDChildren) != 0) {
         ulCount += Node_free(DynArray_get(oNNode->oDChildren, 0));
      }
      DynArray_free(oNNode->oDChildren);
   }

   /* free file contents if it's a file */
   if(oNNode->bIsFile && oNNode->pvContents != NULL) {
      free(oNNode->pvContents);
   }

   /* remove path */
   Path_free(oNNode->oPPath);

   /* finally, free the struct node */
   free(oNNode);
   ulCount++;
   return ulCount;
}

Path_T Node_getPath(Node_T oNNode) {
   assert(oNNode != NULL);
   return oNNode->oPPath;
}

boolean Node_hasChild(Node_T oNParent, Path_T oPPath,
                      size_t *pulChildID) {
   assert(oNParent != NULL);
   assert(oPPath != NULL);
   assert(pulChildID != NULL);

   /* files have no children */
   if(oNParent->bIsFile)
      return FALSE;

   /* *pulChildID is the index into oNParent->oDChildren */
   return DynArray_bsearch(oNParent->oDChildren,
            (char*) Path_getPathname(oPPath), pulChildID,
            (int (*)(const void*,const void*)) Node_compareString);
}

size_t Node_getNumChildren(Node_T oNParent) {
   assert(oNParent != NULL);

   if(oNParent->bIsFile)
      return 0;

   return DynArray_getLength(oNParent->oDChildren);
}

int Node_getChild(Node_T oNParent, size_t ulChildID,
                  Node_T *poNResult) {

   assert(oNParent != NULL);
   assert(poNResult != NULL);

   /* files have no children */
   if(oNParent->bIsFile) {
      *poNResult = NULL;
      return NO_SUCH_PATH;
   }

   /* ulChildID is the index into oNParent->oDChildren */
   if(ulChildID >= Node_getNumChildren(oNParent)) {
      *poNResult = NULL;
      return NO_SUCH_PATH;
   }
   else {
      *poNResult = DynArray_get(oNParent->oDChildren, ulChildID);
      return SUCCESS;
   }
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

boolean Node_isFile(Node_T oNNode) {
   assert(oNNode != NULL);
   return oNNode->bIsFile;
}

void *Node_getContents(Node_T oNNode) {
   assert(oNNode != NULL);
   
   if(!oNNode->bIsFile)
      return NULL;
   
   return oNNode->pvContents;
}

size_t Node_getLength(Node_T oNNode) {
   assert(oNNode != NULL);
   
   if(!oNNode->bIsFile)
      return 0;
   
   return oNNode->ulLength;
}

int Node_setContents(Node_T oNNode, void *pvNewContents,
                     size_t ulNewLength) {
   void *pvNewCopy;
   
   assert(oNNode != NULL);
   
   /* can only set contents of files */
   if(!oNNode->bIsFile)
      return NOT_A_FILE;
   
   /* allocate and copy new contents */
   if(pvNewContents != NULL && ulNewLength > 0) {
      pvNewCopy = malloc(ulNewLength);
      if(pvNewCopy == NULL)
         return MEMORY_ERROR;
      memcpy(pvNewCopy, pvNewContents, ulNewLength);
   }
   else {
      pvNewCopy = NULL;
      ulNewLength = 0;
   }
   
   /* free old contents */
   if(oNNode->pvContents != NULL)
      free(oNNode->pvContents);
   
   /* set new contents */
   oNNode->pvContents = pvNewCopy;
   oNNode->ulLength = ulNewLength;
   
   return SUCCESS;
}

int Node_replaceContents(Node_T oNNode, void *pvNewContents,
                         size_t ulNewLength) {
   assert(oNNode != NULL);
   
   /* can only replace contents of files */
   if(!oNNode->bIsFile)
      return NOT_A_FILE;
   
   /* Don't free old contents - caller will handle that */
   /* Just replace the pointer directly */
   oNNode->pvContents = pvNewContents;
   oNNode->ulLength = ulNewLength;
   
   return SUCCESS;
}
