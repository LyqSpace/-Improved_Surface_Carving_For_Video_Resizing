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
				  int layerLimit, int &widthDeleted, int bandWidthDefault, int badCutLimit, int frameStId,
				  int frameEdId, int threadsNum, int blockSize, int resizeType ) {

	if ( widthDeleted == 0 ) return;

	cout << endl;
	if ( resizeType ) {
		cout << " Extend Video --ING" << endl;
	} else {
		cout << " Shrink Video --ING" << endl;
	}
	cout << " Produce " << widthDeleted << " Cut" << endl;

	//vector<int> edgeHead;
	//vector<typeEdge> edge;

	vector< vector<Mat> > pyramidFrames;
	vector< vector<Mat> > pyramidPixelEnergy;
	vector< vector<Mat> > pyramidEdgeProtect;

	Mat linkHead = Mat_<int>( frames.size(), frames[0].rows, -1 );
	vector<typeLink> link;
	Mat removePts;
	int surfaceDeletedCount = 0;

   	int cutCriterion = generalCrop( pixelEnergy, edgeProtect, widthDeleted );

	int cutAlert = 0;
	bool isBuildPyramid = true;
	int bandLeft, bandWidth;

	while ( surfaceDeletedCount < widthDeleted ) {

		int surfaceTime = clock();

		surfaceDeletedCount++;
		//cout << "\n Cut " << surfaceDeletedCount << endl;

		if ( (surfaceDeletedCount - 1) % 5 != 0 ) {
			isBuildPyramid = false;
		} else {
			isBuildPyramid = true;
		}

		buildPyramid( pyramidFrames, pyramidPixelEnergy, pyramidEdgeProtect, frames, pixelEnergy, edgeProtect, layerLimit, isBuildPyramid );

		if ( isBuildPyramid ) {
			bandLeft = 0;
			bandWidth = pyramidFrames[layerLimit - 1][0].cols;
			bandWidthDefault = min( bandWidthDefault, bandWidth );
		}

		for ( int pyramidIndex = layerLimit - 1; pyramidIndex >= 0; pyramidIndex-- ) {

			if ( pyramidIndex > 0 && !isBuildPyramid ) continue;

			//vector<int> num2pos;
			int frameCount = pyramidFrames[pyramidIndex].size();
			Size frameSize = pyramidFrames[pyramidIndex][0].size();
			Grid* grid = new Grid( bandWidth, frameSize.height, frameCount, threadsNum, blockSize );
			bandLeft = min( bandLeft, frameSize.width - bandWidth );

			//buildGraph( pyramidFrames[pyramidIndex], pyramidPixelEnergy[pyramidIndex], pyramidEdgeProtect[pyramidIndex],
			//			bandLeft, bandWidth, num2pos, edgeHead, edge );
			//int clock3 = clock();
			//cout << "3-2 " << clock3 - clock2 << endl;
			buildGraph_Grid( bandWidth, frameSize.height, frameCount, pyramidPixelEnergy[pyramidIndex], pyramidEdgeProtect[pyramidIndex],
							 bandLeft, bandWidth, grid );
			//int cutEvaluate = maxFlow( edgeHead, edge );
			//int clock4 = clock();
			//cout << "4-3 " << clock4 - clock3 << endl;
			grid->compute_maxflow();
			//int clock5 = clock();
			//cout << "5-4 " << clock5 - clock4 << endl;
			int cutEvaluate = grid->get_flow();
			//cout << cutEvaluate << endl;
			if ( pyramidIndex == 0 ) {
				//cout << " Cut / Terminate Evaluate : " << cutEvaluate << " / " << cutCriterion << endl;
				if ( cutEvaluate > cutCriterion ) cutAlert++;
				if ( cutAlert >= badCutLimit ) {
					grid->delete_cap();
					delete grid;
					break;
				}
			}

			//removePts = vector< vector<int> >( frameCount, vector<int>( frameSize.height, 0 ) );
			removePts = Mat_<int>( frameCount, frameSize.height, 0 );
			//calcSurfaceBand( frameCount, frameSize, num2pos, edgeHead, edge, removePts );
			calcSurfaceBand_Grid( frameCount, frameSize.height, bandWidth, bandLeft, grid, removePts );
			//int clock6 = clock();
			//cout << "6-5 " << clock6 - clock5 << endl;
			if ( pyramidIndex > 0 ) {
				bandWidth = bandWidthDefault;
				settleBand( removePts, bandLeft, bandWidth, frameSize.height, frameCount );
			}
			grid->delete_cap();
			delete grid;
			//clock7 = clock();
			//cout << "7-6 " << clock7 - clock6 << endl;
		}

		if ( cutAlert >= badCutLimit ) break;

		surfaceCarving( frames, pixelEnergy, edgeProtect, removePts, linkHead, link );
		//surfaceTime = clock() - surfaceTime;
		//surfaceTime = (surfaceTime + 500) / 1000;
		//printf( " Time Used : %d min %d sec\n", surfaceTime / 60, surfaceTime % 60 );
	}

	if ( cutAlert >= badCutLimit ) surfaceDeletedCount--;

	widthDeleted = widthDeleted - surfaceDeletedCount;
	saveFrame( keyFrame, resizeType, surfaceDeletedCount, frames, linkHead, link, frameStId, frameEdId );

	cout << endl;

}

void scaleVideo( vector<int> &keyFrame, vector<Mat> &frames, vector<Mat> &pixelEnergy,
				 int &len, int frameStId, int frameEdId, int resizeType ) {

	if ( resizeType ) {
		cout << " Normally Scale Out Video --ING" << endl;
	} else {
		cout << " Normally Scale In Video --ING" << endl;
	}

	Mat mat;
	Size frameSize = frames[0].size();
	char pngName[100];

	if ( resizeType ) {

		for ( int i = frameStId; i < frameEdId; i++ ) {

			sprintf( pngName, "extendResult//%d.png", i );
			mat = imread( pngName );
			resize( mat, mat, Size( frameSize.width + len * 2, frameSize.height ) );
			sprintf( pngName, "tempMat//%d.png", i );
			imwrite( pngName, mat );
		}

		len = 0;

	} else {

		int height = (int)((double)frameSize.height * (frameSize.width - len) / frameSize.width);
		if ( (frameSize.height & 1) ^ (height & 1) ) height++;

		for ( int i = frameStId; i < frameEdId; i++ ) {

			sprintf( pngName, "shrinkResult//%d.png", i );
			mat = imread( pngName );
			if ( mat.empty() ) break;

			resize( mat, mat, Size( frameSize.width - len, height ) );
			sprintf( pngName, "tempMat//%d.png", i );
			imwrite( pngName, mat );
		}

		int frameCount = frames.size();
		for ( int j = 0; j < frameCount; j++ ) {

			resize( frames[j], frames[j], Size( frameSize.width - len, height ) );
			resize( pixelEnergy[j], pixelEnergy[j], Size( frameSize.width - len, height ) );

		}

		len = (frameSize.height - height) / 2;
	}
}

void rotateVideo( vector<int> &keyFrame, vector<Mat> &frames, vector<Mat> &pixelEnergy,
				  int frameStId, int frameEdId, int fileType ) {

	cout << " Rotate Video --ING" << endl;

	Size frameSize;
	char pngName[100];
	Mat rotateFrame, originFrame;

	int n = keyFrame.size() - 1;
	int t = 0;
	for ( int i = frameStId; i < frameEdId; i++ ) {

		sprintf( pngName, "tempMat//%d.png", i );
		originFrame = imread( pngName );

		if ( t < n && (i - frameStId) == keyFrame[t + 1] ) t++;

		frameSize = originFrame.size();
		rotateFrame = Mat( frameSize.width, frameSize.height, CV_8UC3 );
		for ( int y = 0; y < frameSize.height; y++ ) {
			for ( int x = 0; x < frameSize.width; x++ ) {
				rotateFrame.ptr<Vec3b>( x )[y] = originFrame.ptr<Vec3b>( y )[x];
			}
		}

		if ( (i - frameStId) == keyFrame[t] ) frames[t] = rotateFrame.clone();

		if ( fileType ) {
			sprintf( pngName, "output//%d.png", i );
		} else {
			sprintf( pngName, "tempMat//%d.png", i );
		}

		imwrite( pngName, rotateFrame );

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