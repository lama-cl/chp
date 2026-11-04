
/*
 *
 * vcg.h
 *
 */


/*
 *
 * Includes.
 *
 */

#include "types.h"


#ifndef VCG_H
#define VCG_H

/*
 *
 * Defines.
 *
 */

#define	EMPTY			0
#define	EMPTY_HALF_EMPTY	2
#define	EMPTY_FULL		3
#define	HALF_EMPTY		4
#define	HALF_EMPTY_FULL		5
#define	FULL			6


/*
 *
 * Types.
 *
 */

typedef struct _constraintVCGType {
    ulong	top;
    ulong	bot;
    ulong	col;
    ulong	removed;
} constraintVCGType;

typedef struct _nodeVCGType {
    constraintVCGType * netsAboveHook;
    ulong			netsAbove;
    ulong			netsAboveLabel;
    ulong			netsAboveReached;
    constraintVCGType * netsBelowHook;
    ulong			netsBelow;
    ulong			netsBelowLabel;
    ulong			netsBelowReached;
} nodeVCGType;


/*
 *
 * Globals.
 *
 */

extern nodeVCGType *			VCG;
extern constraintVCGType *		storageRootVCG;
extern constraintVCGType *		storageVCG;
extern ulong				storageLimitVCG;
extern constraintVCGType * *	removeVCG;
extern ulong				removeTotalVCG;
extern ulong *			SCC;
extern ulong				totalSCC;
extern ulong *			perSCC;


/*
 *
 * Prototypes.
 *
 */

void
AllocVCG(void);

void
FreeVCG(void);

void
BuildVCG(void);

void
DFSClearVCG(nodeVCGType *);

void
DumpVCG(nodeVCGType *);

void
DFSAboveVCG(nodeVCGType *,	    ulong);

void DFSBelowVCG(nodeVCGType *,	    ulong);

void SCCofVCG(nodeVCGType *,	 ulong *, 	 ulong *);

void SCC_DFSAboveVCG(nodeVCGType *,		ulong,		ulong *);

void SCC_DFSBelowVCG(nodeVCGType *,		ulong,		ulong);

void DumpSCC(ulong *,	ulong *);

void AcyclicVCG(void);

void RemoveConstraintVCG(nodeVCGType *,		    ulong *,		    ulong *,		    constraintVCGType * *);

ulong ExistPathAboveVCG(nodeVCGType *,		  ulong,		  ulong);

void LongestPathVCG(nodeVCGType *,	       ulong);

ulong DFSAboveLongestPathVCG(nodeVCGType *,		       ulong);

ulong DFSBelowLongestPathVCG(nodeVCGType *, 		       ulong);

ulong VCV(nodeVCGType *,    ulong,    ulong,    ulong *);

#endif	/* VCG_H */
