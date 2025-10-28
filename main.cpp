#include <ncurses.h>
#include <complex>
#include <cstring>

using namespace std;

enum FractalType {
	MANDELBROT,		// 0
	JULIA,			// 1
	BURNING_SHIP,	// 2
	FRACTAL_COUNT	// 4
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
	double aspectRatio;		// Symbol height/width
	int width, height;			// Height and width of the terminal in symbols
	int maxiter;				// Maximum number of iterations
	bool running;				// Breaking the while cycle in main()
	const char* fractalNames[3] = {"Mandelbrot", "Julia", "Burning Ship"};

public:
	FractalRenderer(): currentFractal(MANDELBROT), aspectRatio(2.11), running(true), maxiter(100) {
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
	}
	
	void initialize() {
		initscr();	// Create standard screen
		cbreak();	// Disable line buffering
		noecho();	// Obvious
		keypad(stdscr, TRUE);	// Enable special keyboard keys (i'll use arrows)
		curs_set(0);			// Hide the cursor
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
			mvprintw(startY + FRACTAL_COUNT+3, startX-10, "1-3 - quick select   q - exit program");

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
						selected = static_cast<FractalType>((selected - 1) % FRACTAL_COUNT); break;
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
		double gamma = 1.8;
		t = pow(t, 1./gamma);
		
		int index = t * (paletteSize - 1);
		return chars[index];
	}

	int mandelbrotPoint(double cx, double cy) {
		complex<double> c(cx, cy);
		complex<double> z(0, 0);

		int iteration = 0;
		while (abs(z) < 2. && iteration < maxiter) {
			z = z*z + c;
			++iteration;
		}
		return iteration;
	}

	int juliaPoint(double zx, double zy) {
		complex<double> z(zx, zy);
		complex<double> c(fractalSettings[JULIA].juliaCx, fractalSettings[JULIA].juliaCy);

		int iteration = 0;
		while (abs(z) < 2. && iteration < maxiter){
			z = z*z + c;
			++iteration;
		}
		return iteration;
	}

	int burningShipPoint(double cx, double cy) {
		double zx = 0, zy = 0;
		int iteration = 0;

		while (zx*zx + zy*zy < 4.0 && iteration < maxiter) {
			double new_zx = zx*zx - zy*zy + cx;
			zy = 2 * abs(zx*zy) + cy;
			zx = new_zx;
			++iteration;
		}
		return iteration;
	}

	void renderFractal() {
		FractalSettings& settings = fractalSettings[currentFractal];
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				double cx = (x - width/2.) * settings.scale + settings.centerX;	// X coordinate of the point
				double cy = (y - height/2.) * settings.scale * aspectRatio + settings.centerY;	// Y coordinate of the point

				int iteration;
				switch (currentFractal) {
					case MANDELBROT: 	iteration = mandelbrotPoint(cx, cy); break;
					case JULIA:			iteration = juliaPoint(cx, cy); break;
					case BURNING_SHIP:	iteration = burningShipPoint(cx, cy); break;
				}
				
				char pixel = getPixelChar(iteration);
				mvaddch(y, x, pixel);
			}
		}
		
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

			renderFractal();
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
