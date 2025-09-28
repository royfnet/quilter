//
//  quilt.cpp
//  quilter
//
//  Created by Jeffrey Lomicka on 9/14/25.
//  Copyright © 2025 Jeffrey Lomicka. All rights reserved.
//
#include "math.h"
#include "quilt.hpp"

static bool IsSame( double f1, double f2)
{// Compare floats for equality, more or less, in inches
	return fabs( f1 - f2) < 0.000005;	// Could give more leeway
}

#ifndef MAXFLOAT
#define MAXFLOAT FLT_MAX
#endif

class draw
{
protected:
	double sOldx = MAXFLOAT;		// TODO rename this later
	double sOldy = MAXFLOAT;
	double iScaleFactor = 1.0;		// Scale from UI, needs to be set in constructor

public:
	void ResetWihtoutJumpStitch()
	{// Used between objects two avoid generating a jump stitch to subsequent object (I.E., from graph paper)
		sOldx = MAXFLOAT;
		sOldy = MAXFLOAT;
	}

	virtual const char* fileType() = 0;
	virtual void OpenFile( const char* filename) = 0;
	// Positions and distances are all in INCHES
	virtual void SewLine( double x1, double y1, double x2, double y2) = 0;
	virtual void CloseFile() = 0;
};

class drawIQP : public draw
{
	FILE* iqpFile = NULL;
	long iqpPairCountPos = 0;
	int iqpPairCount = 0;

public:
	void WriteInt( int i)
	{
		fwrite( &i, 4, 1, iqpFile);
	}

	void WriteFloat( double d)
	{
		float f = (float) d;
		fwrite( &f, 4, 1, iqpFile);
	}

	virtual const char* fileType() override
	{
		return ".iqp";
	}

	virtual void OpenFile( const char* name) override
	{// Name needs to be given without file type for now
		char scrap[ 256];
		snprintf( scrap, CountItems( scrap), "%s%s", name, fileType());
		iqpFile = fopen( scrap, "wb");
		Test( iqpFile);
		fwrite( "StitchV2        ", 16, 1, iqpFile);
		WriteInt( 0);
		WriteInt( 0);
		WriteInt( 4);
		WriteInt( strli( name));
		fwrite( name, strlen( name), 1, iqpFile);
		WriteInt( 7);
		iqpPairCountPos = ftell( iqpFile);
		WriteInt( 0);
	}

	virtual void CloseFile() override
	{
		if( iqpFile)
		{
			fseek( iqpFile, iqpPairCountPos, SEEK_SET);
			WriteInt( iqpPairCount*4);
			fseek( iqpFile, 0, SEEK_END);
			fclose( iqpFile);
			iqpFile = NULL;
			iqpPairCountPos = 0;
			iqpPairCount = 0;
		}
	}

	virtual void SewLine( double x1, double y1, double x2, double y2) override
	{
		bool needMove = !IsSame(sOldx, x1) || !IsSame( sOldy, y1);
		bool needJump = needMove && sOldx != MAXFLOAT;
		sOldx = x2;
		sOldy = y2;
	 
		double sf = iScaleFactor;
		x1 *= sf;
		y1 *= sf;
		x2 *= sf;
		y2 *= sf;

		if( needJump)
		{
			WriteFloat( 11000.0);
			WriteFloat( 11000.0);
			iqpPairCount++;
			WriteFloat( x1);
			WriteFloat( y1);
			iqpPairCount++;
		}
		WriteFloat( x2);
		WriteFloat( y2);
		iqpPairCount++;
	}

};

class drawSVG : public draw
{// We should add methods for starting new objects so that logoist can idenity separte characters in, for example, Hershey output
	FILE* svgFile = stdout;
	double svgOffsetX = 450;
	double svgOffsetY = 450;
	
public:

	virtual const char* fileType() override
	{
		return ".svg";	//Maybe better as .html?
	}

	virtual void OpenFile( const char* name) override
	{// Name needs to be given without file type for now
		if( name && name[0])
		{// Default to stdout if no name given
			char scrap[ 256];
			snprintf( scrap, CountItems( scrap), "%s%s", name, fileType());
			svgFile = fopen( scrap, "w");
			Test( svgFile);
		}
		fprintf( svgFile, "<svg width=\"1800\" height=\"1800\"><g id=\"Quilting\"><path d=\"\n");
	}

	virtual void CloseFile() override
	{
		fprintf( svgFile, "\" stroke=\"black\" stroke-width=\"1\" fill=\"none\" /></g>SVG not available.</svg>\n");
		if( svgFile != stdout)
		{// If we went to a file, close it
			fclose( svgFile);
			svgFile = stdout;
		}
	}

	virtual void SewLine( double x1, double y1, double x2, double y2) override
	{// Conver fron inches and center
	 // Needs to remember previous point, and not jump if not needed
		double oldx = 0;  // I'd set these to sOldx, but don't want the math to overflow
		double oldy = 0;
		
		bool needMove = !IsSame(sOldx, x1) || !IsSame( sOldy, y1);
		//bool needJump = needMove && sOldx != MAXFLOAT;
		if( needMove)
		{
			oldx = sOldx;
			oldy = sOldy;
		}
		sOldx = x2;
		sOldy = y2;
	 
		double sf = iScaleFactor;
		x1 *= sf;
		y1 *= sf;
		x2 *= sf;
		y2 *= sf;

	//		Based on a 2 inch center square

	// Scale again to SVG points, 0,0 is the middle in SVG format, and the Y axis is inverted
		sf = 90;

		x1 *= sf;
		y1 *= -sf;
		x2 *= sf;
		y2 *= -sf;
		if( needMove)
			fprintf( svgFile, "M%f %f ", x1+svgOffsetX, y1+svgOffsetY);
		fprintf( svgFile, "L%f %f\n", x2+svgOffsetX, y2+svgOffsetY);
	}

};

class drawPS : public draw
{// Draw in PostScript
	FILE* psFile = stdout;
	bool sShowJumps = false;		// Needs to come from consructor
	double sOldGrey = MAXFLOAT;
	bool sOldDashes = false;
	double sDrawColor = 0.0;
	bool sDrawDashes = false;
	bool sAnimate = false;

public:

	virtual const char* fileType() override
	{
		return ".ps";	//Maybe better as .html?
	}

	virtual void OpenFile( const char* name) override
	{// Name needs to be given without file type for now
		if( name && name[0])
		{// Default to stdout if no name given
			char scrap[ 256];
			snprintf( scrap, CountItems( scrap), "%s%s", name, fileType());
			psFile = fopen( scrap, "w");
			Test( psFile);
		}
		fprintf( psFile, "%%!PS\n");
	}

	virtual void CloseFile() override
	{
		fprintf( psFile, "showpage\n");

		if( psFile != stdout)
		{// If we went to a file, close it
			fclose( psFile);
			psFile = stdout;
		}
	}

	virtual void SewLine( double x1, double y1, double x2, double y2) override
	{// Conver fron inches and center
	 // Needs to remember previous point, and not jump if not needed
		double oldx = 0;  // I'd set these to sOldx, but don't want the math to overflow
		double oldy = 0;
		
		bool needMove = !IsSame(sOldx, x1) || !IsSame( sOldy, y1);
		bool needJump = needMove && sOldx != MAXFLOAT;
		if( needMove)
		{
			oldx = sOldx;
			oldy = sOldy;
			if( needJump && sShowJumps)
				fprintf( stderr, "Jump stitch from %f,%f to %f,%f\n", oldx, oldy, x1, y1);
		}
		sOldx = x2;
		sOldy = y2;
	 
		double sf = iScaleFactor;
		x1 *= sf;
		y1 *= sf;
		x2 *= sf;
		y2 *= sf;
		oldx *= sf;
		oldy *= sf;

// Scale again to Postscript points, and offset to origin in the middle of the page
		sf = 72.0;		// Inches to points

		x1 += 4.25;
		y1 += 5.5;
		x2 += 4.25;
		y2 += 5.5;
		oldx += 4.25;
		oldy += 5.5;

		x1 *= sf;
		y1 *= sf;
		x2 *= sf;
		y2 *= sf;
		oldx *= sf;
		oldy *= sf;

		if( sOldGrey != sDrawColor)
		{
			fprintf( psFile, "%3.2f setgray\n", sDrawColor);
			sOldGrey = sDrawColor;
		}
		if( sOldDashes != sDrawDashes)
		{
			fprintf( psFile, "%s setdash\n", sDrawDashes ? "[3] 0" : "[] 0");
			sOldDashes = sDrawDashes;
		}

		if( needJump && sShowJumps)
		{// Show jump stitch in dotted or grey line
			//printf( "currentgray 0.8 setgray %f %f moveto %f %f lineto stroke setgray\n", oldx, oldy, x1, y1);
			fprintf( psFile, "currentdash [3] 0 setdash %f %f moveto %f %f lineto stroke setdash\n", oldx, oldy, x1, y1);
		}
		// TBD do we ned this moveto in the needJump && sShowJumps case?
		fprintf( psFile, "%f %f moveto %f %f lineto stroke\n", x1, y1, x2, y2);
		if( sAnimate) fprintf( psFile, "copypage\n");
	}

};
