#include "GEV_Wrapper.h"
using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;
using namespace std;

CameraList camList;
int main() {
	int result = 0;

	/****************
	* 初期化処理
	*****************/
	// システムオブジェクトのシングルトン参照を取得
	SystemPtr system = System::GetInstance();
	// カメラの初期化を行う
	InitCameras(system, camList);
	/****************
	*****************/

	// カメラへのshared pointerを生成する
	CameraPtr pCam = nullptr;
	try {
		for (int i = 0; i < camList.GetSize(); i++) {
			pCam = camList.GetByIndex(i);
			pCam->BeginAcquisition();
		}
		bool lp_break = true;
		while (lp_break) {
			vector<cv::Mat> Frames(AquireMultiCamImagesMT(camList));
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