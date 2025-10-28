#include <ncurses.h>
#include <complex>
#include <cmath>

using namespace std;

class FractalRenderer {
private:
	double centerX, centerY;	// Center of the FOV
	double scale;				// Horisontal size of one symbol
	int width, height;			// Height and width in symbols
	int maxiter;				// Maximum number of iterations
	bool shouldExit;

public:
	FractalRenderer(): centerX(-1.0), centerY(0), scale(0.03), maxiter(100), shouldExit(false) {}
	
	void initialize() {
		initscr();	// Create standard screen
		cbreak();	// Disable line buffering
		noecho();	// Obvious
		keypad(stdscr, TRUE);	// Enable special keyboard keys (i'll use arrows)
		curs_set(0);			// Hide the cursor
		getmaxyx(stdscr, height, width);	// Get terminal dimensions
	}

	char getPixelChar(int iter) {
		const char* chars = " .:-=+*#%@";	// Palette
		int index = (iter * sizeof(chars)) / maxiter;
		return index < sizeof(chars) ? chars[index] : chars[sizeof(chars) - 1];
	}

	void renderMandelbrot(){
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				double cx = (x - width/2.) * scale + centerX;	// X coordinate of the point
				double cy = (y - height/2.) * scale + centerY;	// Y coordinate of the point

				complex<double> c(cx, cy);
				complex<double> z(0, 0);

				int iteration = 0;
				while (abs(z) < 2. && iteration <= maxiter) {
					z = z*z + c;
					++iteration;
				}

				char pixel = getPixelChar(iteration);
				mvaddch(y, x, pixel);
			}
		}
	}

	void handleInput() {
		int ch = getch();
		switch(ch) {
			case 'q': 	shouldExit = true; break;
			case KEY_UP: 	centerY -= 0.1 * scale; break;
			case KEY_DOWN: 	centerY += 0.1 * scale; break;
			case KEY_LEFT: 	centerX -= 0.1 * scale; break;
			case KEY_RIGHT: centerX += 0.1 * scale; break;
			case 'w': 	centerY -= 10. * scale; break;
			case 's': 	centerY += 10. * scale; break;
			case 'a': 	centerX -= 10. * scale; break;
			case 'd': 	centerX += 10. * scale; break;
			case '+': 	scale *= 0.8; break;
			case '-': 	scale *= 1.2; break;
			case KEY_RESIZE: getmaxyx(stdscr, height, width); break;
		}
	}

	void run() {
		while(!shouldExit) {
			clear();
			renderMandelbrot();
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
