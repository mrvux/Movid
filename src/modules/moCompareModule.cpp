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

#include "../moLog.h"
#include "../moModule.h"
#include "../moDataStream.h"
#include "moCompareModule.h"

MODULE_DECLARE(Compare, "mrvux", "Performs comparison operation on two images");

moCompareModule::moCompareModule() : moModule(MO_MODULE_INPUT|MO_MODULE_OUTPUT, 2, 1) {
	MODULE_INIT();

	this->input1 = NULL;
	this->input2 = NULL;
	this->output = new moDataStream("IplImage");
	this->output_buffer = NULL;

	// declare outputs
	this->input_infos[0] = new moDataStreamInfo(
			"image1", "IplImage", "First Image");
	this->input_infos[1] = new moDataStreamInfo(
			"image2", "IplImage", "Second Image");
	this->output_infos[0] = new moDataStreamInfo(
			"combine", "IplImage", "Operation Result");

	this->properties["operation"] = new moProperty("equal");
	this->properties["operation"]->setChoices("equal;greater;greaterequal;lower;lowerequal;notequal");
}

moCompareModule::~moCompareModule() 
{
	if (this->output_buffer)
	{	
		cvReleaseImage(&this->output_buffer);
	}
}

void moCompareModule::notifyData(moDataStream *input) 
{
	IplImage* src = static_cast<IplImage*>(input->getData());
	assert( input->getFormat() == "IplImage" );
	if ( src == NULL )
		return;

	if ( this->output_buffer == NULL ) {
		this->output_buffer = cvCreateImage(cvGetSize(src), IPL_DEPTH_8U, src->nChannels);
	} else {
		if ( this->output_buffer->width != src->width ||
			 this->output_buffer->height != src->height ) {
			LOG(MO_CRITICAL, "cannot compare image with different size");
		}
	}

	this->notifyUpdate();
}

int moCompareModule::getCvOperation(const std::string &op) 
{
	if ( op == "equal" )
		return CV_CMP_EQ;
	if ( op == "greater" )
		return CV_CMP_GT;
	if ( op == "greaterequal" )
		return CV_CMP_GE;
	if ( op == "lower" )
		return CV_CMP_LT;
	if ( op == "lowerequal" )
		return CV_CMP_LE;
	if ( op == "notequal" )
		return CV_CMP_NE;

	LOGM(MO_ERROR,"Unsupported operation");
	this->setError("Unsupported operation");
	return 0;
}

void moCompareModule::update() {
	IplImage *d1 = NULL, *d2 = NULL;
	if ( this->input1 == NULL || this->input2 == NULL || this->output_buffer == NULL )
		return;

	this->input1->lock();
	d1 = (IplImage *)this->input1->getData();
	if ( d1 == NULL ) 
	{
		this->input1->unlock();
		return;
	}
	d1 = cvCloneImage(d1);
	this->input1->unlock();

	this->input2->lock();
	d2 = (IplImage*)this->input2->getData();
	if ( d2 == NULL ) 
	{
		this->input2->unlock();
		if (d1)
		{ 
			cvReleaseImage(&d1);
		}
		return;
	}
	d2 = cvCloneImage(d2);
	this->input2->unlock();

	if (d1 && d2 && this->output_buffer)
	{
		//TODO: Use function pointer dictionary
		std::string op = this->property("operation").asString();

		cvCmp(d1,d2,this->output_buffer,this->getCvOperation(op));
		this->output->push(this->output_buffer);
	}

	if (d1)
	{
		cvReleaseImage(&d1);
	}
	if (d2)
	{
		cvReleaseImage(&d2);
	}
}

void moCompareModule::setInput(moDataStream *stream, int n) {
	assert( n == 0 || n == 1 );

	if ( n == 0 ) {
		if ( this->input1 != NULL )
			this->input1->removeObserver(this);
		this->input1 = stream;
		if ( stream->getFormat() != "IplImage" ) {
			this->setError("Input 0 accept only IplImage");
			this->input1 = NULL;
			return;
		}
	} else {
		if ( this->input2 != NULL )
			this->input2->removeObserver(this);
		this->input2 = stream;
		if ( stream->getFormat() != "IplImage" ) {
			this->setError("Input 1 accept only IplImage");
			this->input2 = NULL;
			return;
		}
	}

	if ( stream != NULL )
		stream->addObserver(this);
}

moDataStream *moCompareModule::getInput(int n) {
	if ( n == 0 )
		return this->input1;
	if ( n == 1 )
		return this->input2;

	this->setError("Invalid input index");
	return NULL;
}

moDataStream *moCompareModule::getOutput(int n) {
	if ( n != 0 ) {
		this->setError("Invalid output index");
		return NULL;
	}
	return this->output;
}
