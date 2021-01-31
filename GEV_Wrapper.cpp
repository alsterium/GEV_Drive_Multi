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

// この関数は、トリガを使用するようにカメラを設定します。
//まず、トリガモードをオフに設定し、トリガソースを選択します。
//トリガソースを選択すると、トリガモードが有効になり、選択した
//トリガの実行時にカメラは1枚の画像のみを撮影します。
int ConfigureTrigger(INodeMap& nodeMap)
{
	int result = 0;

	cout << endl << endl << "*** CONFIGURING TRIGGER ***" << endl << endl;

	cout << "Note that if the application / user software triggers faster than frame time, the trigger may be dropped "
		"/ skipped by the camera."
		<< endl
		<< "If several frames are needed per trigger, a more reliable alternative for such case, is to use the "
		"multi-frame mode."
		<< endl
		<< endl;

		cout << "Software trigger chosen..." << endl;

	try
	{
		//
		// Ensure trigger mode off
		//
		// *** NOTES ***
		// The trigger must be disabled in order to configure whether the source
		// is software or hardware.
		//
		CEnumerationPtr ptrTriggerMode = nodeMap.GetNode("TriggerMode");
		if (!IsAvailable(ptrTriggerMode) || !IsReadable(ptrTriggerMode))
		{
			cout << "Unable to disable trigger mode (node retrieval). Aborting..." << endl;
			return -1;
		}

		CEnumEntryPtr ptrTriggerModeOff = ptrTriggerMode->GetEntryByName("Off");
		if (!IsAvailable(ptrTriggerModeOff) || !IsReadable(ptrTriggerModeOff))
		{
			cout << "Unable to disable trigger mode (enum entry retrieval). Aborting..." << endl;
			return -1;
		}

		ptrTriggerMode->SetIntValue(ptrTriggerModeOff->GetValue());

		cout << "Trigger mode disabled..." << endl;

		//
		// Set TriggerSelector to FrameStart
		//
		// *** NOTES ***
		// For this example, the trigger selector should be set to frame start.
		// This is the default for most cameras.
		//
		CEnumerationPtr ptrTriggerSelector = nodeMap.GetNode("TriggerSelector");
		if (!IsAvailable(ptrTriggerSelector) || !IsWritable(ptrTriggerSelector))
		{
			cout << "Unable to set trigger selector (node retrieval). Aborting..." << endl;
			return -1;
		}

		CEnumEntryPtr ptrTriggerSelectorFrameStart = ptrTriggerSelector->GetEntryByName("FrameStart");
		if (!IsAvailable(ptrTriggerSelectorFrameStart) || !IsReadable(ptrTriggerSelectorFrameStart))
		{
			cout << "Unable to set trigger selector (enum entry retrieval). Aborting..." << endl;
			return -1;
		}

		ptrTriggerSelector->SetIntValue(ptrTriggerSelectorFrameStart->GetValue());

		cout << "Trigger selector set to frame start..." << endl;

		//
		// Select trigger source
		//
		// *** NOTES ***
		// The trigger source must be set to hardware or software while trigger
		// mode is off.
		//
		CEnumerationPtr ptrTriggerSource = nodeMap.GetNode("TriggerSource");
		if (!IsAvailable(ptrTriggerSource) || !IsWritable(ptrTriggerSource))
		{
			cout << "Unable to set trigger mode (node retrieval). Aborting..." << endl;
			return -1;
		}

		// Set trigger mode to software
		CEnumEntryPtr ptrTriggerSourceSoftware = ptrTriggerSource->GetEntryByName("Software");
		if (!IsAvailable(ptrTriggerSourceSoftware) || !IsReadable(ptrTriggerSourceSoftware))
		{
			cout << "Unable to set trigger mode (enum entry retrieval). Aborting..." << endl;
			return -1;
		}

		ptrTriggerSource->SetIntValue(ptrTriggerSourceSoftware->GetValue());

		cout << "Trigger source set to software..." << endl;

		//
		// Turn trigger mode on
		//
		// *** LATER ***
		// Once the appropriate trigger source has been set, turn trigger mode
		// on in order to retrieve images using the trigger.
		//

		CEnumEntryPtr ptrTriggerModeOn = ptrTriggerMode->GetEntryByName("On");
		if (!IsAvailable(ptrTriggerModeOn) || !IsReadable(ptrTriggerModeOn))
		{
			cout << "Unable to enable trigger mode (enum entry retrieval). Aborting..." << endl;
			return -1;
		}

		ptrTriggerMode->SetIntValue(ptrTriggerModeOn->GetValue());

		// NOTE: Blackfly and Flea3 GEV cameras need 1 second delay after trigger mode is turned on

		cout << "Trigger mode turned back on..." << endl << endl;
	}
	catch (Spinnaker::Exception& e)
	{
		cout << "Error: " << e.what() << endl;
		result = -1;
	}

	return result;
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
			ptrDeviceLinkThroughputLimit->SetValue((int64_t)28728000);
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

			// トリガーモードを設定する
			err = ConfigureTrigger(nodeMap);
			if (err < 0)
				result = -1;

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

// This function returns the camera to a normal state by turning off trigger
// mode.
int ResetTrigger(INodeMap& nodeMap)
{
	int result = 0;

	try
	{
		//
		// Turn trigger mode back off
		//
		// *** NOTES ***
		// Once all images have been captured, turn trigger mode back off to
		// restore the camera to a clean state.
		//
		CEnumerationPtr ptrTriggerMode = nodeMap.GetNode("TriggerMode");
		if (!IsAvailable(ptrTriggerMode) || !IsReadable(ptrTriggerMode))
		{
			cout << "Unable to disable trigger mode (node retrieval). Non-fatal error..." << endl;
			return -1;
		}

		CEnumEntryPtr ptrTriggerModeOff = ptrTriggerMode->GetEntryByName("Off");
		if (!IsAvailable(ptrTriggerModeOff) || !IsReadable(ptrTriggerModeOff))
		{
			cout << "Unable to disable trigger mode (enum entry retrieval). Non-fatal error..." << endl;
			return -1;
		}

		ptrTriggerMode->SetIntValue(ptrTriggerModeOff->GetValue());

		cout << "Trigger mode disabled..." << endl << endl;
	}
	catch (Spinnaker::Exception& e)
	{
		cout << "Error: " << e.what() << endl;
		result = -1;
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
		ImagePtr pResultImage = param.pCam->GetNextImage(500);
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

DWORD WINAPI TransmitTrigger(LPVOID lpParam) {

	PCameraParam& param = *((PCameraParam*)lpParam);
	try
	{
		INodeMap& nodeMap = param.pCam->GetNodeMap();
		// ソフトウェアトリガーを発信
		CCommandPtr ptrSoftwareTriggerCommand = nodeMap.GetNode("TriggerSoftware");
		if (!IsAvailable(ptrSoftwareTriggerCommand) || !IsWritable(ptrSoftwareTriggerCommand)) {
			cout << "Unable to execute trigger. Abortiong..." << endl;
			return 99;
		}
		ptrSoftwareTriggerCommand->Execute();
		return 1;
	}
	catch (const std::exception& e)
	{
		cout << "Error " << e.what() << endl;
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
		HANDLE* transmitThreads = new HANDLE[camListSize];
		PCameraParam* pParam = new PCameraParam[camListSize];

		for (int i = 0; i < camListSize; i++) {
			// カメラを選択
			pCamList[i] = camList.GetBySerial(cameraID[i]);
			pParam[i].pCam = pCamList[i];
			pParam[i].map1 = map1;
			pParam[i].map2 = map2;
			transmitThreads[i] = CreateThread(nullptr, 0, TransmitTrigger, &pParam[i], 0, nullptr);
			grabTheads[i] = CreateThread(nullptr, 0, AcquireImage, &pParam[i], 0, nullptr);
			assert(grabTheads[i] != nullptr);
		}

		// トリガーを発信する(4ms)
		WaitForMultipleObjects(
			camListSize,
			transmitThreads,
			TRUE,
			INFINITE
		);

		//時間計測(260ms)
		// カメラ画像を取得する
		WaitForMultipleObjects(
			camListSize, // 待つスレッドの数
			grabTheads,  // 待つスレッドへのハンドル 
			TRUE,        // スレッドをすべて待つ
			INFINITE     // 無限に待つ
		);


		// 各カメラのスレッドの戻り値を確認する
		for (int i = 0; i < camListSize; i++) {
			DWORD exitcode;
			BOOL rc = GetExitCodeThread(transmitThreads[i], &exitcode);
			if (!rc) {
				cout << "Handle error from GetExitCodeThread() returned for camera at index " << i << endl;
				result = -1;
			}
			else if (!exitcode) {
				cout << "Transmit thread for camera at index " << i
					<< " exited with errors."
					"Please check onscreen print outs for error details"
					<< endl;
				result = -1;
			}
		}

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
			CloseHandle(transmitThreads[i]);
		}
		delete[] pCamList;
		delete[] grabTheads;
		delete[] transmitThreads;
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

GEV_Drive::~GEV_Drive()
{
	/**************
	* 終了処理
	***************/
	//カメラの初期化を解除
	pCam->DeInit();
	// カメラへのポインタを開放
	pCam = nullptr;
	// カメラの終了処理
	DeinitCameras(system, camList);
}

void GEV_Drive::Init()
{
	try
	{
		/****************
		* 初期化処理
		*****************/
		// システムオブジェクトのシングルトン参照を取得
		system = System::GetInstance();
		// カメラの初期化を行う
		InitCameras(system, camList, cameraID);
		/****************
		*****************/
		// 構造化束縛により変数を取りだす
		auto [tmpMap1, tmpMap2] = initMap(intrinsicsFile);
		map1 = tmpMap1;
		map2 = tmpMap2;
	}
	catch (const std::exception& e)
	{
		cout << "Error: " << e.what() << endl;
	}
	
}

void GEV_Drive::BeginAcquisition()
{
	try
	{
		for (int i = 0; i < camList.GetSize(); i++) {
			pCam = camList.GetBySerial(cameraID[i]);
			pCam->BeginAcquisition();
		}
	}
	catch (const std::exception& e)
	{
		cout << "Error: " << e.what() << endl;
	}

}

void GEV_Drive::EndAcquisition()
{
	try
	{
		for (int i = 0; i < camList.GetSize(); i++) {
			pCam = camList.GetByIndex(i);
			INodeMap& nodeMap = pCam->GetNodeMap();
			ResetTrigger(nodeMap);
			pCam->EndAcquisition();
		}
	}
	catch (const std::exception& e)
	{
		cout << "Error: " << e.what() << endl;
	}
}

void GEV_Drive::operator>>(std::vector<cv::Mat> &dstFrame)
{
	dstFrame = AquireMultiCamImagesMT(camList, cameraID, map1, map2);
}
