#ifndef TEST_H
#define TEST_H

#include <etwlib.h>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include "ui_test.h"

class Test : public QWidget
{
	Q_OBJECT

public:
	Test(QWidget *parent = 0);
private slots:
	void handleStart();
	void handleEnd();
	//void addUserModeProvider(std::wstring name, bool val);
	void checkBoxClicked(int state);
	void handleFilter();
	void handleFileSaving();
private:
	QGridLayout *grid;
	QPushButton *creatStartButton();
	QPushButton *creatEndButton();
	QPushButton* creatFileBUtton();
	QGroupBox *creatProvides();
	QLineEdit *creatFilter();
	ETWLib::SessionParameters* params;
	ETWLib::ETWSession* session;
	Ui::TestClass ui;
};

#endif // TEST_H
