#include <windows.h>
#include <filesystem>
#include <string>
#include <opencv2/opencv.hpp>

namespace fs = std::filesystem;

// Scrape numImages from url, downloading them into outDir. The path parameter is the location of scraper.exe.
void scrape(std::string path, std::string url, int numImages, std::string outDir);

// Resize an image located at in to width and height, downloading the result into out.
void resize(std::string in, std::string out, int width, int height);

// Resizes images from dir_in to width and height and downloads results into dir_out. Creates and returns one nearest neighbor model that compares images on their average rgbs to the resized images. 
cv::Ptr<cv::ml::KNearest> processImages(fs::path dir_in, fs::path dir_out, int width, int height);

// Given image, finds closest image from model.
cv::Mat findNearest(cv::Mat image, cv::Ptr<cv::ml::KNearest> model);

// Returns a 4 component row vector of type CV_32F representing the mean of image.
cv::Mat mean(cv::Mat image);

// Creates and returns a photomosaic of in, with a scaling factor of scale, and tileSizes of tileSize. The tiles parameter is the location of the tiles, and model is the one nearest neighbor model used. 
cv::Mat createPhotomosaic(std::string in, float scale, int tileSize, fs::path tiles, cv::Ptr<cv::ml::KNearest> model);

// Prints usage of the program on the console.
void usage(char** argv);

// Checks if command line argument is the flag represented by option.
bool isOption(std::string arg, std::string option);

int main(int argc, char** argv) {
	// Set command line arguments
	if (argc < 2) {
		usage(argv);
		return 1;
	}
	std::string url = "EarthPorn";
	int numImages = 100;
	int tileSize = 25;
	float scale = 1;
	std::string outName = "";
	for (int i = 1; i < argc - 1; i++) {
		std::string arg = argv[i];
		if (isOption(arg, "--sub")) {
			url = arg.substr(std::string("--sub").length() + 1);
		}
		else if (isOption(arg, "--n")) {
			numImages = std::stoi(arg.substr(std::string("--n").length() + 1));
		}
		else if (isOption(arg, "--tile_size")) {
			tileSize = std::stoi(arg.substr(std::string("--tile_size").length() + 1));
		}
		else if (isOption(arg, "--scale")) {
			scale = std::stof(arg.substr(std::string("--scale").length() + 1));
		}
		else if (isOption(arg, "--o")) {
			outName = arg.substr(std::string("--o").length() + 1);
		}
		else {
			usage(argv);
			return 1;
		}
	}
	std::string image = argv[argc - 1];

	// Set paths
	std::string pathBuf;
	pathBuf.resize(MAX_PATH);
	GetModuleFileName(NULL, &pathBuf.at(0), MAX_PATH);
	fs::path root(fs::path(pathBuf).remove_filename());
	fs::path scraperPath(root / fs::path("Scraper") / fs::path("scraper.exe"));
	fs::path outPath(root / "images");
	fs::path tilesPath(root / "tiles");
	fs::path photomosaicsPath(root / "photomosaics");

	// Create directories
	fs::create_directory(outPath);
	fs::create_directory(tilesPath);
	fs::create_directory(photomosaicsPath);

	// Scrape images
	scrape(scraperPath.string(), "https://old.reddit.com/r/" + url + "/", numImages, outPath.string());

	// Create model
	cv::Ptr<cv::ml::KNearest> model = processImages(outPath, tilesPath, tileSize, tileSize);

	// Create photomosaic
	cv::Mat photomosaic = createPhotomosaic(image, scale, tileSize, tilesPath, model);

	if (outName == "") {
		outName = fs::path(image).filename().string();
	}
	cv::imwrite((photomosaicsPath / outName).string(), photomosaic);
	cv::namedWindow("Photomosaic", cv::WINDOW_NORMAL);
	cv::imshow("Photomosaic", photomosaic);
	cv::waitKey();
	cv::destroyAllWindows();
}

void scrape(std::string path, std::string url, int numImages, std::string outDir) {
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	// set the size of the structures
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));
	std::string cmdArgs = path + " " + url + " " + std::to_string(numImages) + " " + outDir;

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
		fs::path outName = dir_out / fPath.filename();
		resize(fPath.string(), outName.string(), width, height);

		cv::Mat image = cv::imread(outName.string());
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
	return res;
}

cv::Mat mean(cv::Mat image) {
	cv::Mat mean(cv::mean(image).t());
	cv::Mat mean32F(1, 4, CV_32F);
	mean.convertTo(mean32F, CV_32F);
	return mean32F;
}

cv::Mat createPhotomosaic(std::string in, float scale, int tileSize, fs::path tiles, cv::Ptr<cv::ml::KNearest> model) {
	cv::Mat image = cv::imread(in);
	cv::Mat resized;
	cv::resize(image, resized, cv::Size(), scale, scale, cv::INTER_LINEAR);
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
	std::cerr << "Usage: " << argv[0] << " image_path" << '\n'
		<< "Options:" << '\n'
		<< "--sub=<subreddit>" << '\n'
		<< "--n=<number of images>" << '\n'
		<< "--tile_size=<size of tiles>" << '\n'
		<< "--scale=<scale of image>" << '\n'
		<< "--o=<name of output image>" << std::endl;
}

bool isOption(std::string arg, std::string option) {
	std::size_t found = arg.find(option);
	return found != std::string::npos;
}