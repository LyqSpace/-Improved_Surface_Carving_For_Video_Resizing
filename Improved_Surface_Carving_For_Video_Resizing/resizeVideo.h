/*
*	Copyright (C)   Lyq root#lyq.me
*	File Name     : resizeVideo.h
*	Creation Time : 2015-5-29
*	Environment   : Windows8.1-64bit VS2013 OpenCV2.4.9
*	Homepage      : http://www.lyq.me
*/

#ifndef RESIZE_VIDEO
#define RESIZE_VIDEO

#include "baseFunction.h"

void resizeVideo( vector<int> &keyFrame, vector<Mat> &frames, vector<Mat> &pixelEnergy, vector<Mat> &edgeProtect, 
				  int layerLimit, int &widthDeleted, int bandWidthDefault, int badCutLimit, int resizeType ) {

	cout << endl;
	if ( resizeType ) {
		cout << " Extend Video --ING" << endl;
	} else {
		cout << " Shrink Video --ING" << endl;
	}
	cout << " Produce " << widthDeleted << " Cut" << endl;

	vector<int> edgeHead;
	vector<typeEdge> edge;
	vector< vector<int> > removePts;

	vector< vector<Mat> > pyramidFrames;
	vector< vector<Mat> > pyramidPixelEnergy;
	vector< vector<Mat> > pyramidEdgeProtect;

	vector< vector< int > > linkHead = vector< vector<int> >( frames.size(), vector<int>( frames[0].rows, -1 ) );
	vector<typeLink> link;

	int surfaceDeletedCount = 0;

	int cutCriterion = generalCrop( pixelEnergy, edgeProtect, widthDeleted );

	int cutAlert = 0;

	while ( surfaceDeletedCount < widthDeleted ) {

		int surfaceTime = clock();

		surfaceDeletedCount++;
		cout << "\n Cut " << surfaceDeletedCount << endl;

		buildPyramid( pyramidFrames, pyramidPixelEnergy, pyramidEdgeProtect, frames, pixelEnergy, edgeProtect, layerLimit );

		int bandLeft = 0;
		int bandWidth = pyramidFrames[layerLimit - 1][0].cols;
		bandWidthDefault = min( bandWidthDefault, bandWidth );

		for ( int pyramidIndex = layerLimit - 1; pyramidIndex >= 0; pyramidIndex-- ) {

			//cout << " >> In pyramid Index " << pyramidIndex << endl;
			//int pyramidTime = clock();

			vector<int> num2pos;

			buildGraph( pyramidFrames[pyramidIndex], pyramidPixelEnergy[pyramidIndex], pyramidEdgeProtect[pyramidIndex],
						bandLeft, bandWidth, num2pos, edgeHead, edge );

			int cutEvaluate = maxFlow( edgeHead, edge );
			//if ( surfaceDeletedCount == 3 ) {cutAlert = badCutLimit; break;}
			if ( pyramidIndex == 0 ) {
				cout << " Cut / Terminate Evaluate : " << cutEvaluate << " / " << cutCriterion << endl;
				if ( cutEvaluate > cutCriterion ) cutAlert++;
				if ( cutAlert >= badCutLimit ) break;
			}

			int pyramidFrameCount = pyramidFrames[pyramidIndex].size();
			Size pyramidFrameSize = pyramidFrames[pyramidIndex][0].size();
			removePts = vector< vector<int> >( pyramidFrameCount, vector<int>( pyramidFrameSize.height, 0 ) );
			calcSurfaceBand( pyramidFrames[pyramidIndex], num2pos, edgeHead, edge, removePts );

			bandWidth = bandWidthDefault;
			settleBand( removePts, bandLeft, bandWidth, pyramidFrameSize, pyramidFrameCount );

			//pyramidTime = clock() - pyramidTime;
			//pyramidTime = (pyramidTime + 500) / 1000;
			//printf( "  < In pyramid Time used : %d min %d sec\n", pyramidTime / 60, pyramidTime % 60 );
		}

		if ( cutAlert >= badCutLimit ) break;

		surfaceCarving( frames, pixelEnergy, edgeProtect, removePts, linkHead, link );

		surfaceTime = clock() - surfaceTime;
		surfaceTime = (surfaceTime + 500) / 1000;
		printf( " Time Used : %d min %d sec\n", surfaceTime / 60, surfaceTime % 60 );
	}
	
	if ( cutAlert >= badCutLimit ) surfaceDeletedCount--;
		
	widthDeleted = widthDeleted - surfaceDeletedCount;
	saveFrame( keyFrame, resizeType, surfaceDeletedCount, frames, linkHead, link );

	cout << endl;

}

void scaleVideo( vector<int> &keyFrame, vector<Mat> &frames, vector<Mat> &pixelEnergy, int &len ) {

	cout << " Normally Scale --ING" << endl;

	Mat mat;
	Size frameSize = frames[0].size();
	char pngName[100];
	int height = (int)((double)frameSize.height * (frameSize.width - len) / frameSize.width);
	if ( (frameSize.height & 1) ^ (height & 1) ) height++;

	int i = 0;
	
	while ( true ) {

		sprintf( pngName, "shrinkResult//%d.png", i );
		mat = imread( pngName );
		if ( mat.empty() ) break;

		resize( mat, mat, Size( frameSize.width - len, height ) );
		sprintf( pngName, "resizeResult//%d.png", i++ );
		imwrite( pngName, mat );
	}

	int frameCount = frames.size();
	for ( int j = 0; j < frameCount; j++ ) {
		
		resize( frames[j], frames[j], Size( frameSize.width - len, height ) );
		resize( pixelEnergy[j], pixelEnergy[j], Size( frameSize.width - len, height ) );

	}

	len = (frameSize.height - height) / 2;
}

void rotateVideo( vector<int> &keyFrame, vector<Mat> &frames, vector<Mat> &pixelEnergy, int fileType ) {

	cout << " Rotate Video --ING" << endl;

	Size frameSize = frames[0].size();
	char pngName[100];
	Mat rotateFrame = Mat( frameSize.width, frameSize.height, CV_8UC3 );
	Mat originFrame;

	int n = keyFrame.size();
	int i = 0;
	int t = 0;
	while ( true ) {

		if ( fileType ) {
			sprintf( pngName, "extendResult//%d.png", i );
		} else {
			sprintf( pngName, "resizeResult//%d.png", i );
		}
		originFrame = imread( pngName );
		if ( originFrame.empty() ) break;

		if ( t < n && i == keyFrame[t + 1] ) t++;

		for ( int y = 0; y < frameSize.height; y++ ) {
			for ( int x = 0; x < frameSize.width; x++ ) {
				rotateFrame.ptr<Vec3b>( x )[y] = originFrame.ptr<Vec3b>( y )[x];
			}
		}
		
		if ( i == keyFrame[t] ) frames[t] = rotateFrame.clone();

		if ( fileType ) {
			sprintf( pngName, "output//%d.png", i );
		} else {
			sprintf( pngName, "rotateResult//%d.png", i );
		}
		
		imwrite( pngName, rotateFrame );

		i++;
	}

	if ( fileType == 0 ) {

		int frameCount = pixelEnergy.size();
		frameSize = pixelEnergy[0].size();

		Mat rotateEnergy = Mat( frameSize.width, frameSize.height, CV_8UC1 );

		for ( int t = 0; t < frameCount; t++ ) {
			for ( int y = 0; y < frameSize.height; y++ ) {
				for ( int x = 0; x < frameSize.width; x++ ) {
					rotateEnergy.ptr<uchar>( x )[y] = pixelEnergy[t].ptr<uchar>( y )[x];					
				}
			}
			pixelEnergy[t] = rotateEnergy.clone();
		}
	}
}

#endif