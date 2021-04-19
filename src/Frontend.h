

#ifndef WKTERRAINSYNC_FRONTEND_H
#define WKTERRAINSYNC_FRONTEND_H


class Frontend {
private:
	static void __stdcall hookChangeScreen(int screen);
public:
	static int callMessageBox(char * message, int a2, int a3);
	static void install();
};


#endif //WKTERRAINSYNC_FRONTEND_H
