
/*
 *
 * channel.h
 *
 */


/*
 *
 * Includes.
 *
 */

#include "types.h"


#ifndef CHANNEL_H
#define CHANNEL_H


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


/*
 *
 * Globals.
 *
 */

extern ulong *	TOP;
extern ulong *	BOT;
extern ulong *	FIRST;
extern ulong *	LAST;
extern ulong *	DENSITY;
extern ulong *	CROSSING;
extern ulong		channelNets;
extern ulong		channelColumns;
extern ulong		channelTracks;
extern ulong		channelTracksCopy;
extern ulong		channelDensity;
extern ulong		channelDensityColumn;
extern char *	channelFile;

/*
 *
 * Prototypes.
 *
 */

void
BuildChannel(void);

void
DimensionChannel(void);

void
DescribeChannel(void);

void
DensityChannel(void);

#endif	/* CHANNEL_H */
