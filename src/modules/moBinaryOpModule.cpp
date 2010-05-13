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
#include "moBinaryOpModule.h"

MODULE_DECLARE(BinaryOp, "mrvux", "Performs per element operation on two images");

moBinaryOpModule::moBinaryOpModule() : moModule(MO_MODULE_INPUT|MO_MODULE_OUTPUT, 2, 1) {
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

	this->properties["operation"] = new moProperty("add");
	this->properties["operation"]->setChoices("add;substract;absdiff;multiply;divide;min;max");
}

moBinaryOpModule::~moBinaryOpModule() 
{
	if (this->output_buffer)
	{	
		cvReleaseImage(&this->output_buffer);
	}
}

void moBinaryOpModule::notifyData(moDataStream *input) 
{
	IplImage* src = static_cast<IplImage*>(input->getData());
	assert( input->getFormat() == "IplImage" );
	if ( src == NULL )
		return;

	if ( this->output_buffer == NULL ) {
		this->output_buffer = cvCreateImage(cvGetSize(src), src->depth, src->nChannels);
	} else {
		if ( this->output_buffer->width != src->width ||
			 this->output_buffer->height != src->height ) {
			LOG(MO_CRITICAL, "cannot combine image with different size");
		}
	}

	this->notifyUpdate();
}

void moBinaryOpModule::update() {
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
		if (op == "add")
		{
			cvAdd(d1,d2,this->output_buffer);
		}
		else if (op == "absdiff")
		{
			cvAbsDiff(d1,d2,this->output_buffer);
		}
		else if (op == "substract")
		{
			cvSub(d1,d2,this->output_buffer);
		}
		else if (op == "multiply")
		{
			cvMul(d1,d2,this->output_buffer);
		}
		else if (op == "divide")
		{
			cvDiv(d1,d2,this->output_buffer);
		}
		else if (op == "min")
		{
			cvMin(d1,d2,this->output_buffer);
		}
		else if (op == "max")
		{
			cvMax(d1,d2,this->output_buffer);
		}

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

void moBinaryOpModule::setInput(moDataStream *stream, int n) {
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

moDataStream *moBinaryOpModule::getInput(int n) {
	if ( n == 0 )
		return this->input1;
	if ( n == 1 )
		return this->input2;

	this->setError("Invalid input index");
	return NULL;
}

moDataStream *moBinaryOpModule::getOutput(int n) {
	if ( n != 0 ) {
		this->setError("Invalid output index");
		return NULL;
	}
	return this->output;
}
