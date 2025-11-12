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

   /* Sample check: a NULL pointer is not a valid node */
   if(oNNode == NULL) {
      fprintf(stderr, "A node is a NULL pointer\n");
      return FALSE;
   }

   /* Sample check: parent's path must be the longest possible
      proper prefix of the node's path */
   oNParent = Node_getParent(oNNode);
   if(oNParent != NULL) {
      oPNPath = Node_getPath(oNNode);
      oPPPath = Node_getPath(oNParent);

      if(Path_getSharedPrefixDepth(oPNPath, oPPPath) !=
         Path_getDepth(oPNPath) - 1) {
         fprintf(stderr, "P-C nodes don't have P-C paths: (%s) (%s)\n",
                 Path_getPathname(oPPPath), Path_getPathname(oPNPath));
         return FALSE;
      }
   }

   return TRUE;
}

/*
   static helper function to recursively count the actual number of nodes in the tree.
*/
static size_t CheckerDT_countNodes(Node_T oNNode) {
   size_t count = 0;
   size_t ulIndex;
   
   if(oNNode == NULL)
      return 0;
   
   count = 1; /* initialized this node */
   
   for(ulIndex = 0; ulIndex < Node_getNumChildren(oNNode); ulIndex++) {
      Node_T oNChild = NULL;
      int iStatus = Node_getChild(oNNode, ulIndex, &oNChild);
      if(iStatus == SUCCESS) {
         count += CheckerDT_countNodes(oNChild);
      }
   }
   
   return count;
}

/*
   helper function to check for duplicate paths in the tree
*/
static boolean CheckerDT_noDuplicatePaths(Node_T oNNode, DynArray_T oPaths) {
   size_t ulIndex;
   size_t i;
   Path_T oPCurrentPath;
   
   if(oNNode == NULL)
      return TRUE;

   /* get this path */
   oPCurrentPath = Node_getPath(oNNode);
   
   /* check if this current path is NULL */
   if(oPCurrentPath == NULL) {
      fprintf(stderr, "Node has NULL path\n");
      return FALSE;
   }
   
   /* check if this path already exists in our array */
   for(i = 0; i < DynArray_getLength(oPaths); i++) {
      Path_T oPExistingPath = (Path_T)DynArray_get(oPaths, i);
    
      /* skip NULL entries */
      if(oPExistingPath == NULL) continue;
         
      if(Path_comparePath(oPCurrentPath, oPExistingPath) == 0) {
         fprintf(stderr, "found duplicate path: %s\n", 
                 Path_getPathname(oPCurrentPath));
         return FALSE;
      }
   }
   
   /* add current path to array */
   if(DynArray_add(oPaths, oPCurrentPath) == 0) {
      fprintf(stderr, "Failed to add path to checking array\n");
      return FALSE;
   }
   
   /* check children recursively */
   for(ulIndex = 0; ulIndex < Node_getNumChildren(oNNode); ulIndex++) {
      Node_T oNChild = NULL;
      int iStatus = Node_getChild(oNNode, ulIndex, &oNChild);
      if(iStatus == SUCCESS) {
         if(!CheckerDT_noDuplicatePaths(oNChild, oPaths))
            return FALSE;
      }
   }
   
   return TRUE;
}

/*
   Performs a pre-order traversal of the tree rooted at oNNode.
   Returns FALSE if a broken invariant is found and
   returns TRUE otherwise.
*/
static boolean CheckerDT_treeCheck(Node_T oNNode) {
   size_t ulIndex;

   if(oNNode != NULL) {
      /* Sample check on each node: node must be valid */
      /* If not, pass that failure back up immediately */
      if(!CheckerDT_Node_isValid(oNNode))
         return FALSE;

      /* Recur on every child of oNNode */
      for(ulIndex = 0; ulIndex < Node_getNumChildren(oNNode); ulIndex++)
      {
         Node_T oNChild = NULL;
         int iStatus = Node_getChild(oNNode, ulIndex, &oNChild);
         
         if(iStatus != SUCCESS) {
            fprintf(stderr, "getNumChildren claims more children than getChild returns\n");
            return FALSE;
         }

         /* check: children must follow lexicographic order */
         if(ulIndex > 0) {
            Node_T oNPrevChild = NULL;
            int iPrevStatus = Node_getChild(oNNode, ulIndex-1, &oNPrevChild);
            
            if(iPrevStatus != SUCCESS) {
               fprintf(stderr, "could not get previous child in ordering check\n");
               return FALSE;
            }
            
            if(Path_comparePath(Node_getPath(oNPrevChild), 
                               Node_getPath(oNChild)) >= 0) {
               fprintf(stderr, "child of node %s are not in lexicographic order\n",
                       Path_getPathname(Node_getPath(oNNode)));
               fprintf(stderr, "wrong order of child at index %lu and %lu\n", (unsigned long)(ulIndex-1),
               (unsigned long)ulIndex);
               return FALSE;
            }
         }

         /* if recurring down one subtree results in a failed check
            farther down, passes the failure back up immediately */
         if(!CheckerDT_treeCheck(oNChild))
            return FALSE;
      }
   }
   return TRUE;
}

/* see checkerDT.h for specification */
boolean CheckerDT_isValid(boolean bIsInitialized, Node_T oNRoot,
                          size_t ulCount) {
   DynArray_T oPaths;
   boolean result;
   
   /* Sample check on a top-level data structure invariant:
      if the DT is not initialized, its count should be 0. */
   if(!bIsInitialized) {
      if(ulCount != 0) {
         fprintf(stderr, "Not initialized, but count is not 0\n");
         return FALSE;
      }
   }
   
   /* check: object count must match actual number of nodes in tree */
   if(bIsInitialized) {
      size_t ulActualCount = CheckerDT_countNodes(oNRoot);
      if(ulActualCount != ulCount) {
         fprintf(stderr, "Count is %lu while we have %lu actual nodes \n", 
                 (unsigned long)ulCount, (unsigned long)ulActualCount);
         return FALSE;
      }
   }
   
   /* check: no duplicate paths in the tree */
   if(oNRoot != NULL) {
      size_t arraySize;
      if (ulCount > 0) {
       arraySize = ulCount;
      } else 
	arraySize = 1;
      oPaths = DynArray_new(arraySize);
      if(oPaths == NULL) {
         fprintf(stderr, "could not create array for duplicate path checking\n");
         return FALSE;
      }
      
      result = CheckerDT_noDuplicatePaths(oNRoot, oPaths);
      DynArray_free(oPaths);
      
      if(!result)
         return FALSE;
   }
   
   /* Now checks invariants recursively at each node from the root. */
   return CheckerDT_treeCheck(oNRoot);
}
