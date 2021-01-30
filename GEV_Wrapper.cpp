#include "GEV_Wrapper.h"

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;
using namespace std;

std::tuple<cv::Mat, cv::Mat> initMap(const std::string intrinsicsFile)
{
	cv::Mat map1, map2;
	try
	{
		// 行列をファイルから読み込む
		cout << "Loading camera param from xml file..." << endl;
		cv::FileStorage fs("intrinsics.xml", cv::FileStorage::READ);
		cout << "\nimage width: " << (int)fs["image_width"];
		cout << "\nimage height: " << (int)fs["image_height"];

		cv::Mat intrinsic_matrix_loaded, distortion_coeffs_loaded;
		fs["camera_matrix"] >> intrinsic_matrix_loaded;
		fs["distortion_coefficients"] >> distortion_coeffs_loaded;
		cout << "\nintrinsic matrix: " << intrinsic_matrix_loaded;
		cout << "\ndistortion coefficients: " << distortion_coeffs_loaded << endl;

		//後続フレーム全てに対して用いる歪み補正用のマップを作成する
		//
		cv::initUndistortRectifyMap(
			intrinsic_matrix_loaded,
			distortion_coeffs_loaded,
			cv::Mat(),
			intrinsic_matrix_loaded,
			cv::Size(320, 240),
			CV_16SC2,
			map1,
			map2
		);
	}
	catch (const std::exception& e)
	{
		cout << "Error: " << e.what() << endl;
	}
	return {map1, map2};
}

int ConfigureCustomImageSetting(Spinnaker::GenApi::INodeMap& nodeMap) {
	int result = 0;
	cout << endl << endl << "*** CONFIGURING CUSTOM IMAGE SETTINGS ***" << endl << endl;

	try {

		// カメラのシリアルIDを表示する
		CStringPtr ptrDeviceSerialNumber = nodeMap.GetNode("DeviceSerialNumber");
		if (IsReadable(ptrDeviceSerialNumber))
		{
			cout << "deviceSerialID: " << string(ptrDeviceSerialNumber->GetValue()) << endl;
		}

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

int InitCameras(SystemPtr& system, CameraList &camList, const string cameraID[]) {

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
			pCam = camList.GetBySerial(cameraID[i]);
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
		// 構造化束縛により変数を取りだす
		auto [map1, map2] = initMap("intrinsics.xml");
	}
	catch (Spinnaker::Exception& e) {
		cout << "Error: " << e.what() << endl;
	}
	return result;
}

void DeinitCameras(SystemPtr& system, CameraList &camList) {
	// システムを開放する前にカメラリストを開放
	camList.Clear();
	// システムを開放
	system->ReleaseInstance();

	cout << endl << "Done! Press Enter to exit..." << endl;
	getchar();
}

DWORD WINAPI AcquireImage(LPVOID lpParam) {
	PCameraParam& param = *((PCameraParam*)lpParam);
	try {
		ImagePtr pResultImage = param.pCam->GetNextImage(400);
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
			cv::remap(
				param.dstImg,
				param.dstImg,
				param.map1,
				param.map2,
				cv::INTER_LINEAR,
				cv::BORDER_CONSTANT,
				cv::Scalar()
			);
		}
		return 1;
	}
	catch (Spinnaker::Exception& e) {
		cout << "Error: " << e.what() << endl;
		return 0;
	}
}

vector<cv::Mat> AquireMultiCamImagesMT(
	CameraList &camList, 
	const string cameraID[], 
	cv::Mat map1, 
	cv::Mat map2
) 
{
	unsigned int camListSize = 0;
	vector<cv::Mat> Frames;
	int result = 0;
	try {
		camListSize = camList.GetSize();
		CameraPtr* pCamList = new CameraPtr[camListSize];
		HANDLE* grabTheads = new HANDLE[camListSize];
		PCameraParam* pParam = new PCameraParam[camListSize];

		for (int i = 0; i < camListSize; i++) {
			// カメラを選択
			pCamList[i] = camList.GetBySerial(cameraID[i]);
			pParam[i].pCam = pCamList[i];
			pParam[i].map1 = map1;
			pParam[i].map2 = map2;
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
	for (int i = 0; i < Frames.size(); i++) {
		if (!Frames[i].empty())
			cv::imshow("result " + to_string(i), Frames[i]);
	}
}