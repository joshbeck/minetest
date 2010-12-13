/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef MAPNODE_HEADER
#define MAPNODE_HEADER

#include <iostream>
#include "common_irrlicht.h"
#include "light.h"
#include "utility.h"
#include "exceptions.h"
#include "serialization.h"

// Size of node in rendering units
#define BS 10

#define MATERIALS_COUNT 256

/*
	Ignored node.

	Anything that stores MapNodes doesn't have to preserve parameters
	associated with this material.
	
	Doesn't create faces with anything and is considered being
	out-of-map in the game map.
*/
#define CONTENT_IGNORE 255
#define CONTENT_IGNORE_DEFAULT_PARAM 0

/*
	The common material through which the player can walk and which
	is transparent to light
*/
#define CONTENT_AIR 254

/*
	Suggested materials:
	GRAVEL
	  - Dynamics of gravel: if there is a drop of more than two
	    blocks on any side, it will drop in there. Is this doable?
	
	New naming scheme:
	- Material = irrlicht's Material class
	- Content = (u8) content of a node
	- Tile = (u16) Material ID at some side of a node
*/

enum Content
{
	CONTENT_STONE=0,

	CONTENT_GRASS,

	CONTENT_WATER,

	CONTENT_LIGHT,

	CONTENT_TREE,
	
	CONTENT_LEAVES,

	CONTENT_GRASS_FOOTSTEPS,
	
	CONTENT_MESE,

	CONTENT_MUD,

	CONTENT_OCEAN,
	
	// This is set to the number of the actual values in this enum
	USEFUL_CONTENT_COUNT
};

/*
	If true, the material allows light propagation and brightness is stored
	in param.
*/
inline bool light_propagates_content(u8 m)
{
	return (m == CONTENT_AIR || m == CONTENT_LIGHT || m == CONTENT_WATER || m == CONTENT_OCEAN);
}

/*
	If true, the material allows lossless sunlight propagation.
*/
inline bool sunlight_propagates_content(u8 m)
{
	return (m == CONTENT_AIR || m == CONTENT_LIGHT);
}

/*
	On a node-node surface, the material of the node with higher solidness
	is used for drawing.
	0: Invisible
	1: Transparent
	2: Opaque
*/
inline u8 content_solidness(u8 m)
{
	if(m == CONTENT_AIR)
		return 0;
	if(m == CONTENT_WATER || m == CONTENT_OCEAN)
		return 1;
	return 2;
}

// Objects collide with walkable contents
inline bool content_walkable(u8 m)
{
	return (m != CONTENT_AIR && m != CONTENT_WATER && m != CONTENT_OCEAN && m != CONTENT_LIGHT);
}

// A liquid resists fast movement
inline bool content_liquid(u8 m)
{
	return (m == CONTENT_WATER || m == CONTENT_OCEAN);
}

// Pointable contents can be pointed to in the map
inline bool content_pointable(u8 m)
{
	return (m != CONTENT_AIR && m != CONTENT_WATER && m != CONTENT_OCEAN);
}

inline bool content_diggable(u8 m)
{
	return (m != CONTENT_AIR && m != CONTENT_WATER && m != CONTENT_OCEAN);
}

inline bool content_buildable_to(u8 m)
{
	return (m == CONTENT_AIR || m == CONTENT_WATER || m == CONTENT_OCEAN);
}

/*
	TODO: Make a mapper class for mapping every side of a content
	      to some tile.
	This dumbily maps all sides of content to the tile of the same id.
*/
inline u8 content_tile(u8 c)
{
	if(c == CONTENT_IGNORE || c == CONTENT_LIGHT)
		return CONTENT_AIR;
	return c;
}

/*
	Returns true for contents that form the base ground that
	follows the main heightmap
*/
inline bool is_ground_content(u8 m)
{
	return(
		m == CONTENT_STONE ||
		m == CONTENT_GRASS ||
		m == CONTENT_GRASS_FOOTSTEPS ||
		m == CONTENT_MESE ||
		m == CONTENT_MUD
	);
}

/*
	Nodes make a face if contents differ and solidness differs.
	Return value:
		0: No face
		1: Face uses m1's content
		2: Face uses m2's content
*/
inline u8 face_contents(u8 m1, u8 m2)
{
	if(m1 == CONTENT_IGNORE || m2 == CONTENT_IGNORE)
		return 0;
	
	bool contents_differ = (m1 != m2);
	bool solidness_differs = (content_solidness(m1) != content_solidness(m2));
	bool makes_face = contents_differ && solidness_differs;

	if(makes_face == false)
		return 0;

	if(content_solidness(m1) > content_solidness(m2))
		return 1;
	else
		return 2;
}

inline bool liquid_replaces_content(u8 c)
{
	return (c == CONTENT_AIR || c == CONTENT_LIGHT);
}

/*
	When placing a node, drection info is added to it if this is true
*/
inline bool content_directional(u8 c)
{
	return (c == CONTENT_LIGHT);
}

/*
	Packs directions like (1,0,0), (1,-1,0)
*/
inline u8 packDir(v3s16 dir)
{
	u8 b = 0;

	if(dir.X > 0)
		b |= (1<<0);
	else if(dir.X < 0)
		b |= (1<<1);

	if(dir.Y > 0)
		b |= (1<<2);
	else if(dir.Y < 0)
		b |= (1<<3);

	if(dir.Z > 0)
		b |= (1<<4);
	else if(dir.Z < 0)
		b |= (1<<5);
	
	return b;
}
inline v3s16 unpackDir(u8 b)
{
	v3s16 d(0,0,0);

	if(b & (1<<0))
		d.X = 1;
	else if(b & (1<<1))
		d.X = -1;

	if(b & (1<<2))
		d.Y = 1;
	else if(b & (1<<3))
		d.Y = -1;

	if(b & (1<<4))
		d.Z = 1;
	else if(b & (1<<5))
		d.Z = -1;
	
	return d;
}

struct MapNode
{
	// Content
	u8 d;

	/*
		Misc parameter. Initialized to 0.
		- For light_propagates() blocks, this is light intensity,
		  stored logarithmically from 0 to LIGHT_MAX.
		  Sunlight is LIGHT_SUN, which is LIGHT_MAX+1.
	*/
	s8 param;
	
	union
	{
		/*
			Pressure for liquids
		*/
		u8 pressure;

		/*
			Direction for torches and other stuff.
			If possible, packed with packDir.
		*/
		u8 dir;
	};

	MapNode(const MapNode & n)
	{
		*this = n;
	}
	
	MapNode(u8 data=CONTENT_AIR, u8 a_param=0, u8 a_pressure=0)
	{
		d = data;
		param = a_param;
		pressure = a_pressure;
	}

	bool operator==(const MapNode &other)
	{
		return (d == other.d
				&& param == other.param
				&& pressure == other.pressure);
	}

	bool light_propagates()
	{
		return light_propagates_content(d);
	}
	
	bool sunlight_propagates()
	{
		return sunlight_propagates_content(d);
	}
	
	u8 solidness()
	{
		return content_solidness(d);
	}

	u8 light_source()
	{
		/*
			Note that a block that isn't light_propagates() can be a light source.
		*/
		if(d == CONTENT_LIGHT)
			return LIGHT_MAX;
		
		return 0;
	}

	u8 getLight()
	{
		// Select the brightest of [light source, propagated light]
		u8 light = 0;
		if(light_propagates())
			light = param & 0x0f;
		if(light_source() > light)
			light = light_source();
		return light;
	}

	void setLight(u8 a_light)
	{
		// If not transparent, can't set light
		if(light_propagates() == false)
			return;
		param = a_light;
	}

	/*
		These serialization functions are used when informing client
		of a single node add
	*/

	static u32 serializedLength(u8 version)
	{
		if(!ser_ver_supported(version))
			throw VersionMismatchException("ERROR: MapNode format not supported");
			
		if(version == 0)
			return 1;
		else if(version <= 9)
			return 2;
		else
			return 3;
	}
	void serialize(u8 *dest, u8 version)
	{
		if(!ser_ver_supported(version))
			throw VersionMismatchException("ERROR: MapNode format not supported");
			
		if(version == 0)
		{
			dest[0] = d;
		}
		else if(version <= 9)
		{
			dest[0] = d;
			dest[1] = param;
		}
		else
		{
			dest[0] = d;
			dest[1] = param;
			dest[2] = pressure;
		}
	}
	void deSerialize(u8 *source, u8 version)
	{
		if(!ser_ver_supported(version))
			throw VersionMismatchException("ERROR: MapNode format not supported");
			
		if(version == 0)
		{
			d = source[0];
		}
		else if(version == 1)
		{
			d = source[0];
			// This version doesn't support saved lighting
			if(light_propagates() || light_source() > 0)
				param = 0;
			else
				param = source[1];
		}
		else if(version <= 9)
		{
			d = source[0];
			param = source[1];
		}
		else
		{
			d = source[0];
			param = source[1];
			pressure = source[2];
		}
	}
};

/*
	Returns integer position of the node in given
	floating point position.
*/
inline v3s16 floatToInt(v3f p)
{
	v3s16 p2(
		(p.X + (p.X>0 ? BS/2 : -BS/2))/BS,
		(p.Y + (p.Y>0 ? BS/2 : -BS/2))/BS,
		(p.Z + (p.Z>0 ? BS/2 : -BS/2))/BS);
	return p2;
}

inline v3f intToFloat(v3s16 p)
{
	v3f p2(
		p.X * BS,
		p.Y * BS,
		p.Z * BS
	);
	return p2;
}



#endif
