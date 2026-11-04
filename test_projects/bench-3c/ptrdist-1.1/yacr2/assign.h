/*
 *
 * assign.h
 *
 */


/*
 *
 * Includes.
 *
 */

#include "types.h"
#include "vcg.h"
#include "hcg.h"

#ifndef ASSIGN_H
#define ASSIGN_H


/*
 *
 * Defines.
 *
 */

#define	LOW		1
#define	MEDIUM		100
#define	HIGH		10000
#define	INFINITY	1000000


/*
 *
 * Types.
 *
 */


/*
 *
 * Globals.
 *
 */

extern ulong * *		costMatrix;
extern ulong *		tracksNoHCV;
extern ulong *		tracksNotPref;
extern ulong *		tracksTopNotPref;
extern ulong *		tracksBotNotPref;
extern ulong			cardNotPref;
extern ulong			cardTopNotPref;
extern ulong			cardBotNotPref;
extern ulong *		tracksAssign;
extern ulong *		netsAssign;
extern ulong *		netsAssignCopy;


/*
 *
 * Prototypes.
 *
 */

void
AllocAssign(void);

void
FreeAssign(void);

void
NetsAssign(void);

void
MaxNetsAssign(void);

void
RightNetsAssign(void);

void
LeftNetsAssign(void);

void
Assign(nodeVCGType *,       ulong *,       ulong);

void
Select(nodeVCGType *,       nodeHCGType *,       ulong *,       ulong *,       ulong *);

void
BuildCostMatrix(nodeVCGType *,		nodeHCGType *,		ulong *,		ulong *);

void
IdealTrack(ulong tracks, ulong top, ulong bot, ulong * ideal);


#endif	/* ASSIGN_H */

