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
#include <QtWidgets/QMessageBox>
#include "ui_test.h"

class Test : public QWidget
{
	Q_OBJECT

public:
	Test(QWidget *parent = 0);
private slots:
	void HandleSave();
	void HandleStart();
	void HandleEnd();
	void HandleFilter();
	void CheckBoxClicked(int state);
	void SeclectAllProviders(int state);
	void SeclectAllFiltedProviders(int state);
private:
	std::vector<std::wstring> allProvidersName;
	QPushButton *CreatStartButton();
	QPushButton *CreatEndButton();
	QPushButton* CreatSavePathButton();
	QCheckBox *CreatSelectAllCheckBox();
	QGroupBox *CreatProvidesGroupBox();
	QLineEdit *CreatFilterLineEdit(); 
	QCheckBox *selcetAll;
	QScrollArea *scrollAreaAllProvider;
	QVBoxLayout *vBoxAllProviders;
	QGroupBox* groupBoxAllProviders;
	QLineEdit* filter;
	QGridLayout* grid;
	QPushButton* start;
	QPushButton* end;
	QPushButton* save;
	ETWLib::ETWSession* session;
	ETWLib::SessionParameters* param;
	std::vector<QCheckBox*> vecAllProviders;//1079
	std::vector<QCheckBox*> vecAllFilterProviders;
	std::wstring filePath;
	//Ui::TestClass ui;
};

#endif // TEST_H
