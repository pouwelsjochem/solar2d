//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _Rtt_Tesselator_H__
#define _Rtt_Tesselator_H__

#include "Core/Rtt_Geometry.h"
#include "Display/Rtt_DisplayTypes.h"
#include "Renderer/Rtt_Geometry_Renderer.h"

// ----------------------------------------------------------------------------

namespace Rtt
{

class Geometry;
class Matrix;

// ----------------------------------------------------------------------------

class Tesselator
{
	protected:
		static const Vertex2 kUnitCircleVertices[];

	public:
		// If non-NULL, srcToDstSpace is used to transform all generated
		// vertices. It overrides the "origin" parameter for the Generate() 
		// methods (i.e. rounded rectangle, circle, ellipse). If NULL, then
		// the origin parameter is used to displace (translate) the vertices.
		Tesselator();
		virtual ~Tesselator();

	public:
		virtual void GetSelfBounds( Rect& rect ) = 0;

	public:
		enum eType
		{
			kType_None,
			kType_Line,
			kType_Circle,
			kType_Polygon,
			kType_Rect,
			kType_RoundedRect,
			kType_Mesh,

			kType_Count
		};

		virtual eType GetType(){ return kType_None; }

	public:
		Real GetWidth() const { return fWidth; }
		void SetWidth( Real newValue ) { fWidth = newValue; };

	protected:
		// Adds degenerate triangles so two distinct tri strips can coexist in the same array
		void MoveTo( ArrayVertex2& vertices, const Vertex2& p );

	protected:
		void ApplyTransform( ArrayVertex2& vertices, const Vertex2& origin );

	private:
		void SubdivideCircleSector( ArrayVertex2& vertices, const Vertex2& p1, const Vertex2& p2, int depth );

	protected:
		// Fill
		enum _Constants
		{
			kNoScale = 0x1,
		};

		void AppendCircle( ArrayVertex2& vertices, Real radius, U32 options );
		void AppendCircleQuadrants( ArrayVertex2& vertices, Real radius, U32 options );

		static void AppendRect( ArrayVertex2& vertices, Real halfW, Real halfH );

		static void MoveCenterToOrigin( ArrayVertex2& vertices, Vertex2 currentCenter );

	private:
		int fMaxSubdivideDepth;

	protected:
		Real fWidth;
};

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

#endif // _Rtt_Tesselator_H__
