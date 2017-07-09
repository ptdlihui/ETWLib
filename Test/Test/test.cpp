#include "test.h"
#include <iostream>
#include <Qtcore/QCoreApplication>

Test::Test(QWidget *parent): QWidget(parent)
{
	//ui.setupUi(this);
	grid = new QGridLayout;
	QGridLayout *grid_start_end_save = new QGridLayout;
	grid_start_end_save->addWidget(creatStartButton(), 0, 0);
	grid_start_end_save->addWidget(creatEndButton(), 0, 1);
	grid_start_end_save->addWidget(creatFileBUtton(), 0, 2);
	grid->addLayout(grid_start_end_save, 0, 0);
	grid->addWidget(creatFilter(), 0, 1);
	
	QScrollArea *scrollarea = new QScrollArea;
	scrollarea->setWidget(creatProvides());
	
	grid->addWidget(scrollarea, 1, 0);
	
	grid->setColumnStretch(0, 1);
	grid->setColumnStretch(1, 1);
	

	setLayout(grid);
	setWindowTitle(tr("Test Version"));
	const wchar_t* privilege[1] = {SE_SYSTEM_PROFILE_NAME};
	bool tokenValid = ETWLib::GrantPrivilegeW(privilege, 1);
	
	params = new ETWLib::SessionParameters();
	params->EnableProfilling(true);

	session = new ETWLib::ETWSession(L"TraceTest", L"Test.etl");
	
	resize(480, 320);
}

QPushButton* Test::creatStartButton() 
{
	QPushButton* start = new QPushButton(tr("Start"));
	start->setChecked(true);
	connect(start, SIGNAL(clicked()), this, SLOT(handleStart()));
	return start;
}

QPushButton* Test::creatEndButton()
{
	QPushButton* end = new QPushButton(tr("End"));
	end->setCheckable(true);
	connect(end, SIGNAL(clicked()), this, SLOT(handleEnd()));	
	return end;
}

QPushButton* Test::creatFileBUtton() 
{
	QPushButton* path = new QPushButton(tr("Save file as"));
	path->setCheckable(true);
	connect(path, SIGNAL(clicked()), this, SLOT(handleFileSaving()));
	return path;
}

QGroupBox* Test::creatProvides()
{
	QGroupBox *groupBox = new QGroupBox(tr("All Providers"));
	groupBox->setFlat(true);

	std::vector<std::wstring> allproviders_name = ETWLib::GetUserProvidersName();
	QVBoxLayout *vbox = new QVBoxLayout;
	for (int i = 0; i < allproviders_name.size(); ++i) 
	{
		const QString qprovider_str = QString::fromStdWString(allproviders_name[i]);
		QCheckBox *checkBox = new QCheckBox(qprovider_str);
		connect(checkBox, SIGNAL(stateChanged(int)), this, SLOT(checkBoxClicked(int)));
		vbox->addWidget(checkBox);
	}
	vbox->addStretch(1);
	groupBox->setLayout(vbox);

	return groupBox;
}

QLineEdit* Test::creatFilter() 
{
	QLineEdit *lineedit = new QLineEdit;
	lineedit->setPlaceholderText("Search here!");
	lineedit->setFocus();
	connect(lineedit, SIGNAL(textEdited(const QString)), this, SLOT(handleFilter()));
	return lineedit;
}
void Test::handleStart() 
{
	session->SetParameters(*params);
	ULONG status = session->StartSession(ETWLib::LogFileMode);
	std::cout << "already start" << std::endl;
}

void Test::handleEnd() 
{
	for (int i = 0; i < 10; i++)
	{
		Sleep(1 * 100);
		printf("%d\n", i + 1);
	}
	session->CloseSession();
}

void Test::handleFilter()
{
	QGroupBox* groupbox = new QGroupBox(tr("searching result after filter"));
	groupbox->setFlat(true);
	std::vector<std::wstring> allproviders_name = ETWLib::GetUserProvidersName();
	QObject* sender = QObject::sender();
	QString filterqstr = ((QLineEdit*)sender)->text();
	QVBoxLayout *vbox = new QVBoxLayout;
	for (int i = 0; i < allproviders_name.size(); ++i) 
	{
		QString providerqstr = QString::fromStdWString(allproviders_name[i]);
		if (providerqstr.contains(filterqstr, Qt::CaseInsensitive)) 
		{
			QCheckBox *checkBox = new QCheckBox(providerqstr);
			connect(checkBox, SIGNAL(stateChanged(int)), this, SLOT(checkBoxClicked(int)));
			vbox->addWidget(checkBox);
		}	
	}
	vbox->addStretch(1);
	groupbox->setLayout(vbox);
	QScrollArea *scrollareafilter = new QScrollArea;
	grid->addWidget(scrollareafilter, 1, 1);
	scrollareafilter->setWidget(groupbox);
}

void Test::handleFileSaving()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"),
		"",
		tr("File (*.txt)"));
}
void Test::checkBoxClicked(int state) 
{
	QObject* sender = QObject::sender();
	std::string str = ((QCheckBox*)sender)->text().toStdString();
	std::wstring wstr(str.length(), L' '); 
	std::copy(str.begin(), str.end(), wstr.begin());//std::string to std::wstring	
	//if (((QCheckBox*)sender)->isChecked())
	if(state == Qt::Checked)
	{
		params->AddUserModeProvider(wstr, true);
	}
	else
	{
		params->EraseUserModeProvider(wstr);
	}
	
}