/*
 *
 * hcg.h
 *
 */


/*
 *
 * Includes.
 *
 */

#include "types.h"


#ifndef HCG_H
#define HCG_H

/*
 *
 * Defines.
 *
 */


/*
 *
 * Types.
 *
 */

typedef struct _nodeHCGType {
    ulong *     netsHook;
    ulong	nets;
    ulong	netsReached;
} nodeHCGType;


/*
 *
 * Globals.
 *
 */

extern nodeHCGType *			HCG;
extern ulong *			storageRootHCG;
extern ulong *			storageHCG;
extern ulong				storageLimitHCG;


/*
 *
 * Prototypes.
 *
 */

void
AllocHCG(void);

void
FreeHCG(void);

void
BuildHCG(void);

void
DFSClearHCG(nodeHCGType *);

void
DumpHCG(nodeHCGType *);

void
NoHCV(nodeHCGType *,      ulong,      ulong *,      ulong *);

#endif	/* HCG_H */
