#include <windows.h>
#include <filesystem>
#include <string>
#include <opencv2/opencv.hpp>

namespace fs = std::filesystem;

void scrape(std::string path, std::string url, int numPages);
void resize(std::string in, std::string out, int width, int height);
cv::Ptr<cv::ml::KNearest> processImages(fs::path dir_in, fs::path dir_out, int width, int height);
cv::Mat findNearest(cv::Mat image, cv::Ptr<cv::ml::KNearest> model);
cv::Mat mean(cv::Mat image);
cv::Mat flatten(cv::Mat image, int size);
cv::Mat createPhotomosaic(std::string in, int finalWidth, int finalHeight, int tileSize, fs::path tiles, cv::Ptr<cv::ml::KNearest> model);
void usage(char** argv);

int main(int argc, char** argv) {
	if (argc != 2 && argc != 4) {
		usage(argv);
		return 1;
	}
	std::string image = argv[1];
	if (argc == 4) {
		std::string url = argv[2];
		int numPages = std::stoi(argv[3]);
		scrape("C:/Users/trevo/Code/Scraper/main.exe", url, numPages);
	}

	int width = 50;
	int height = 50;
	fs::create_directory("tiles");
	cv::Ptr<cv::ml::KNearest> model = processImages("images", "tiles", width, height);
	cv::Mat photomosaic = createPhotomosaic(image, 7200, 4800, 50, "tiles", model);
	cv::imwrite("photomosaic.jpg", photomosaic);
	cv::namedWindow("Photomosaic", cv::WINDOW_NORMAL);
	cv::imshow("Photomosaic", photomosaic);
	cv::waitKey();
	cv::destroyAllWindows();
}

void scrape(std::string path, std::string url, int numPages) {
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	// set the size of the structures
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));
	std::string cmdArgs = path + " " + url + " " + std::to_string(numPages);

	// start the program up
	CreateProcess(
		path.c_str(),   // the path
		&*cmdArgs.begin(),			// Command line
		NULL,           // Process handle not inheritable
		NULL,           // Thread handle not inheritable
		FALSE,          // Set handle inheritance to FALSE
		0,              // No creation flags
		NULL,           // Use parent's environment block
		NULL,           // Use parent's starting directory 
		&si,            // Pointer to STARTUPINFO structure
		&pi             // Pointer to PROCESS_INFORMATION structure (removed extra parentheses)
	);

	// Wait until child process exits.
	WaitForSingleObject(pi.hProcess, INFINITE);

	// Close process and thread handles. 
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}

void resize(std::string in, std::string out, int width, int height) {
	cv::Mat image = cv::imread(in);
	cv::Mat resized;
	cv::resize(image, resized, cv::Size(width, height), cv::INTER_LINEAR);
	cv::imwrite(out, resized);
}

cv::Ptr<cv::ml::KNearest> processImages(fs::path dir_in, fs::path dir_out, int width, int height) {
	cv::Mat trainMat(0, 4, CV_32F);
	cv::Mat classes(0, 1, CV_32F);
	for (auto const& dir_entry : fs::directory_iterator{ dir_in }) {
		fs::path fPath = dir_entry.path();
		fs::path outPath = dir_out / fPath.filename();
		resize(fPath.string(), outPath.string(), width, height);

		cv::Mat image = cv::imread(outPath.string());

		//cv::Mat flattened = flatten(image, width * height * 3);
		//trainMat.push_back(flattened);

		cv::Mat mean32F = mean(image);
		trainMat.push_back(mean32F);

		classes.push_back(std::stoi(fPath.stem()));
	}
	cv::Ptr<cv::ml::TrainData> train = cv::ml::TrainData::create(trainMat, cv::ml::SampleTypes::ROW_SAMPLE, classes);
	cv::Ptr< cv::ml::KNearest > model = cv::ml::KNearest::create();
	model->setIsClassifier(true);
	model->train(train);
	return model;
}

cv::Mat findNearest(cv::Mat image, cv::Ptr<cv::ml::KNearest> model) {
	cv::Mat res(1, 1, CV_32F);

	cv::Mat mean32F = mean(image);
	model->findNearest(mean32F, 1, res);

	//cv::Mat resized;
	//cv::resize(image, resized, cv::Size(50, 50), cv::INTER_LINEAR);
	//cv::Mat flattened = flatten(resized, 7500);
	//model->findNearest(flattened, 1, res);

	return res;
}

cv::Mat mean(cv::Mat image) {
	cv::Mat mean(cv::mean(image).t());
	cv::Mat mean32F(1, 4, CV_32F);
	mean.convertTo(mean32F, CV_32F);
	return mean32F;
}

cv::Mat flatten(cv::Mat image, int size) {
	cv::Mat flattened = image.reshape(1, 1);
	cv::Mat flattened32f(size, 1, CV_32F);
	flattened.convertTo(flattened32f, CV_32F);
	return flattened32f;
}

cv::Mat createPhotomosaic(std::string in, int finalWidth, int finalHeight, int tileSize, fs::path tiles, cv::Ptr<cv::ml::KNearest> model) {
	cv::Mat image = cv::imread(in);
	cv::Mat resized;
	cv::resize(image, resized, cv::Size(finalWidth, finalHeight), cv::INTER_LINEAR);
	image = resized;

	int i = 0;
	while (i < image.rows) {
		int j = 0;
		while (j < image.cols) {
			cv::Mat sub = image.colRange(j, std::min(j + tileSize, image.cols - 1)).rowRange(i, std::min(i + tileSize, image.rows - 1));
			cv::Mat nearest = findNearest(sub, model);
			fs::path tilePath = tiles / (std::to_string(static_cast<int>(nearest.at<float>(0, 0))) + ".jpg");
			cv::Mat tile = cv::imread(tilePath.string());
			if (tile.size() != sub.size()) {
				cv::Mat resized;
				cv::resize(tile, resized, sub.size(), cv::INTER_LINEAR);
				tile = resized;
			}
			tile.copyTo(sub);
			j += tileSize;
		}
		i += tileSize;
	}
	return image;
}

void usage(char** argv) {
	std::cerr << "Usage: " << argv[0] << " source_image [subreddit_url pages]" << std::endl;
}