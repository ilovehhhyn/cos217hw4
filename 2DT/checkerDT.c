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
   Helper function to count the actual number of nodes in the tree.
*/
static size_t CheckerDT_countNodes(Node_T oNNode) {
   size_t count = 0;
   size_t ulIndex;
   
   if(oNNode == NULL)
      return 0;
   
   count = 1; /* Count this node */
   
   /* Count all descendants */
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
   Helper function to check for duplicate paths in the tree.
   Uses a DynArray to track all paths seen so far.
*/
static boolean CheckerDT_noDuplicatePaths(Node_T oNNode, DynArray_T oPaths) {
   size_t ulIndex;
   size_t i;
   Path_T oPCurrentPath;
   
   if(oNNode == NULL)
      return TRUE;
   
   oPCurrentPath = Node_getPath(oNNode);
   
   /* Check if current path is NULL */
   if(oPCurrentPath == NULL) {
      fprintf(stderr, "Node has NULL path\n");
      return FALSE;
   }
   
   /* Check if this path already exists in our array */
   for(i = 0; i < DynArray_getLength(oPaths); i++) {
      Path_T oPExistingPath = (Path_T)DynArray_get(oPaths, i);
      
      /* Skip NULL entries */
      if(oPExistingPath == NULL)
         continue;
         
      if(Path_comparePath(oPCurrentPath, oPExistingPath) == 0) {
         fprintf(stderr, "Duplicate path found: %s\n", 
                 Path_getPathname(oPCurrentPath));
         return FALSE;
      }
   }
   
   /* Add current path to array */
   if(DynArray_add(oPaths, oPCurrentPath) == 0) {
      fprintf(stderr, "Failed to add path to checking array\n");
      return FALSE;
   }
   
   /* Check all children recursively */
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

         /* CHECK: Children must be in lexicographic order */
         if(ulIndex > 0) {
            Node_T oNPrevChild = NULL;
            int iPrevStatus = Node_getChild(oNNode, ulIndex - 1, &oNPrevChild);
            
            if(iPrevStatus != SUCCESS) {
               fprintf(stderr, "Cannot get previous child for ordering check\n");
               return FALSE;
            }
            
            if(Path_comparePath(Node_getPath(oNPrevChild), 
                               Node_getPath(oNChild)) >= 0) {
               fprintf(stderr, "Children are not in lexicographic order at node %s\n",
                       Path_getPathname(Node_getPath(oNNode)));
               fprintf(stderr, "  Child %lu: %s\n", (unsigned long)(ulIndex-1),
                       Path_getPathname(Node_getPath(oNPrevChild)));
               fprintf(stderr, "  Child %lu: %s\n", (unsigned long)ulIndex,
                       Path_getPathname(Node_getPath(oNChild)));
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
   boolean bResult;
   
   /* Sample check on a top-level data structure invariant:
      if the DT is not initialized, its count should be 0. */
   if(!bIsInitialized) {
      if(ulCount != 0) {
         fprintf(stderr, "Not initialized, but count is not 0\n");
         return FALSE;
      }
   }
   
   /* NEW CHECK: Count must match actual number of nodes in tree */
   if(bIsInitialized) {
      size_t ulActualCount = CheckerDT_countNodes(oNRoot);
      if(ulActualCount != ulCount) {
         fprintf(stderr, "Count is %lu but actual nodes is %lu\n", 
                 (unsigned long)ulCount, (unsigned long)ulActualCount);
         return FALSE;
      }
   }
   
   /* CHECK: No duplicate paths in the tree */
   if(oNRoot != NULL) {
      size_t arraySize = (ulCount > 0) ? ulCount : 1;
      oPaths = DynArray_new(arraySize);
      if(oPaths == NULL) {
         fprintf(stderr, "Unable to create array for duplicate path checking\n");
         return FALSE;
      }
      
      bResult = CheckerDT_noDuplicatePaths(oNRoot, oPaths);
      DynArray_free(oPaths);
      
      if(!bResult)
         return FALSE;
   }
   
   /* Now checks invariants recursively at each node from the root. */
   return CheckerDT_treeCheck(oNRoot);
}
