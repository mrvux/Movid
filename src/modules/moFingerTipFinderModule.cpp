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

/*
 * This module can be used to detect a hand in an image.
 * The output is binary image showing the hand's outline, the fingertips (as
 * white circles) and the center of the hand (white rectangle).
 * The following steps are performed:
 *		1. The contours of the segmented hand are found.
 *		   If the area of the contour is not above a certain threshold, the
 *		   contour can be discarded.
 *		2. The convex hull of the contour is determined and convexity defects
 *		   calculated. The defects that are more than a given threshold
 *		   away from the convex hull are considered. Their start and endpoint
 *		   is interpreted as fingertips.
 *		   Since there are 5 fingers visible at max, we consider only the
 *		   strongest defects, of which we only take 5 at a max. (1 def = 2 points)
 *		3. Since there could (should) be double-detections of the index, middle
 *		   and ring finger, we merge fingertips that are in very close proximity
 *		   of each other.
 */


#include <assert.h>
#include "moFingerTipFinderModule.h"
#include "../moLog.h"
#include "cv.h"

// XXX This is currently rather the scratchspace for a fingertip finder...
MODULE_DECLARE(FingerTipFinder, "native", "Module capable of detecting hands in an image. Detection is based on color-segmentation, contour-shape and distance transform. Finds fingertips & centerpoint of the palm.");

typedef std::pair<float, CvConvexityDefect*> depthToDefect;

moFingerTipFinderModule::moFingerTipFinderModule() : moImageFilterModule(){

	MODULE_INIT();

	this->storage = cvCreateMemStorage(0);
	this->properties["min_distance"] = new moProperty(20.);
	this->properties["min_area"] = new moProperty(150.);
	this->properties["merge_distance"] = new moProperty(10.);
}

moFingerTipFinderModule::~moFingerTipFinderModule() {
	cvReleaseMemStorage(&this->storage);
}

bool _sort_pred(const depthToDefect &left, const depthToDefect &right) {
	return left.first > right.first;
}

bool _in2(std::vector<int> &vec, int e) {
	for (unsigned int i = 0; i < vec.size(); i++) {
		if (vec[i] == e) return true;
	}
	return false;
}

void moFingerTipFinderModule::applyFilter(IplImage *src) {
	// Create a copy since cvFindContours will manipulate its input
	cvCopy(src, this->output_buffer);
	CvSeq *contours = 0;
	cvFindContours(this->output_buffer, this->storage, &contours, sizeof(CvContour), CV_RETR_CCOMP);
	cvZero(this->output_buffer);
	if(contours) {
		// Find the exterior contour (i.e. the hand has to be white) that has the greatest area
		CvSeq *max_cont = contours, *cur_cont = contours;
		double area, max_area = 0;
		while (cur_cont != 0) {
			area = cvContourArea(cur_cont);
			area *= area;
			if (area >= max_area) {
				max_area = area;
				max_cont = cur_cont;
			}
			cur_cont = cur_cont->h_next;
		}
		// Ignore contours whose area is too small, they're likely not hands anyway
		if (max_area < this->property("min_area").asDouble())
			return;
		contours = max_cont;

		cvDrawContours(this->output_buffer, contours, cvScalarAll(255), cvScalarAll(255), 100);

		// Compute the convex hull of the contour
		CvSeq* hull = 0;
		hull = cvConvexHull2(contours, contours->storage, CV_CLOCKWISE, 0);

		// Compute the convexity defects of the convex contour with respect to the convex hull.
		// The fingertips are at the start and endpoints of the defects
		CvSeq* defects = 0;
		defects = cvConvexityDefects(contours, hull, contours->storage);

		// Sort the defects based on their distance from the convex hull
		std::vector<depthToDefect> def_depths;
		for(int i=0; i < defects->total; ++i) {
			CvConvexityDefect* defect = (CvConvexityDefect*)cvGetSeqElem (defects, i);
			def_depths.push_back(depthToDefect(defect->depth, defect));
		}
		std::sort(def_depths.begin(), def_depths.end(), _sort_pred);

		// Find the start and end points of the 5 best defects
		std::vector<CvPoint*> points;
		for (unsigned int i=0; i < def_depths.size(); i++) {
			// 5 Fingers max
			if (i == 5)
				break;
			float depth = def_depths[i].first;
			double min_dist = this->property("min_distance").asDouble();
			if (min_dist > depth)
				continue;
			CvConvexityDefect *defect = def_depths[i].second;
			points.push_back(defect->start);
			points.push_back(defect->end);
		}

		// Merge almost coinciding points
		double distance;
		double merge_distance = this->property("merge_distance").asDouble();
		CvPoint *p1, *p2;
		std::vector<int> suppressed;
		for (unsigned int i = 0; i < points.size(); i++) {
			p1 = points[i];
			for (unsigned int j = i+1; j < points.size(); j++) {
				if (i == j) continue;
				if (_in2(suppressed, j)) continue;
				p2 = points[j];
				distance = sqrt(pow(p1->x - p2->x, 2.0) + pow(p1->y - p2->y, 2.0));
				if (distance <= merge_distance) {
					suppressed.push_back(j);
				}
			}
		}
		std::vector<CvPoint*> good_points;
		for (unsigned int i = 0; i < points.size(); i++) {
			if (!_in2(suppressed, i)) good_points.push_back(points[i]);
		}

		// Draw the points
		CvPoint *p;
		for (unsigned int i = 0; i < good_points.size(); i++) {
			p = good_points[i];
			int radius = cvRound(10);
			cvCircle(this->output_buffer, *p, radius, CV_RGB(255, 255, 255), -1);
		}
	}
}

