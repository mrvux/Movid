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


#include "moSampleHoldModule.h"
#include "../moLog.h"
#include "cv.h"

MODULE_DECLARE(SampleHold, "mrvux", "Keeps a sample of the image when we send a signal");

moSampleHoldModule::moSampleHoldModule() 
{
	this->bFirstframe = true;
	MODULE_INIT();
	this->properties["dohold"] = new moProperty(false);
}

moSampleHoldModule::~moSampleHoldModule() {
}

void moSampleHoldModule::applyFilter(IplImage *src) 
{
	if (this->bFirstframe)
	{
		cvZero(this->output_buffer);
		this->bFirstframe = false;
	}

	if (this->property("dohold").asBool())
	{
		cvCopy(src,this->output_buffer);
		this->property("dohold").set(false);
	}
}


