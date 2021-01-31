#include "GEV_Wrapper.h"
#include <string>
#include <iostream>
using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;
using namespace std;

// カメラの順番を固定する配列
// カメラのシリアルIDを入力することで、
// 配列に記述した順番でカメラ映像を取得できる
const string cameraID[15] = {
	
	"18551299",
	"18464590",
	"18464423",
	"18551296",
	"18509956",

	"18509958",
	"18408232",
	"18509333",
	"18465007",
	"18509955",
	
	"18464424",
	"18464589",
	"18509340",
	"18464421",
	"18551294",
};

// カメラオブジェクトの初期化
GEV_Drive cam(cameraID);

std::string getTimeStamp()
{
	auto now = std::chrono::system_clock::now();
	auto now_c = std::chrono::system_clock::to_time_t(now);
	std::stringstream ss;
	ss << std::put_time(localtime(&now_c), "%Y%m%d_%H%M%S");
	return ss.str();
}

int main() {
	int result = 0;
	try {
		// カメラの初期化
		cam.Init();
		// カメラ画像の取得開始
		cam.BeginAcquisition();
		bool lp_break = true;
		while (lp_break) {
			vector<cv::Mat> Frames;
			// 1フレーム分すべてのカメラから画像を取得する
			cam >> Frames;
			ShowAquiredImages(Frames);
			if (cv::waitKey(1) == 'c')lp_break = false;
			else if (cv::waitKey(10) == 'a') {
				string time = getTimeStamp();
				for (int i = 0; i < Frames.size(); i++) {
					cv::imwrite(to_string(i) + time, Frames[i]);
				}
			}
		}
		// カメラ画像の取得終了
		cam.EndAcquisition();
	}
	catch (Spinnaker::Exception& e) {
		cout << "Error: " << e.what() << endl;
		result = -1;
	}
	return result;
}