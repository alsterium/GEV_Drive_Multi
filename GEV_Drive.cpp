#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <sstream>

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;
using namespace std;

struct PCameraParam
{
public:
	CameraPtr pCam;
	ImagePtr pImg;
	cv::Mat dstImg;
};

CameraList camList;
// カメラの設定を行う関数
// (I) INodeMap nodeMap 
// returnval:result(int) 終了状態
int ConfigureCustomImageSetting(INodeMap& nodeMap) {
	int result = 0;
	cout << endl << endl << "*** CONFIGURING CUSTOM IMAGE SETTINGS ***" << endl << endl;

	try {
		// ピクセルモードをBayerRG8に設定する
		CEnumerationPtr ptrPixelFormat = nodeMap.GetNode("PixelFormat");
		if (IsAvailable(ptrPixelFormat) && IsWritable(ptrPixelFormat)) {
			CEnumEntryPtr ptrPixelFormatBayerRG8 = ptrPixelFormat->GetEntryByName("BayerRG8");
			if (IsAvailable(ptrPixelFormatBayerRG8) && IsReadable(ptrPixelFormatBayerRG8)) {
				int64_t pixelFormatBayerRG8 = ptrPixelFormatBayerRG8->GetValue();

				ptrPixelFormat->SetIntValue(pixelFormatBayerRG8);
				cout << "Pixel format set to " << ptrPixelFormat->GetCurrentEntry()->GetSymbolic() << "..." << endl;
			}
			else {
				cout << "Pixel format BayerRG8 not available..." << endl;
			}
		}
		else {
			cout << "Pixel format not available..." << endl;
		}

		//CFloatPtr ptrAcquisitionFrameRate = nodeMap.GetNode("AcquisitionFrameRate");
		//if (IsAvailable(ptrAcquisitionFrameRate) && IsWritable(ptrAcquisitionFrameRate)) {
		//	ptrAcquisitionFrameRate->SetValue(15.0);
		//	cout << "AcquisitionFrameRate set to" << ptrAcquisitionFrameRate->GetValue() << "..." << endl;
		//}
		//else {
		//	cout << "AcquisitionFrameRate not available..." << endl;
		//}

		// DeviceLinkThroughputLimitを設定（帯域を制限する）
		CIntegerPtr ptrDeviceLinkThroughputLimit = nodeMap.GetNode("DeviceLinkThroughputLimit");
		if (IsAvailable(ptrDeviceLinkThroughputLimit) && IsWritable(ptrDeviceLinkThroughputLimit)) {
			ptrDeviceLinkThroughputLimit->SetValue((int64_t)28200000);
			cout << "DeviceLinkThroughputLimit set to " << ptrDeviceLinkThroughputLimit->GetValue() << "..." << endl;
		}
		else {
			cout << "DeviceLinkThroughputLimit not available..." << endl;
		}

		// 列挙ノードをnodeMapから取得
		CEnumerationPtr ptrAcquisitionMode = nodeMap.GetNode("AcquisitionMode");
		if (IsAvailable(ptrAcquisitionMode) && IsWritable(ptrAcquisitionMode)) {
			CEnumEntryPtr ptrAcquisitionModeContinuous = ptrAcquisitionMode->GetEntryByName("Continuous");
			if (IsAvailable(ptrAcquisitionModeContinuous) && IsReadable(ptrAcquisitionModeContinuous)) {
				const int64_t acquisitonModeContinuous = ptrAcquisitionModeContinuous->GetValue();

				ptrAcquisitionMode->SetIntValue(acquisitonModeContinuous);
				cout << "Acquisition mode set to continuous..." << endl;
			}
			else {
				cout << "Unable to set acquisition mode to continuous (entry retrieval). Aborting..." << endl << endl;
			}
		}
		else {
			cout << "Unable to set acquisition mode to continuous (enum retrieval). Aborting..." << endl << endl;
		}
	}
	catch (Spinnaker::Exception& e) {
		cout << "Error: " << e.what() << endl;
		result = -1;
	}

	return result;
}

// カメラの初期化を行う関数
// 繋がっているカメラすべてを初期化する
//
// (I) SystemPtr& system システムのシングルトン
// returnVal:result(int) 終了状態
int InitCameras(SystemPtr& system) {

	// ライブラリバージョンを出力
	const LibraryVersion spinnakerLibraryVersion = system->GetLibraryVersion();
	cout << "Spinnaker library version: " << spinnakerLibraryVersion.major << "." << spinnakerLibraryVersion.minor
		<< "." << spinnakerLibraryVersion.type << "." << spinnakerLibraryVersion.build << endl
		<< endl;

	// カメラのリストをシステムオブジェクトから取得する
	camList = system->GetCameras();

	// カメラの台数
	const unsigned int numCameras = camList.GetSize();

	cout << "Number of cameras detected: " << numCameras << endl << endl;

	// カメラが0台の時、終了させる
	if (numCameras == 0) {
		camList.Clear();
		system->ReleaseInstance();
		cout << "Not enough cameras!" << endl;
		cout << "Done! Press Enter to exit..." << endl;
		getchar();

		return -1;
	}

	CameraPtr pCam = nullptr;
	int result = 0;
	int err = 0;
	try {
		for (int i = 0; i < camList.GetSize(); i++) {
			pCam = camList.GetByIndex(i);
			// TLデバイスノードマップとGenICamノードマップを取得
			INodeMap& nodeMapTLDevice = pCam->GetTLDeviceNodeMap();
			// カメラを初期化
			pCam->Init();
			INodeMap& nodeMap = pCam->GetNodeMap();

			// カメラの画像設定を行う
			err = ConfigureCustomImageSetting(nodeMap);
			if (err < 0)
				result = -1;
		}
	}
	catch (Spinnaker::Exception& e) {
		cout << "Error: " << e.what() << endl;
	}
	return result;
}

// カメラの終了処理
// 繋がっているカメラすべてを終了
//
// (I) SystemPtr& system システムのシングルトン
void DeinitCameras(SystemPtr& system) {
	// システムを開放する前にカメラリストを開放
	camList.Clear();
	// システムを開放
	system->ReleaseInstance();

	cout << endl << "Done! Press Enter to exit..." << endl;
	getchar();
}

// カメラ画像の取得
// pCamのカメラ画像を取得し、カメラ画像を返却する
// (I) CameraPtr pCam カメラへのポインタ
// returnVal: cv::Mat カメラ画像（cv::Mat）
cv::Mat AquireImage(CameraPtr pCam) {
	cv::Mat dstFrame;
	ImagePtr pResultImage = pCam->GetNextImage(100);
	if (pResultImage->IsIncomplete()) {
		// Retrieve and print the image status description
		cout << "Image incomplete: " << Image::GetImageStatusDescription(pResultImage->GetImageStatus())
			<< "..." << endl
			<< endl;
	}
	else {
		//cout << "Grabbed image " << ", width = " << width << ", height = " << height << endl;
					// 画像を変換
		ImagePtr convertedImage = pResultImage->Convert(PixelFormat_BGR8, HQ_LINEAR);
		dstFrame = cv::Mat((int)convertedImage->GetHeight(), (int)convertedImage->GetWidth(), CV_8UC3, convertedImage->GetData()).clone();
		pResultImage->Release();
	}
	return dstFrame;
}

// カメラ画像の取得(マルチスレッド用)
// lpParamの構造体からカメラポインタとカメラ画像ポインタを取得し、
// 取得したカメラ画像をカメラ画像ポインタに書き込む
// (I)lpParam カメラポインタと画像ポインタが格納されている構造体
// returVal : (int) - 1 正常終了
//                  - 0 例外発生

DWORD WINAPI AcquireImage(LPVOID lpParam) {
	PCameraParam& param = *((PCameraParam*)lpParam);
	try {
		ImagePtr pResultImage = param.pCam->GetNextImage(1000);
		if (pResultImage->IsIncomplete()) {
			// Retrieve and print the image status description
			cout << "Image incomplete: " << Image::GetImageStatusDescription(pResultImage->GetImageStatus())
				<< "..." << endl
				<< endl;
		}
		else {
			param.pImg = pResultImage->Convert(PixelFormat_BGR8, HQ_LINEAR);
			param.dstImg = cv::Mat((int)param.pImg->GetHeight(), (int)param.pImg->GetWidth(), CV_8UC3, param.pImg->GetData()).clone();
			cv::resize(param.dstImg, param.dstImg, cv::Size(320, 240));
		}
		return 1;
	}
	catch (Spinnaker::Exception& e) {
		cout << "Error: " << e.what() << endl;
		return 0;
	}
}

//
vector<cv::Mat> AquireMultiCamImages() {
	CameraPtr pCam;
	vector<cv::Mat> Frames;
	for (int i = 0; i < camList.GetSize(); i++) {
		pCam = camList.GetByIndex(i);
		Frames.push_back(AquireImage(pCam));
	}
	pCam = nullptr;
	return Frames;
}

vector<cv::Mat> AquireMultiCamImagesMT() {
	unsigned int camListSize = 0;
	vector<cv::Mat> Frames;	
	int result = 0;
	try {
		camListSize = camList.GetSize();
		CameraPtr* pCamList = new CameraPtr[camListSize];
		HANDLE* grabTheads = new HANDLE[camListSize];
		PCameraParam *pParam = new PCameraParam[camListSize];

		for (int i = 0; i < camListSize; i++) {
			// カメラを選択
			pCamList[i] = camList.GetByIndex(i);
			pParam[i].pCam = pCamList[i];
			grabTheads[i] = CreateThread(nullptr, 0, AcquireImage, &pParam[i], 0, nullptr);
			assert(grabTheads[i] != nullptr);
		}

		WaitForMultipleObjects(
			camListSize, // 待つスレッドの数
			grabTheads,  // 待つスレッドへのハンドル 
			TRUE,        // スレッドをすべて待つ
			INFINITE     // 無限に待つ
		);

		// 各カメラのスレッドの戻り値を確認する
		for (int i = 0; i < camListSize; i++) {
			DWORD exitcode;
			BOOL rc = GetExitCodeThread(grabTheads[i], &exitcode);
			if (!rc) {
				cout << "Handle error from GetExitCodeThread() returned for camera at index " << i << endl;
				result = -1;
			}
			else if (!exitcode) {
				cout << "Grab thread for camera at index " << i
					<< " exited with errors."
					"Please check onscreen print outs for error details"
					<< endl;
				result = -1;
			}
		}

		// カメラ画像を取りだす
		for (int i = 0; i < camListSize; i++) {
			Frames.push_back(pParam[i].dstImg);
		}

		// カメラポインタの配列をクリア＆すべてのハンドルを閉じる
		for (int i = 0; i < camListSize; i++) {
			pCamList[i] = 0;
			CloseHandle(grabTheads[i]);
		}
		delete[] pCamList;
		delete[] grabTheads;
		delete[] pParam;
	}
	catch (Spinnaker::Exception& e) {
		cout << "Error: " << e.what() << endl;
		result = -1;
	}
	return Frames;
}

void ShowAquiredImages(vector<cv::Mat> Frames) {
	for (int i = 0; i < camList.GetSize(); i++) {
		cv::imshow("result " + to_string(i), Frames[i]);
	}

}

int main() {
	int result = 0;

	/****************
	* 初期化処理
	*****************/
	// システムオブジェクトのシングルトン参照を取得
	SystemPtr system = System::GetInstance();
	// カメラの初期化を行う
	InitCameras(system);
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
			vector<cv::Mat> Frames(AquireMultiCamImagesMT());
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
	DeinitCameras(system);
	/**************
	***************/
	return result;

}