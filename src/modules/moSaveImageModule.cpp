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


#include <sstream>
#include <assert.h>

#include "moSaveImageModule.h"
#include "../moLog.h"
#include "../moDataStream.h"

MODULE_DECLARE(SaveImage, "mrvux", "Saves an image to disk");

moSaveImageModule::moSaveImageModule() : moModule(MO_MODULE_INPUT, 1, 0) {

	MODULE_INIT();

	this->input = NULL;

	// declare inputs
	this->input_infos[0] = new moDataStreamInfo("image", "IplImage", "Image");

	// declare properties
	this->properties["path"] = new moProperty("image.bmp");
	this->properties["dosave"] = new moProperty(false);
}

moSaveImageModule::~moSaveImageModule(){

}


void moSaveImageModule::notifyData(moDataStream *input) 
{
	assert( input != NULL );
	assert( input == this->input );

	this->notifyUpdate();
}

void moSaveImageModule::setInput(moDataStream *stream, int n) {
	if ( n != 0 ) {
		this->setError("Invalid input index");
		return;
	}

	if ( this->input != NULL )
		this->input->removeObserver(this);

	this->input = stream;
	if ( stream->getFormat() != "IplImage" ) {
		this->setError("Input 0 accept only IplImage");
		this->input = NULL;
		return;
	}

	if ( this->input != NULL )
		this->input->addObserver(this);
}

moDataStream* moSaveImageModule::getInput(int n) {
	if ( n != 0 ) {
		this->setError("Invalid input index");
		return NULL;
	}
	return this->input;
}

moDataStream* moSaveImageModule::getOutput(int n) {
	this->setError("no output supported");
	return NULL;
}

void moSaveImageModule::update() 
{
	IplImage *d1 = NULL;
	if ( this->input == NULL)
		return;

	this->input->lock();
	d1 = (IplImage *)this->input->getData();
	if ( d1 == NULL ) {
		this->input->unlock();
		return;
	}

	d1 = cvCloneImage(d1);
	this->input->unlock();

	if (this->property("dosave").asBool())
	{	
		cvSaveImage(this->property("path").asString().c_str(),d1);
		this->property("dosave").set(false);
	}

	cvReleaseImage(&d1);
}


