#include "GEV_Wrapper.h"
#include <string>
#include <iostream>
using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;
using namespace std;

CameraList camList;


// カメラの順番を固定する配列
// カメラのシリアルIDを入力することで、
// 配列に記述した順番でカメラ映像を取得できる
const string cameraID[15] = {
	
	//"18551299",
	//"18464590",
	//"18464423",
	//"18551296",
	//"18509956",

	//"18509958",
	//"18408232",
	//"18509333",
	//"18465007",
	//"18509955",
	
	"18464424",
	"18464589",
	"18509340",
	"18464421",
	//"18551294",
};

int main() {
	int result = 0;
	// 構造化束縛により変数を取りだす
	auto[ map1, map2 ] = initMap("intrinsics.xml");

	/****************
	* 初期化処理
	*****************/
	// システムオブジェクトのシングルトン参照を取得
	SystemPtr system = System::GetInstance();
	// カメラの初期化を行う
	InitCameras(system, camList, cameraID);
	/****************
	*****************/

	// カメラへのshared pointerを生成する
	CameraPtr pCam = nullptr;
	try {
		for (int i = 0; i < camList.GetSize(); i++) {
			pCam = camList.GetBySerial(cameraID[i]);
			pCam->BeginAcquisition();
		}
		bool lp_break = true;
		while (lp_break) {
			vector<cv::Mat> Frames(AquireMultiCamImagesMT(camList, cameraID, map1, map2));
			ShowAquiredImages(Frames);
			if (cv::waitKey(1) == 'c')lp_break = false;
		}
		for (int i = 0; i < camList.GetSize(); i++) {
			pCam = camList.GetByIndex(i);
			pCam->EndAcquisition();
		}
	}
	catch (Spinnaker::Exception& e) {
		cout << "Error: " << e.what() << endl;
		result = -1;
	}


	/**************
	* 終了処理
	***************/
	//カメラの初期化を解除
	pCam->DeInit();
	// カメラへのポインタを開放
	pCam = nullptr;
	// カメラの終了処理
	DeinitCameras(system,camList);
	/**************
	***************/
	return result;

}