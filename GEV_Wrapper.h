#pragma once
#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"
#include <opencv2/opencv.hpp>
#include <vector>

struct PCameraParam
{
	Spinnaker::CameraPtr pCam;
	Spinnaker::ImagePtr pImg;
	cv::Mat dstImg;
};

// カメラの設定を行う関数
// (I) INodeMap nodeMap 
// returnval:result(int) 終了状態
int ConfigureCustomImageSetting(Spinnaker::GenApi::INodeMap& nodeMap);

// カメラの初期化を行う関数
// 繋がっているカメラすべてを初期化する
//
// (I) SystemPtr& system システムのシングルトン
// returnVal:result(int) 終了状態
int InitCameras(Spinnaker::SystemPtr& system, Spinnaker::CameraList &camList);

// カメラの終了処理
// 繋がっているカメラすべてを終了
//
// (I) SystemPtr& system システムのシングルトン
void DeinitCameras(Spinnaker::SystemPtr& system, Spinnaker::CameraList &camList);

// カメラ画像の取得(マルチスレッド用)
// lpParamの構造体からカメラポインタとカメラ画像ポインタを取得し、
// 取得したカメラ画像をカメラ画像ポインタに書き込む
// (I)lpParam カメラポインタと画像ポインタが格納されている構造体
// returVal : (int) - 1 正常終了
//                  - 0 例外発生
DWORD WINAPI AcquireImage(LPVOID lpParam);

std::vector<cv::Mat> AquireMultiCamImagesMT(Spinnaker::CameraList &camList);

void ShowAquiredImages(std::vector<cv::Mat> Frames);