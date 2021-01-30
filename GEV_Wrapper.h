#pragma once
#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include <tuple>

struct PCameraParam
{
	Spinnaker::CameraPtr pCam;
	Spinnaker::ImagePtr pImg;
	cv::Mat map1;
	cv::Mat map2;
	cv::Mat dstImg;
};

class GEV_Drive {
public:
	// GEVカメラオブジェクトの初期化
	// グローバル変数として一度だけ宣言する
	// (I) const std::string Input_cameraID[]     カメラの順番を記した配列
	GEV_Drive(const std::string Input_cameraID[]) :cameraID(Input_cameraID) {};
	// GEVカメラオブジェクトのデストラクタ
	// 関数のスコープを抜ける際に呼ばれる
	~GEV_Drive();
	// 繋がっているカメラすべてを初期化
	void Init();
	// カメラ画像の取得開始
	void BeginAcquisition();
	// カメラ画像の取得終了
	void EndAcquisition();
	// 繋がっているカメラすべての画像を取得
	void operator>>(std::vector<cv::Mat> &dstFrame);
private:
	const std::string *cameraID;
	const std::string intrinsicsFile = "intrinsics.xml";
	Spinnaker::SystemPtr system;
	Spinnaker::CameraList camList;
	Spinnaker::CameraPtr pCam;
	cv::Mat map1, map2;
};

// 歪み補正用のmapをカメラパラメータを読み込んで作成する関数
// ※注意！returnValの形式はtuple型,構造化束縛により要素を分割して取り出す
// (I) const string intrinsicsFile  カメラパラメータが記録されているxmlファイル
// returnVal cv::Mat map1, map2     歪み補正用map
std::tuple<cv::Mat, cv::Mat>initMap(const std::string intrinsicsFile);

// カメラの設定を行う関数
// (I) INodeMap nodeMap 
// returnval:result(int) 終了状態
int ConfigureCustomImageSetting(Spinnaker::GenApi::INodeMap& nodeMap);

// カメラの初期化を行う関数
// 繋がっているカメラすべてを初期化する
//
// (I) SystemPtr&  system   システムのシングルトン
// (I) CameraList& camList  グローバルなカメラリスト
// (I) string      cameraID カメラID（カメラの順番を固定するリスト） 
// returnVal:result(int) 終了状態
int InitCameras(Spinnaker::SystemPtr& system, Spinnaker::CameraList& camList, const std::string cameraID[]);

// カメラの終了処理
// 繋がっているカメラすべてを終了
//
// (I) SystemPtr&  system  システムのシングルトン
// (I) CameraList& camList グローバルなカメラリスト
void DeinitCameras(Spinnaker::SystemPtr& system, Spinnaker::CameraList &camList);

// カメラ画像の取得(マルチスレッド用)
// lpParamの構造体からカメラポインタとカメラ画像ポインタを取得し、
// 取得したカメラ画像をカメラ画像ポインタに書き込む
// (I)lpParam カメラポインタと画像ポインタが格納されている構造体
// returVal : (int) - 1 正常終了
//                  - 0 例外発生
DWORD WINAPI AcquireImage(LPVOID lpParam);

// すべてのカメラ画像をcameraID順に取得する
// cameraID配列に記述した順番で、カメラ画像を取得し、
// vector<cv::Mat>に格納
//
// (I) CameraList& camList      グローバルなカメラリスト
// (I) const string cameraID    カメラID（カメラの順番を固定するリスト）
// (I) cv::Mat map1, map2       歪み補正用マップ
std::vector<cv::Mat> AquireMultiCamImagesMT(Spinnaker::CameraList &camList, const std::string cameraID[], cv::Mat map1, cv::Mat map2);

void ShowAquiredImages(std::vector<cv::Mat> Frames);