// g++ -o fractals main.cpp -lncurses `pkg-config --cflags --libs opencv4`

#include <ncurses.h>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

using namespace std;

enum FractalType {
	MANDELBROT,		// 0
	JULIA,			// 1
	BURNING_SHIP,	// 2
	NEWTON_BASINS,	// 3
	FRACTAL_COUNT	// 4
};

enum ColorPalette {
	GRAYSCALE,
	FIRE,
	OCEAN,
	FOREST,
	PALETTE_COUNT
};

struct RGBColor {
	int r, g, b;

	RGBColor() : r(0), g(0), b(0) {}
	RGBColor(int red, int green, int blue) : 
		r(red), g(green), b(blue) {}
};

struct FractalSettings {
	double centerX, centerY;	// Center of the FOV
	double scale;				// Horisontal size of one symbol
	double juliaCx, juliaCy;	// Fixed point for Julia
    
	FractalSettings() : centerX(0), centerY(0), scale(0.01), juliaCx(-0.7), juliaCy(0.27) {}
};

class FractalRenderer {
private:
	FractalSettings fractalSettings[FRACTAL_COUNT]; // Array of settings for each fractal
	FractalType currentFractal; // Data about current fractal
	ColorPalette currentPalette;// For saving .png
	double aspectRatio;			// Symbol height/width
	double scaleX, scaleY;
	int width, height;			// Height and width of the terminal in symbols
	int maxiter;				// Maximum number of iterations
	bool running;				// Breaking the while cycle in main()
	const char* fractalNamesSpaces[FRACTAL_COUNT] = {"Mandelbrot   ", "Julia        ", "Burning Ship ", "Newton Basins"};
	const char* fractalNamesUnderscore[FRACTAL_COUNT] = {"Mandelbrot", "Julia", "Burning_Ship", "Newton_Basins"};
	const char* fractalNames[FRACTAL_COUNT] = {"Mandelbrot", "Julia", "Burning Ship", "Newton Basins"};
	const char* paletteNames[PALETTE_COUNT] = {"Grayscale", "Fire", "Ocean", "Forest"};
	vector<int> compressionParams;

public:
	FractalRenderer(): currentFractal(MANDELBROT), currentPalette(GRAYSCALE), aspectRatio(2.11), running(true), maxiter(300) {
		fractalSettings[MANDELBROT].centerX = -0.5;
		fractalSettings[MANDELBROT].centerY = 0.0;
		fractalSettings[MANDELBROT].scale = 0.015;

		fractalSettings[JULIA].centerX = 0.0;
		fractalSettings[JULIA].centerY = 0.0;
		fractalSettings[JULIA].scale = 0.015;
		fractalSettings[JULIA].juliaCx = -0.7;
		fractalSettings[JULIA].juliaCy = 0.27;

		fractalSettings[BURNING_SHIP].centerX = -0.5;
		fractalSettings[BURNING_SHIP].centerY = -0.5;
		fractalSettings[BURNING_SHIP].scale = 0.02;

		fractalSettings[NEWTON_BASINS].centerX = 0.0;
		fractalSettings[NEWTON_BASINS].centerY = 0.0;
		fractalSettings[NEWTON_BASINS].scale = 0.02;

		updateScales();
		
		compressionParams.push_back(cv::IMWRITE_PNG_COMPRESSION);
		compressionParams.push_back(8);
		compressionParams.push_back(cv::IMWRITE_PNG_STRATEGY);
		compressionParams.push_back(cv::IMWRITE_PNG_STRATEGY_RLE); // Хорошо для фракталов
		compressionParams.push_back(cv::IMWRITE_PNG_BILEVEL);
		compressionParams.push_back(0);
	}

	void updateScales() {
		FractalSettings& settings = fractalSettings[currentFractal];
		scaleX = settings.scale;
		scaleY = settings.scale * aspectRatio;
	}
	
	void initialize() {
		initscr();	// Create standard screen
		cbreak();	// Disable line buffering
		noecho();	// Obvious
		keypad(stdscr, TRUE);	// Enable special keyboard keys (i'll use arrows)
		curs_set(0);			// Hide the cursor
		start_color();  // Включить поддержку цветов
		use_default_colors();  // Использовать цвета по умолчанию
		init_pair(0, COLOR_BLACK, COLOR_BLACK);
		init_pair(1, COLOR_RED, COLOR_RED);
		init_pair(2, COLOR_GREEN, COLOR_GREEN);
		init_pair(3, COLOR_BLUE, COLOR_BLUE);
		getmaxyx(stdscr, height, width);	// Get terminal dimensions
    
	}

	void setAspectRatio() {
		clear();
		mvprintw(0, 0, "Current aspect ratio: %.2f", aspectRatio);
		mvprintw(1, 0, "Enter new aspect ratio in the interval [0.5, 3.0] (default 2.11): ");
		refresh();

		echo();
		char input[20];
		getstr(input);
		double newRatio = atof(input);

		if (0.5 <= newRatio && newRatio <= 3.0)
			aspectRatio = newRatio;
		noecho();
	}

	void setJuliaParams() {
		FractalSettings& settings = fractalSettings[JULIA];
		clear();
		mvprintw(0, 0, "Current Julia parameters: c = %.2f + %.2fi. I also recommend c = -0.4 + 0.6i", settings.juliaCx, settings.juliaCy);
		mvprintw(1, 0, "Enter new Julia parameters");
		mvprintw(2, 0, "Real part: ");
		refresh();

		echo();
		char input[20];
		getstr(input);
		settings.juliaCx = atof(input);

		mvprintw(3, 0, "Imaginary part: ");\
		getstr(input);
		settings.juliaCy = atof(input);
		noecho();
		
	}

	void selectFractalMenu() {
		FractalType selected = currentFractal;
		bool menuActive = true;

		while(menuActive) {
			clear();
			int startX = width/2 - 15;
			int startY = height/2 - FRACTAL_COUNT / 2 - 3;

			attron(A_BOLD);
			mvprintw(startY-3, startX, "FRACTAL VIEWER");
			mvprintw(startY-2, startX, "==============");
			attroff(A_BOLD);
			mvprintw(startY-1, startX, "SELECT FRACTAL:");

			for(int i = 0; i < FRACTAL_COUNT; ++i) {
				if (i == selected) attron(A_STANDOUT);
				mvprintw(startY + i, startX, "%d. %s", i + 1, fractalNames[i]);
				if (i == selected) attroff(A_STANDOUT);
			}
			
			attron(A_BOLD);
			mvprintw(startY + FRACTAL_COUNT+1, startX-10, "In menu:");
			attroff(A_BOLD);
			mvprintw(startY + FRACTAL_COUNT+2, startX-10, "Arrows - navigate    Enter - confirm");
			mvprintw(startY + FRACTAL_COUNT+3, startX-10, "1-%d - quick select   q - exit program", FRACTAL_COUNT);

			attron(A_BOLD);
			mvprintw(startY + FRACTAL_COUNT+5, startX-10, "In fractal viewer:");
			attroff(A_BOLD);
			mvprintw(startY + FRACTAL_COUNT+6, startX-10, "+/- - zoom in/out");
			mvprintw(startY + FRACTAL_COUNT+7, startX-10, "WASD - fast move     Arrows - precise move");
			mvprintw(startY + FRACTAL_COUNT+8, startX-10, "m - back to menu     r - change aspect ratio");
			mvprintw(startY + FRACTAL_COUNT+9, startX-10, "q - exit program     c - change Julia parameters");

			refresh();

			bool tryAgain = true;
			while (tryAgain) {
				tryAgain = false;
				int ch = getch();
				switch (ch) {
					case KEY_UP:
						selected = static_cast<FractalType>((selected - 1 + FRACTAL_COUNT) % FRACTAL_COUNT); break;
					case KEY_DOWN:
						selected = static_cast<FractalType>((selected + 1) % FRACTAL_COUNT); break;
					case 10: case 13:
						currentFractal = selected; menuActive = false; break;
					case '1':
						currentFractal = MANDELBROT; menuActive = false; break;
					case '2':
						currentFractal = JULIA; menuActive = false; break;
					case '3':
						currentFractal = BURNING_SHIP; menuActive = false; break;
					case '4':
						currentFractal = NEWTON_BASINS; menuActive = false; break;
					case 'q':
						running = false; menuActive = false; break;
					case KEY_RESIZE:
						getmaxyx(stdscr, height, width); break;
					default:
						tryAgain = true;
				}	
			}
			
		}
	}

	char getPixelChar(int iter) {
		const char* chars = " .-:=*#%@";	// Palette
		int paletteSize = strlen(chars);
		
		if (iter >= maxiter) return chars[paletteSize-1];

		double t = (double)iter / maxiter;
		double gamma = 2.2;
		t = pow(t, 1./gamma);
		
		int index = t * (paletteSize - 1);
		return chars[index];
	}

	int mandelbrotPoint(double cx, double cy) {
		double zx = 0., zy = 0., zx2 = 0., zy2 = 0.; // Re and Im parts and their squares

		int iteration = 0;
		while (zx2 + zy2 < 4. && iteration < maxiter) {
			zy = 2.0*zx*zy + cy;
			zx = zx2 - zy2 + cx;
			zx2 = zx*zx; zy2 = zy*zy;
			++iteration;
		}
		return iteration;
	}

	int juliaPoint(double zx, double zy) {
		double cx = fractalSettings[JULIA].juliaCx, cy = fractalSettings[JULIA].juliaCy;
		double zx2 = zx*zx, zy2 = zy*zy;

		int iteration = 0;
		while (zx2 + zy2 < 4. && iteration < maxiter){
			zy = 2.0*zx*zy + cy;
			zx = zx2 - zy2 + cx;
			zx2 = zx*zx; zy2 = zy*zy;
			++iteration;
		}
		return iteration;
	}

	int burningShipPoint(double cx, double cy) {
		double zx = 0., zy = 0., zx2 = 0., zy2 = 0.;
		int iteration = 0;

		while (zx2 + zy2 < 4.0 && iteration < maxiter) {
			zy = 2 * fabs(zx * zy) + cy;
			zx = zx2 - zy2 + cx;
			zx2 = zx*zx; zy2 = zy*zy;
			++iteration;
		}
		return iteration;
	}

	int newtonBasinsPoint(double zx, double zy){
		double roots[6] = { // of y = x^3 - 1
			1.0, 0.0,
			-0.5, sqrt(3.)/2,
			-0.5, -sqrt(3.)/2
		};

		double tolerance = 1e-4;
		int max_newton_iter = 50;

		double x2, y2, xy, fx, fy, fpx, fpy, denom, dx_root, dy_root;
		int j;

		for (int i = 0; i < max_newton_iter; ++i) {
			x2 = zx * zx;
			y2 = zy * zy;
			xy = zx * zy;

			fx = zx * (x2 - 3*y2) - 1;
			fy = zy * (3*x2 - y2);

			fpx = 3 * (x2 - y2);
			fpy = 6 * xy;

			denom = fpx * fpx + fpy * fpy;
			if (denom == 0) break;

			zx -= (fx * fpx + fy * fpy) / denom;
			zy -= (fy * fpx - fx * fpy) / denom;

			for (j = 0; j < 3; ++j) {
				dx_root = zx - roots[2*j];
				dy_root = zy - roots[2*j+1];
				if (dx_root * dx_root + dy_root * dy_root < tolerance * tolerance) {
					return j + 1;
				}
			}
		}
		return 0;
	}

	void renderNewtonBasins() {
		FractalSettings& settings = fractalSettings[NEWTON_BASINS];
		updateScales();
		double cx, cy;
		int color;
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				cx = (x - width/2.) * scaleX + settings.centerX;	// X coordinate of the point
				cy = (y - height/2.) * scaleY + settings.centerY;	// Y coordinate of the point

				color = newtonBasinsPoint(cx, cy);
				attron(COLOR_PAIR(color));
				mvaddch(y, x, '@');
				attroff(COLOR_PAIR(color));
			}
		}
	}

	void renderOtherFractals() {
		FractalSettings& settings = fractalSettings[currentFractal];
		updateScales();
		double cx, cy;
		int iteration;
		char pixel;
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				cx = (x - width/2.) * scaleX + settings.centerX;	// X coordinate of the point
				cy = (y - height/2.) * scaleY + settings.centerY;	// Y coordinate of the point

				switch (currentFractal) {
					case MANDELBROT: 	iteration = mandelbrotPoint(cx, cy); break;
					case JULIA:			iteration = juliaPoint(cx, cy); break;
					case BURNING_SHIP:	iteration = burningShipPoint(cx, cy); break;
				}
				
				pixel = getPixelChar(iteration);
				mvaddch(y, x, pixel);
			}
		}
		
	}

	RGBColor getPixelColor(int iter) {
		if (iter >= maxiter) return RGBColor(0, 0, 0);
	
		double t = (double)iter / maxiter;
		double gamma = 2.2;
		t = pow(t, 1./gamma);

		switch (currentPalette) {
			case GRAYSCALE: {
				int value = t*255;
				return RGBColor(value, value, value);
			}
			case FIRE: {
				int r = min(255, (int)(t * 255*1.5));
				int g = (int)(t * 255*0.8);
				int b = (int)(t * 255*0.2);
				return RGBColor(r, g, b);
			}
			case OCEAN: {
				int r = (int)(t * 255*0.2);
				int g = (int)(t * 255*0.5);
				int b = min(255, (int)(t * 255*1.2));
				return RGBColor(r, g, b);
			}
			case FOREST: {
				int r = (int)(t * 255*0.3);
				int g = min(255, (int)(t * 255*1.2));
				int b = (int)(t * 255*0.25);
				return RGBColor(r, g, b);
			}
		}
		return RGBColor(128, 128, 128);
	}

	void saveOtherFractals(int imageHeight, int imageWidth, string filename) {
		FractalSettings& settings = fractalSettings[currentFractal];
		cv::Mat image(imageHeight, imageWidth, CV_8UC3);
		RGBColor color;
		double cx, cy;
		double scX = width * settings.scale, scY = width * imageHeight / imageWidth * settings.scale;
		int iteration;

		for (int y = 0; y < imageHeight; ++y) {
			for (int x = 0; x < imageWidth; ++x) {
				cx = (static_cast<double>(x) / imageWidth - 0.5) * scX + settings.centerX;
				cy = (static_cast<double>(y) / imageHeight - 0.5) * scY + settings.centerY;

				switch (currentFractal) {
					case MANDELBROT:	iteration = mandelbrotPoint(cx, cy); break;
					case JULIA:			iteration = juliaPoint(cx, cy); break;
					case BURNING_SHIP:	iteration = burningShipPoint(cx, cy); break;
				}

				color = getPixelColor(iteration);
				image.at<cv::Vec3b>(y, x) = cv::Vec3b(color.b, color.g, color.r); // BGR format!
			}
		}
		cv::imwrite(filename, image, compressionParams);
	}

	void saveNewtonBasins(int imageHeight, int imageWidth, string filename) {
		FractalSettings& settings = fractalSettings[NEWTON_BASINS];
		cv::Mat image(imageHeight, imageWidth, CV_8UC3);
		int color, colors[12] = {38, 84, 124, 239, 71, 111, 255, 209, 102, 6, 214, 160}; // 4 colors in RGB
		double cx, cy;

		for (int y = 0; y < imageHeight; ++y) {
			for (int x = 0; x < imageWidth; ++x) {
				cx = (static_cast<double>(x) / imageWidth - 0.5) * width * settings.scale + settings.centerX;
				cy = (static_cast<double>(y) / imageHeight - 0.5) * width * imageHeight / imageWidth * settings.scale + settings.centerY;

				color = newtonBasinsPoint(cx, cy);
				if (color >= 0 && color < 4)
					image.at<cv::Vec3b>(y, x) = cv::Vec3b(colors[3*color + 2], colors[3*color + 1], colors[3*color]); // Saving in BGR
				else
					image.at<cv::Vec3b>(y, x) = cv::Vec3b(0, 0, 0);
			}
		}
		cv::imwrite(filename, image, compressionParams);
	}

	string currentDateTime() {
		auto now = chrono::system_clock::now();
		auto time_t = chrono::system_clock::to_time_t(now);

		stringstream ss;
		ss << put_time(localtime(&time_t), "%Y%m%d_%H%M%S");
		
		string currentDateTimeStr = ss.str();
		return currentDateTimeStr;
	}
	
	void imageSave() {
		ColorPalette selected = currentPalette;
		bool selectActive = true, needSaving = true;
		clear();
		int imageHeight, imageWidth;
		mvprintw(0, 0, "Filename is generated automatically. Enter width and height of the output image");
		mvprintw(1, 0, "Enter width: ");
		refresh();

		echo();
		char input[20];
		getstr(input);
		imageWidth = atoi(input);

		mvprintw(2, 0, "Enter height: ");
		getstr(input);
		imageHeight = atoi(input);
		noecho();

		while(selectActive && (currentFractal != NEWTON_BASINS)) {
			clear();
			mvprintw(0, 0, "FIlename is generated automatically. Enter width and height of the output image");
			mvprintw(1, 0, "Enter width: %d", imageWidth);
			mvprintw(2, 0, "Enter height: %d", imageHeight);
			attron(A_BOLD);
			mvprintw(4, 0, "CHOOSE COLOR PALETTE:");
			attroff(A_BOLD);

			for(int i = 0; i < PALETTE_COUNT; ++i) {
				if (i == selected) attron(A_STANDOUT);
				mvprintw(5+i, 0, "%d. %s", i+1, paletteNames[i]);
				if (i == selected) attroff(A_STANDOUT);
			}
			mvprintw(6+PALETTE_COUNT, 0, "Arrows - navigate    Enter - confirm");
			mvprintw(7+PALETTE_COUNT, 0, "1-%d - quick select   q - back to fractal viewer", PALETTE_COUNT);

			refresh();

			bool tryAgain = true;
			while (tryAgain) {
				tryAgain = false;
				int ch = getch();
				switch (ch) {
					case KEY_UP: 
						selected = static_cast<ColorPalette>((selected - 1 + PALETTE_COUNT) % PALETTE_COUNT); break;
					case KEY_DOWN:
						selected = static_cast<ColorPalette>((selected + 1) % PALETTE_COUNT); break;
					case 10: case 13:
						currentPalette = selected; selectActive = false; break;
					case '1':
						currentPalette = GRAYSCALE; selectActive = false; break;
					case '2':
						currentPalette = FIRE; selectActive = false; break;
					case '3':
						currentPalette = OCEAN; selectActive = false; break;
					case '4':
						currentPalette = FOREST; selectActive = false; break;
					case 'q':
						needSaving = false; selectActive = false; break;
					default:
						tryAgain = true;
				}
			}
		}

		if (needSaving){
			string filename = "./PNG_output/" + string(fractalNamesUnderscore[currentFractal]) + "_" + currentDateTime() + ".png";

			switch (currentFractal) {
				case NEWTON_BASINS: saveNewtonBasins(imageHeight, imageWidth, filename); break;
				default: saveOtherFractals(imageHeight, imageWidth, filename);
			}
			mvprintw(9 + PALETTE_COUNT, 0, "%s successfully saved. Press any button", filename.c_str());
			getch();
		}
	}

	void showFractalInfo() {
		FractalSettings& settings = fractalSettings[currentFractal];
		attron(A_REVERSE);
		mvprintw(0, 0, "Fractal: %s | Scale: %.2e | Center coordinates: (%+.7e, %+.7e)       ", fractalNamesSpaces[currentFractal], settings.scale, settings.centerX, settings.centerY);
		mvprintw(1, 0, "Terminal dimensions: %4d x %4d | Aspect ratio: %.2f | q - quit | m - menu | r - change aspect ratio ", width, height, aspectRatio);
		if (currentFractal == JULIA) 
			mvprintw(2, 0, "Julia parameter: c = (%+.2f, %+.2f) | c - change Julia parameter                                      ", settings.juliaCx, settings.juliaCy);
		attroff(A_REVERSE);
	}
	
	void handleInput() {
		int ch = getch();
		FractalSettings& settings = fractalSettings[currentFractal];
		switch (ch) {
			case 'q':		running = false; break;
			case 'm':		selectFractalMenu(); break;
			case 'r':		setAspectRatio(); break;
			case 'c': 
				if (currentFractal == JULIA) setJuliaParams();
				break;
			case 'S': imageSave(); break;
			case KEY_UP: 	settings.centerY -= 0.01 * settings.scale * height * aspectRatio; break;
			case KEY_DOWN: 	settings.centerY += 0.01 * settings.scale * height * aspectRatio; break;
			case KEY_LEFT: 	settings.centerX -= 0.01 * settings.scale * width; break;
			case KEY_RIGHT: settings.centerX += 0.01 * settings.scale * width; break;
			case 'w': 	settings.centerY -= 0.1 * settings.scale * height * aspectRatio; break;
			case 's': 	settings.centerY += 0.1 * settings.scale * height * aspectRatio; break;
			case 'a': 	settings.centerX -= 0.1 * settings.scale * width; break;
			case 'd': 	settings.centerX += 0.1 * settings.scale * width; break;
			case '+': 	settings.scale *= 0.8; break;
			case '-': 	settings.scale *= 1.2; break;
			case KEY_RESIZE: getmaxyx(stdscr, height, width); break;
		}
	}

	void run() {
		selectFractalMenu();

		while(running) {
			clear();

			switch (currentFractal) {
				case NEWTON_BASINS: renderNewtonBasins(); break;
				default: renderOtherFractals();
			}
			showFractalInfo();
			refresh();
			handleInput();
		}
		endwin();
	}	
};

int main() {
	FractalRenderer renderer;
	renderer.initialize();
	renderer.run();
	return 0;
}
