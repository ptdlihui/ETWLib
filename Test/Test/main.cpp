#include "test.h"
#include <QtWidgets/QApplication>


int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	Test w;
	w.show();
	return a.exec();
}
