/***********************************************************************
 ** Copyright (C) 2010 Movid Authors.  All rights reserved.
 **
 ** This file is part of the Movid Software.
 **
 ** This file may be distributed under the terms of the Q Public License
 ** as defined by Trolltech AS of Norway and appearing in the file
 ** LICENSE included in the packaging of this file.
 **
 ** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 ** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 **
 ** Contact info@movid.org if any conditions of this licensing are
 ** not clear to you.
 **
 **********************************************************************/


#include <assert.h>
#include "moResizeModule.h"
#include "../moLog.h"
#include "cv.h"

MODULE_DECLARE(Resize, "mrvux", "Resizes an image to a specified size");

moResizeModule::moResizeModule() : moImageFilterModule(){

	MODULE_INIT();

	// declare properties
	this->properties["width"] = new moProperty(320);
	this->properties["width"]->setMin(1);
	this->properties["height"] = new moProperty(240);
	this->properties["height"]->setMin(1);
	this->properties["interpolation"] = new moProperty("bilinear");
	this->properties["interpolation"]->setChoices("nearestneigbour;bilinear;pixelarea;bicubic");
}

moResizeModule::~moResizeModule() {
}

int moResizeModule::toCvType(const std::string &filter) {
	if ( filter == "nearestneigbour" )
		return CV_MEDIAN;
	if ( filter == "bilinear" )
		return CV_GAUSSIAN;
	if ( filter == "pixelarea" )
		return CV_BLUR;
	if ( filter == "bicubic" )
		return CV_BLUR_NO_SCALE;

	LOGM(MO_ERROR, "Unsupported interpolation mode: " << filter);
	this->setError("Unsupported interpolation mode");
	return 0;
}

void moResizeModule::applyFilter(IplImage *src) 
{	
	CvSize size;
	size.width = this->property("width").asInteger();
	size.height = this->property("height").asInteger();

	if (!this->output_buffer)
	{
		this->output_buffer = cvCreateImage(size,src->depth,src->nChannels);
	}
	else
	{
		if (this->output_buffer->width != size.width || this->output_buffer->height != size.height)
		{
			cvReleaseImage(&this->output_buffer);
			this->output_buffer = cvCreateImage(size,src->depth,src->nChannels);
		}
	}

	if (this->output_buffer)
	{
		cvResize(src,this->output_buffer,this->toCvType(this->property("interpolation").asString()));
	}
}


