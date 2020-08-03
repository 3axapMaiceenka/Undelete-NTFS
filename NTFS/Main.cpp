#include "UserInterface.h"

int main(int argc, char** argv)
{
	ntfs::Controller controller;
	if (!controller.start())
	{
		return 1;
	}

	ntfs::UserInterface ui(controller); 
	ui.start();

	return 0;
}